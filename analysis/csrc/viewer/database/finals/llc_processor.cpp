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

LLCProcessor::OriDataFormat LLCProcessor::GetData(const std::string &dbPath, DBInfo &dbInfo)
{
    OriDataFormat data;
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, data, dbPath);
    std::string sql {"SELECT device_id, l3tid, timestamp, hitrate, throughput FROM " + dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, data)) {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return data;
}

LLCProcessor::ProcessedDataFormat LLCProcessor::FormatData(const OriDataFormat &oriData, const ThreadData &threadData,
                                                           const uint16_t mode)
{
    ProcessedDataFormat processedData;
    if (oriData.empty()) {
        ERROR("LLC oriData is empty.");
        return processedData;
    }
    if (!Reserve(processedData, oriData.size())) {
        ERROR("Reserve for LLC data failed.");
        return processedData;
    }
    LLCData llcData;
    for (auto &row: oriData) {
        std::tie(llcData.deviceId, llcData.llcID, llcData.timestamp, llcData.hitRate, llcData.throughput) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(llcData.timestamp,
                                                       threadData.hostMonotonic - threadData.deviceMonotonic);
        processedData.emplace_back(
            llcData.deviceId, llcData.llcID, GetLocalTime(timestamp, threadData.timeRecord).Uint64(),
            llcData.hitRate * PERCENTAGE, static_cast<uint64_t>(llcData.throughput * BYTE_SIZE * BYTE_SIZE), mode);
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
    ThreadData threadData;
    DBInfo llcDB("llc.db", "LLCMetrics");
    auto deviceList = File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetClockMonotonicRaw(threadData.hostMonotonic, HOST_ID, fileDir);
    bool timeRecordFlag = Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir);
    for (const auto &devicePath: deviceList) {
        std::string dbPath = File::PathJoin({devicePath, SQLITE, llcDB.dbName});
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
        threadData.deviceId = GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetClockMonotonicRaw(threadData.deviceMonotonic, threadData.deviceId, fileDir) ||
                (threadData.hostMonotonic < threadData.deviceMonotonic)) {
            ERROR("Device MonotonicRaw is invalid in path: %., device id is %", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        auto iter = MEMORY_TABLE.find(Context::GetInstance().GetLLCProfiling(threadData.deviceId, fileDir));
        if (iter == MEMORY_TABLE.end()) {
            ERROR("GetLLCProfiling failed in path: %, device id is %", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        uint16_t mode = iter->second;
        INFO("Start to process %, deviceId:%.", dbPath, threadData.deviceId);
        auto oriData = GetData(dbPath, llcDB);
        auto processedData = FormatData(oriData, threadData, mode);
        if (!SaveData(processedData, TABLE_NAME_LLC)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process %, deviceId:% ends.", dbPath, threadData.deviceId);
    }
    return flag;
}

}  // Database
}  // Viewer
}  // Analysis
