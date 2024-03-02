/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_processor.h
 * Description        : hccs_processor，处理hccs表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/29
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/hccs_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct HCCSOriData {
    uint32_t deviceId = 0;
    double timestamp = 0.0;
    double txThroughput = 0.0;
    double rxThroughput = 0.0;
};
}

HCCSProcessor::HCCSProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool HCCSProcessor::Run()
{
    INFO("HCCSProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_HCCS);
    return flag;
}

bool HCCSProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    DBInfo hccsDB("hccs.db", "HCCSEventsData");
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hccsDB.dbName});
        // 并不是所有场景都有hccs数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        if (!timeFlag) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            return false;
        }
        HccsDataFormat hccsData = GetData(dbPath, hccsDB);
        ProcessedDataFormat processedData;
        if (!FormatData(timeRecord, hccsData, processedData)) {
            ERROR("FormatData failed, fileDir is %.", fileDir);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, TABLE_NAME_HCCS)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process dbpath % ends.", dbPath);
    }
    INFO("process % ends.", fileDir);
    return flag;
}


HCCSProcessor::HccsDataFormat HCCSProcessor::GetData(const std::string &dbPath, DBInfo &hccsDB)
{
    INFO("HCCSProcessor GetData, dir is %.", dbPath);
    HccsDataFormat hccsData;
    MAKE_SHARED_RETURN_VALUE(hccsDB.dbRunner, DBRunner, hccsData, dbPath);
    if (hccsDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return hccsData;
    }
    std::string sql{"SELECT device_id, timestamp, txthroughput, rxthroughput FROM " + hccsDB.tableName};
    if (!hccsDB.dbRunner->QueryData(sql, hccsData)) {
        ERROR("Failed to obtain data from the % table.", hccsDB.tableName);
    }
    return hccsData;
}

bool HCCSProcessor::FormatData(const Utils::ProfTimeRecord timeRecord,
                               const HccsDataFormat &hccsData, ProcessedDataFormat &processedData)
{
    INFO("HCCSProcessor FormatData.");
    if (hccsData.empty()) {
        ERROR("Hccs original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, hccsData.size())) {
        ERROR("Reserve for Hccs data failed.");
        return false;
    }
    HCCSOriData tempData;
    for (auto &row: hccsData) {
        std::tie(tempData.deviceId, tempData.timestamp, tempData.txThroughput, tempData.rxThroughput) = row;
        HPFloat timestamp{tempData.timestamp};
        // 暂时不做数据格式转换,目前是MB/s
        processedData.emplace_back(static_cast<uint16_t>(tempData.deviceId),
                                   GetLocalTime(timestamp, timeRecord).Uint64(),
                                   tempData.txThroughput, tempData.rxThroughput);
    }
    if (processedData.empty()) {
        ERROR("Hccs data processing error.");
        return false;
    }
    return true;
}

}
}
}