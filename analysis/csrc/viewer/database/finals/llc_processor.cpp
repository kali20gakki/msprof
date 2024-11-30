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

#include <unordered_set>

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "collector/dvvp/common/config/config.h"


namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
using namespace analysis::dvvp::common::config;
namespace {
struct LLCData {
    uint16_t deviceId = 0;
    uint32_t llcID = 0;
    double timestamp = 0.0;
    double hitRate = 0.0;
    double throughput = 0.0;
};

std::unordered_set<std::string> LLC_MODES = {LLC_PROFILING_READ, LLC_PROFILING_WRITE};
}

LLCProcessor::LLCProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths)
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
                                                           const uint64_t mode)
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
                                                       threadData.hostMonotonic, threadData.deviceMonotonic);
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
    DBInfo llcDB("llc.db", "LLCMetrics");
    auto deviceList = File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto &devicePath: deviceList) {
        ThreadData threadData;
        std::string dbPath = File::PathJoin({devicePath, SQLITE, llcDB.dbName});
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        threadData.deviceId = GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir, threadData.deviceId)) {
            ERROR("Failed to obtain the time in start_info and end_info. "
                  "Path is %, device is id %.", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        if (!Context::GetInstance().GetClockMonotonicRaw(threadData.hostMonotonic, true,
                                                         threadData.deviceId, fileDir) ||
            !Context::GetInstance().GetClockMonotonicRaw(threadData.deviceMonotonic, false,
                                                         threadData.deviceId, fileDir)) {
            ERROR("Device MonotonicRaw is invalid in path: %., device id is %", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        std::string mode = Context::GetInstance().GetLLCProfiling(threadData.deviceId, fileDir);
        if (LLC_MODES.find(mode) == LLC_MODES.end()) {
            ERROR("GetLLCProfiling failed in path: %, device id is %", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        INFO("Start to process %, deviceId:%.", dbPath, threadData.deviceId);
        auto oriData = GetData(dbPath, llcDB);
        auto processedData = FormatData(oriData, threadData, IdPool::GetInstance().GetUint64Id(mode));
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
