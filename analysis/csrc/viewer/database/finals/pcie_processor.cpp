/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_processor.cpp
 * Description        : 处理pcie数据
 * Author             : msprof team
 * Creation Date      : 2024/03/02
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/pcie_processor.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;
namespace {
struct BandwidthData {
    double min = 0.0;
    double max = 0.0;
    double avg = 0.0;
};

struct PCIeOriData {
    uint32_t deviceId = UINT32_MAX;
    uint64_t timestamp = UINT64_MAX;
    BandwidthData txPost;
    BandwidthData txNopost;
    BandwidthData txCpl;
    BandwidthData txNopostLatency;
    BandwidthData rxPost;
    BandwidthData rxNopost;
    BandwidthData rxCpl;
};
}

PCIeProcessor::PCIeProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool PCIeProcessor::Run()
{
    INFO("PCIeProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_PCIE);
    return flag;
}

bool PCIeProcessor::Process(const std::string &fileDir)
{
    INFO("PCIeProcessor Process, dir is %", fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    DBInfo pcieDB("pcie.db", "PcieOriginalData");
    bool flag = true;
    Utils::ProfTimeRecord timeRecord;
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, pcieDB.dbName});
        // 并不是所有场景都有PCIe数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        if (!timeFlag) {
            ERROR("Failed to GetProfTimeRecordInfo, fileDir is %.", fileDir);
            return false;
        }
        PCIeDataFormat pcieData = GetData(dbPath, pcieDB);
        ProcessedDataFormat processedData;
        if (!FormatData(timeRecord, pcieData, processedData)) {
            ERROR("FormatData failed, fileDir is %.", fileDir);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, TABLE_NAME_PCIE)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
    }
    return flag;
}


PCIeProcessor::PCIeDataFormat PCIeProcessor::GetData(const std::string &dbPath, DBInfo &pcieDB)
{
    INFO("PCIeProcessor GetData, dir is %.", dbPath);
    PCIeDataFormat pcieData;
    MAKE_SHARED_RETURN_VALUE(pcieDB.dbRunner, DBRunner, pcieData, dbPath);
    if (pcieDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return pcieData;
    }
    std::string sql = "SELECT device_id, timestamp, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, "
                      "tx_np_bandwidth_min, tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, "
                      "tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg, tx_np_lantency_min, tx_np_lantency_max, "
                      "tx_np_lantency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max, rx_p_bandwidth_avg, "
                      "rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min, "
                      "rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg "
                      "FROM "+ pcieDB.tableName +" WHERE tx_p_bandwidth_max >= tx_p_bandwidth_min" ;
    if (!pcieDB.dbRunner->QueryData(sql, pcieData)) {
        ERROR("Query PCIe data failed, db path is %.", dbPath);
        return pcieData;
    }
    return pcieData;
}

bool PCIeProcessor::FormatData(const Utils::ProfTimeRecord timeRecord,
                               const PCIeDataFormat &pcieData, ProcessedDataFormat &processedData)
{
    INFO("PCIeProcessor FormatData.");
    if (pcieData.empty()) {
        ERROR("Pcie original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, pcieData.size())) {
        ERROR("Reserve for PCIe data failed.");
        return false;
    }
    PCIeOriData tempData;
    for (const auto& data : pcieData) {
        std::tie(tempData.deviceId, tempData.timestamp,
                 tempData.txPost.min, tempData.txPost.max, tempData.txPost.avg,
                 tempData.txNopost.min, tempData.txNopost.max, tempData.txNopost.avg,
                 tempData.txCpl.min, tempData.txCpl.max, tempData.txCpl.avg,
                 tempData.txNopostLatency.min, tempData.txNopostLatency.max, tempData.txNopostLatency.avg,
                 tempData.rxPost.min, tempData.rxPost.max, tempData.rxPost.avg,
                 tempData.rxNopost.min, tempData.rxNopost.max, tempData.rxNopost.avg,
                 tempData.rxCpl.min, tempData.rxCpl.max, tempData.rxCpl.avg) = data;
        Utils::HPFloat timestamp{tempData.timestamp};
        // 暂时不做数据格式转换,目前是B/us
        processedData.emplace_back(static_cast<uint16_t>(tempData.deviceId),
                                   Utils::GetLocalTime(timestamp, timeRecord).Uint64(),
                                   tempData.txPost.min, tempData.txPost.max, tempData.txPost.avg,
                                   tempData.txNopost.min, tempData.txNopost.max, tempData.txNopost.avg,
                                   tempData.txCpl.min, tempData.txCpl.max, tempData.txCpl.avg,
                                   tempData.txNopostLatency.min, tempData.txNopostLatency.max,
                                   tempData.txNopostLatency.avg,
                                   tempData.rxPost.min, tempData.rxPost.max, tempData.rxPost.avg,
                                   tempData.rxNopost.min, tempData.rxNopost.max, tempData.rxNopost.avg,
                                   tempData.rxCpl.min, tempData.rxCpl.max, tempData.rxCpl.avg);
    }
    if (processedData.empty()) {
        ERROR("PCIe data processing error.");
        return false;
    }
    return true;
}

} // Database
} // Viewer
} // Analysis