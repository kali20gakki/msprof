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
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct BandwidthData {
    uint32_t min = UINT32_MAX;
    uint32_t max = UINT32_MAX;
    uint32_t avg = UINT32_MAX;
};

struct PCIeOriData {
    uint32_t deviceId = UINT32_MAX;
    uint64_t timestamp = UINT64_MAX;
    BandwidthData txPost;
    BandwidthData txNonpost;
    BandwidthData txCpl;
    BandwidthData txNonpostLatency;
    BandwidthData rxPost;
    BandwidthData rxNonpost;
    BandwidthData rxCpl;
};
}

PCIeProcessor::PCIeProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

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
    ThreadData threadData;
    auto deviceList = File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    DBInfo pcieDB("pcie.db", "PcieOriginalData");
    bool flag = true;
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = File::PathJoin({devicePath, SQLITE, pcieDB.dbName});
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
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetClockMonotonicRaw(threadData.hostMonotonic, true, deviceId, fileDir) ||
            !Context::GetInstance().GetClockMonotonicRaw(threadData.deviceMonotonic, false, deviceId, fileDir)) {
            ERROR("Device MonotonicRaw is invalid in path: %., device id is %", fileDir, deviceId);
            flag = false;
            continue;
        }
        PCIeDataFormat pcieData = GetData(dbPath, pcieDB);
        ProcessedDataFormat processedData;
        if (!FormatData(threadData, pcieData, processedData)) {
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

bool PCIeProcessor::FormatData(const ThreadData &threadData,
                               const PCIeDataFormat &pcieData, ProcessedDataFormat &processedData)
{
    INFO("PCIeProcessor FormatData.");
    if (pcieData.empty()) {
        ERROR("Pcie original data is empty.");
        return false;
    }
    if (!Reserve(processedData, pcieData.size())) {
        ERROR("Reserve for PCIe data failed.");
        return false;
    }
    PCIeOriData tempData;
    for (const auto& data : pcieData) {
        std::tie(tempData.deviceId, tempData.timestamp,
                 tempData.txPost.min, tempData.txPost.max, tempData.txPost.avg,
                 tempData.txNonpost.min, tempData.txNonpost.max, tempData.txNonpost.avg,
                 tempData.txCpl.min, tempData.txCpl.max, tempData.txCpl.avg,
                 tempData.txNonpostLatency.min, tempData.txNonpostLatency.max, tempData.txNonpostLatency.avg,
                 tempData.rxPost.min, tempData.rxPost.max, tempData.rxPost.avg,
                 tempData.rxNonpost.min, tempData.rxNonpost.max, tempData.rxNonpost.avg,
                 tempData.rxCpl.min, tempData.rxCpl.max, tempData.rxCpl.avg) = data;
        HPFloat timestamp = GetTimeBySamplingTimestamp(tempData.timestamp,
                                                       threadData.hostMonotonic, threadData.deviceMonotonic);
        // B/us -> B/s, txNopostLatency 单位是ns
        processedData.emplace_back(static_cast<uint16_t>(tempData.deviceId),
                                   GetLocalTime(timestamp, threadData.timeRecord).Uint64(),
                                   tempData.txPost.min * MICRO_SECOND, tempData.txPost.max * MICRO_SECOND,
                                   tempData.txPost.avg * MICRO_SECOND, tempData.txNonpost.min * MICRO_SECOND,
                                   tempData.txNonpost.max * MICRO_SECOND, tempData.txNonpost.avg * MICRO_SECOND,
                                   tempData.txCpl.min * MICRO_SECOND, tempData.txCpl.max * MICRO_SECOND,
                                   tempData.txCpl.avg * MICRO_SECOND, tempData.txNonpostLatency.min,
                                   tempData.txNonpostLatency.max, tempData.txNonpostLatency.avg,
                                   tempData.rxPost.min * MICRO_SECOND, tempData.rxPost.max * MICRO_SECOND,
                                   tempData.rxPost.avg * MICRO_SECOND, tempData.rxNonpost.min * MICRO_SECOND,
                                   tempData.rxNonpost.max * MICRO_SECOND, tempData.rxNonpost.avg * MICRO_SECOND,
                                   tempData.rxCpl.min * MICRO_SECOND, tempData.rxCpl.max * MICRO_SECOND,
                                   tempData.rxCpl.avg * MICRO_SECOND);
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