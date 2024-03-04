/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_processor.cpp
 * Description        : llc_processor，处理LLC表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/26
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/llc_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"


namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct LLCData {
    uint16_t deviceId = 0;
    uint32_t llcID = 0;
    double timestamp = 0.0;
    double hitRate = 0.0;
    double throughput = 0.0;
};
}

LLCProcessor::LLCProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths)
{}

bool LLCProcessor::Run()
{
    INFO("LLCProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_LLC);
    return flag;
}

LLCProcessor::OriDataFormat LLCProcessor::GetData(DBInfo &dbInfo)
{
    OriDataFormat data;
    std::string sql {"SELECT device_id, l3tid, timestamp, hitrate, throughput FROM " + dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, data)) {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return data;
}

LLCProcessor::ProcessedDataFormat LLCProcessor::FormatData(const OriDataFormat &oriData, uint16_t mode,
                                                           const Utils::ProfTimeRecord &timeRecord,
                                                           SyscntConversionParams &params)
{
    ProcessedDataFormat processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for LLC data failed.");
        return processedData;
    }
    LLCData llcData;
    for (auto &row: oriData) {
        std::tie(llcData.deviceId, llcData.llcID, llcData.timestamp, llcData.hitRate, llcData.throughput) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(llcData.timestamp, params);
        processedData.emplace_back(
            llcData.deviceId, llcData.llcID, GetLocalTime(timestamp, timeRecord).Uint64(),
            llcData.hitRate, llcData.throughput, mode);
    }
    return processedData;
}

bool LLCProcessor::Process(const std::string &fileDir)
{
    auto platformVersion = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, fileDir);
    if (Context::IsFirstChipV1(platformVersion)) {
        WARN("Skip llc processing in path: %.", fileDir);
        return true;
    }
    INFO("Start to process %.", fileDir);
    if (!ProcessData(fileDir)) {
        ERROR("process % failed.", fileDir);
        return false;
    }
    INFO("process % ends.", fileDir);
    return true;
}

bool LLCProcessor::ProcessData(const std::string &fileDir)
{
    bool flag = true;
    Utils::ProfTimeRecord timeRecord;
    Utils::SyscntConversionParams params;
    DBInfo llcDB("llc.db", "LLCMetrics");
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, fileDir);
    bool timeRecordFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto &devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, llcDB.dbName});
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        if (!timeFlag || !timeRecordFlag) {
            ERROR("Get param flag: %, get time record flag: %, in path %.", timeFlag, timeRecordFlag, fileDir);
            return false;
        }
        uint16_t deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        INFO("Start to process %, deviceId:%.", dbPath, deviceId);
        auto iter = MEMORY_TABLE.find(Context::GetInstance().GetLLCProfiling(deviceId, fileDir));
        if (iter == MEMORY_TABLE.end()) {
            ERROR("Failed to obtain llc profiling from sample.json in path: %.", devicePath);
            flag = false;
            continue;
        }
        uint16_t mode = iter->second;
        MAKE_SHARED_RETURN_VALUE(llcDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(llcDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data empty in %.", llcDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, mode, timeRecord, params);
        if (!SaveData(processedData, TABLE_NAME_LLC)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process %, deviceId:% ends.", dbPath, deviceId);
    }
    return flag;
}
}  // Database
}  // Viewer
}  // Analysis
