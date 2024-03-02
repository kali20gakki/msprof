/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_processor.cpp
 * Description        : pmu 处理task-based和sample-based时的数据
 * Author             : msprof team
 * Creation Date      : 2024/2/26
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/pmu_processor.h"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;
using namespace Association::Credential;
using namespace Analysis::Utils;

namespace {
const std::set<std::string> SampleBasedDBNames = {"ai_vector_core_", "aicore_"};
const std::string TaskBased = "task-based";
const double INVALID_FREQ = 0.0;

struct SampleTimelineData {
    double timestamp = 0.0;
    uint32_t coreid = UINT32_MAX;
    std::string task_cyc;
};

struct SampleSummaryData {
    std::string metric;
    double value = 0.0;
    uint32_t coreid = UINT32_MAX;
};
}

PmuProcessor::PmuProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool PmuProcessor::Run()
{
    INFO("PmuProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_PMU);
    return flag;
}

bool PmuProcessor::Process(const std::string &fileDir)
{
    std::string metricMode;
    if (!Context::GetInstance().GetMetricMode(metricMode, fileDir)) {
        ERROR("GetMetricMode failed.");
        return false;
    }
    if (metricMode == TaskBased) {
        // 对于task-based pmu数据,目前只支持stars芯片(仅stars支持获取correlationId)
        auto version = Context::GetInstance().GetPlatformVersion(Parser::Environment::DEFAULT_DEVICE_ID, fileDir);
        if (!Context::GetInstance().IsStarsChip(version)) {
            WARN("This platformVersion does not support the processing of pmu data.");
            return true;
        }
        return true;
    }
    return SampleBasedProcess(fileDir);
}

bool PmuProcessor::SampleBasedProcess(const std::string &fileDir)
{
    INFO("SampleBasedProcess, dir is %", fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    std::unordered_map<std::string, uint16_t> dbPathTable;
    for (const auto& name : SampleBasedDBNames) {
        for (const auto& devicePath : deviceList) {
            auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
            std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, name + std::to_string(deviceId) + ".db"});
            auto status = CheckPath(dbPath);
            if (status == CHECK_SUCCESS) {
                dbPathTable.insert({dbPath, deviceId});
            } else if (status == CHECK_FAILED) {
                return false;
            }
        }
    }
    bool flag = true;
    if (!SampleBasedTimelineProcess(dbPathTable, fileDir)) {
        ERROR("Failed to process sample-based timeline data, fileDir is %.", fileDir);
        flag = false;
    }
    if (!SampleBasedSummaryProcess(dbPathTable)) {
        ERROR("Failed to process sample-based summary data, fileDir is %.", fileDir);
        flag = false;
    }
    return flag;
}

bool PmuProcessor::SampleBasedTimelineProcess(const std::unordered_map<std::string, uint16_t> &dbPathTable,
                                              const std::string &fileDir)
{
    INFO("SampleBasedTimeline");
    bool flag = true;
    ThreadData threadData;
    if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    for (const auto& pair : dbPathTable) {
        threadData.deviceId = pair.second;
        if (!Context::GetInstance().GetPmuFreq(threadData.freq, threadData.deviceId, fileDir) ||
                IsDoubleEqual(threadData.freq, INVALID_FREQ)) {
            ERROR("GetPmuFreq failed or aic freq is invalid, profPath is %.", fileDir);
            return false;
        }
        auto oriData = GetSampleBasedTimelineData(pair.first);
        PSTFormat processedData;
        if (!FormatSampleBasedTimelineData(oriData, processedData, threadData)) {
            ERROR("FormatSampleBasedTimelineData failed, dbPath is %.", pair.first);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, TABLE_NAME_SAMPLE_PMU_TIMELINE)) {
            ERROR("SampleBasedTimeline: saveData failed, dbPath is %.", pair.first);
            flag = false;
        }
    }
    return flag;
}

PmuProcessor::OSTFormat PmuProcessor::GetSampleBasedTimelineData(const std::string &dbPath)
{
    INFO("GetSampleBasedTimelineData, dbPath is %.", dbPath);
    OSTFormat oriData;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, oriData, dbPath);
    if (dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT timestamp, task_cyc, coreid FROM AICoreOriginalData;";
    if (!dbRunner->QueryData(sql, oriData)) {
        ERROR("Query sample-based timeline data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool PmuProcessor::FormatSampleBasedTimelineData(const OSTFormat &oriData, PSTFormat &processedData,
                                                 const ThreadData &threadData)
{
    INFO("FormatSampleBasedTimelineData.");
    if (oriData.empty()) {
        ERROR("Sample-based timeline original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for % sample-based timeline data failed.");
        return false;
    }
    std::unordered_map<uint32_t, double> coreIdTimeMap;
    SampleTimelineData tempData;
    for (uint64_t i = 0; i < oriData.size(); ++i) {
        std::tie(tempData.timestamp, tempData.task_cyc, tempData.coreid) = oriData[i];
        uint64_t cycle;
        if (StrToU64(cycle, tempData.task_cyc) != ANALYSIS_OK) {
            ERROR("task_cyc to uint64_t failed.");
            return false;
        }
        auto corePreRecord = coreIdTimeMap.find(tempData.coreid);
        double interval = (corePreRecord == coreIdTimeMap.end()) ? 1.0 : (tempData.timestamp - corePreRecord->second);
        coreIdTimeMap[tempData.coreid] = tempData.timestamp;
        if (IsDoubleEqual(threadData.freq, INVALID_FREQ)) {
            WARN("The interval is invalid, the record timestamp is %, "
                 "core id is %.", tempData.timestamp, tempData.coreid);
            continue;
        }
        // task_cyc 单位为 次; freq单位为 MHz,即次/秒; interval单位为 ns
        // 由公式 (task_cyc / (freq * 10^6) / (interval / 10^9)) * 100% 化简而来
        double usage = (cycle * MILLI_SECOND) / (threadData.freq * interval) * PERCENTAGE;

        Utils::HPFloat timestamp{tempData.timestamp};
        processedData.emplace_back(threadData.deviceId, Utils::GetLocalTime(timestamp, threadData.timeRecord).Uint64(),
                                   cycle, usage, threadData.freq, static_cast<uint16_t>(tempData.coreid));
    }
    if (processedData.empty()) {
        ERROR("FormatSampleBasedSummaryData data processing error.");
        return false;
    }
    return true;
}

bool PmuProcessor::SampleBasedSummaryProcess(const std::unordered_map<std::string, uint16_t> &dbPathTable)
{
    INFO("SampleBasedSummary");
    bool flag = true;
    for (const auto& pair : dbPathTable) {
        auto oriData = GetSampleBasedSummaryData(pair.first);
        PSSFormat processedData;
        if (!FormatSampleBasedSummaryData(oriData, processedData, pair.second)) {
            ERROR("FormatSampleBasedSummaryData failed, dbPath is %.", pair.first);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, TABLE_NAME_SAMPLE_PMU_SUMMARY)) {
            ERROR("SampleBasedSummary: saveData failed, dbPath is %.", pair.first);
            flag = false;
        }
    }
    return flag;
}

PmuProcessor::OSSFormat PmuProcessor::GetSampleBasedSummaryData(const std::string &dbPath)
{
    INFO("GetSampleBasedSummaryData, dbPath is %.", dbPath);
    OSSFormat oriData;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, oriData, dbPath);
    if (dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT metric, value, coreid FROM MetricSummary WHERE value IS NOT NULL;";
    if (!dbRunner->QueryData(sql, oriData)) {
        ERROR("Query sample-based summary data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool PmuProcessor::FormatSampleBasedSummaryData(const OSSFormat &oriData, PSSFormat &processedData,
                                                const uint16_t deviceId)
{
    INFO("FormatSampleBasedSummaryData.");
    if (oriData.empty()) {
        ERROR("Sample-based summary original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for % sample-based summary data failed.");
        return false;
    }
    SampleSummaryData tempData;
    for (const auto& data : oriData) {
        std::tie(tempData.metric, tempData.value, tempData.coreid) = data;
        processedData.emplace_back(deviceId, IdPool::GetInstance().GetUint64Id(tempData.metric),
                                   tempData.value, static_cast<uint16_t>(tempData.coreid));
    }
    if (processedData.empty()) {
        ERROR("FormatSampleBasedSummaryData data processing error.");
        return false;
    }
    return true;
}

} // Database
} // Viewer
} // Analysis