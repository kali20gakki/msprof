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
const std::string AI_VECTOR_CORE_PREFIX = "ai_vector_core_";
const std::string TYPE_AIV = "AIV";
const std::string AI_CORE_PREFIX = "aicore_";
const std::string TYPE_AIC = "AIC";
const std::set<std::string> SAMPLE_BASED_DB_NAMES = {AI_VECTOR_CORE_PREFIX, AI_CORE_PREFIX};
const std::set<std::string> INVALID_COLUMN_NAMES = {"task_id", "stream_id", "subtask_id", "batch_id", "task_type",
                                                    "start_time", "end_time", "ffts_type", "core_type"};
const std::string TASK_BASED = "task-based";
const double DOUBLE_ZERO = 0.0;

struct TaskPmuData {
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t subtaskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    double value = 0.0;
};

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
    if (metricMode == TASK_BASED) {
        // 对于task-based pmu数据,目前只支持stars芯片(仅stars支持获取globalTaskId)
        auto version = Context::GetInstance().GetPlatformVersion(Parser::Environment::DEFAULT_DEVICE_ID, fileDir);
        if (!Context::GetInstance().IsStarsChip(version)) {
            WARN("This platformVersion does not support the processing of pmu data.");
            return true;
        }
        return TaskBasedProcess(fileDir);
    }
    return SampleBasedProcess(fileDir);
}

bool PmuProcessor::TaskBasedProcess(const std::string &fileDir)
{
    INFO("TaskBasedProcess, dir is %", fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    std::unordered_map<std::string, uint16_t> dbPathTable;
    for (const auto& devicePath : deviceList) {
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, "metric_summary.db"});
        auto status = CheckPath(dbPath);
        if (status == CHECK_SUCCESS) {
            dbPathTable.insert({dbPath, deviceId});
        } else if (status == CHECK_FAILED) {
            return false;
        }
    }
    DBInfo metricDB("metric_summary.db", "MetricSummary");
    auto tableColumns = GetAndCheckTableColumns(dbPathTable, metricDB);
    if (tableColumns.empty()) {
        ERROR("GetAndCheckTableColumns failed, please check tableColums are consistent.");
        return false;
    }
    bool flag = true;
    for (const auto& pair : dbPathTable) {
        for (const auto& tableColumn : tableColumns) {
            if (!TaskBasedProcessByColumnName(pair, tableColumn.name, metricDB)) {
                ERROR("TaskBasedProcess: process % data failed, dbPath is %.", tableColumn.name, pair.first);
                flag = false;
            }
        }
    }
    return flag;
}

TableColumns PmuProcessor::GetAndCheckTableColumns(std::unordered_map<std::string, uint16_t> dbPathTable,
                                                   DBInfo &metricDB)
{
    INFO("GetAndCheckTableColumns.");
    TableColumns tableColumns;
    for (const auto& pair : dbPathTable) {
        MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, tableColumns, pair.first);
        if (metricDB.dbRunner == nullptr) {
            ERROR("Create % connection failed.", pair.first);
        }
        if (tableColumns.empty()) {
            tableColumns = metricDB.dbRunner->GetTableColumns(metricDB.tableName);
        } else if (tableColumns != metricDB.dbRunner->GetTableColumns(metricDB.tableName)) {
            ERROR("%'s tableColumns are different from others.", pair.first);
            // 若发现在各db中表字段不同,则返回空表头
            tableColumns.clear();
            return tableColumns;
        }
    }
    return tableColumns;
}

bool PmuProcessor::TaskBasedProcessByColumnName(const std::pair<const std::string, uint16_t> dbRecord,
                                                const std::string &columnName, DBInfo &metricDB)
{
    if (INVALID_COLUMN_NAMES.find(columnName) != INVALID_COLUMN_NAMES.end()) {
        INFO("This column[%] does not need to be processed.", columnName);
        return true;
    }
    OTFormat pmuData = GetTaskBasedData(dbRecord.first, columnName, metricDB);
    PTFormat processedData;
    if (!FormatTaskBasedData(pmuData, processedData, columnName, dbRecord.second)) {
        ERROR("FormatData failed, dbPath is %.", dbRecord.first);
        return false;
    }
    if (!SaveData(processedData, TABLE_NAME_TASK_PMU_INFO)) {
        ERROR("Save data failed, %.", dbRecord.first);
        return false;
    }
    return true;
}

PmuProcessor::OTFormat PmuProcessor::GetTaskBasedData(const std::string &dbPath, const std::string &columnName,
                                                      DBInfo &metricDB)
{
    INFO("GetTaskBasedData, dbPath is %, column name is %.", dbPath, columnName);
    OTFormat oriData;
    MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, oriData, dbPath);
    if (metricDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT stream_id, task_id, subtask_id, batch_id, " + columnName + " "
                      "FROM " + metricDB.tableName;
    if (!metricDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query task-based % data failed, db path is %.", columnName, dbPath);
        return oriData;
    }
    return oriData;
}

bool PmuProcessor::FormatTaskBasedData(const OTFormat &oriData, PTFormat &processedData,
                                       const std::string &columnName, const uint16_t &deviceId)
{
    INFO("FormatTaskBasedData.");
    if (oriData.empty()) {
        ERROR("Task-based original data is empty, columnName is %.", columnName);
        return false;
    }
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for % task-based data failed, columnName is %.", columnName);
        return false;
    }
    // 当前task-based pmu存在两个问题:
    // 1、没有考虑memory_bound或是cube utilization的计算
    // 2、对于类似ratio_extra命名的字段没有做名称的刷新(去掉extra等动作)
    // 当前pmu time的单位为us
    TaskPmuData tempData;
    for (const auto &row: oriData) {
        std::tie(tempData.streamId, tempData.taskId, tempData.subtaskId, tempData.batchId, tempData.value) = row;
        uint64_t globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(static_cast<uint16_t>(deviceId),
                                                                            tempData.streamId, tempData.taskId,
                                                                            tempData.subtaskId, tempData.batchId));
        processedData.emplace_back(globalTaskId, IdPool::GetInstance().GetUint64Id(columnName), tempData.value);
    }
    if (processedData.empty()) {
        ERROR("FormatTaskBasedData data processing error.");
        return false;
    }
    return true;
}

bool PmuProcessor::SampleBasedProcess(const std::string &fileDir)
{
    INFO("SampleBasedProcess, dir is %", fileDir);
    bool flag = true;
    uint64_t stringAiCoreId = IdPool::GetInstance().GetUint64Id(TYPE_AIC);
    uint64_t stringAiVectorCoreId = IdPool::GetInstance().GetUint64Id(TYPE_AIV);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> dbPathTable;
    for (const auto& name : SAMPLE_BASED_DB_NAMES) {
        uint64_t coreType = (name == AI_CORE_PREFIX) ? stringAiCoreId : stringAiVectorCoreId;
        for (const auto& devicePath : deviceList) {
            auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
            std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, name + std::to_string(deviceId) + ".db"});
            auto status = CheckPath(dbPath);
            if (status == CHECK_SUCCESS) {
                dbPathTable.insert({dbPath, std::make_tuple(deviceId, coreType)});
            } else if (status == CHECK_FAILED) {
                flag = false;
                ERROR("Check % failed!", dbPath);
                continue;
            }
        }
    }
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

bool PmuProcessor::SampleBasedTimelineProcess(
    const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable, const std::string &fileDir)
{
    INFO("SampleBasedTimeline");
    bool flag = true;
    ThreadData threadData;
    if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    for (const auto& pair : dbPathTable) {
        // 0 deviceId, 1 coreType
        threadData.deviceId = std::get<0>(pair.second);
        double freq;
        if (!Context::GetInstance().GetPmuFreq(freq, threadData.deviceId, fileDir) ||
                IsDoubleEqual(freq, DOUBLE_ZERO)) {
            ERROR("GetPmuFreq failed or aic freq is invalid, profPath is %.", fileDir);
            return false;
        }
        auto oriData = GetSampleBasedTimelineData(pair.first);
        PSTFormat processedData;
        if (!FormatSampleBasedTimelineData(oriData, processedData, threadData, freq, std::get<1>(pair.second))) {
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
                                                 const ThreadData &threadData, const double freq,
                                                 const uint64_t coreType)
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
        if (IsDoubleEqual(interval, DOUBLE_ZERO)) {
            WARN("The interval is invalid, the record timestamp is %, "
                 "core id is %.", tempData.timestamp, tempData.coreid);
            continue;
        }
        // task_cyc 单位为 次; freq单位为 MHz,即次/秒; interval单位为 ns
        // 由公式 (task_cyc / (freq * 10^6) / (interval / 10^9)) * 100% 化简而来
        double usage = (cycle * MILLI_SECOND) / (freq * interval) * PERCENTAGE;

        Utils::HPFloat timestamp{tempData.timestamp};
        processedData.emplace_back(threadData.deviceId, Utils::GetLocalTime(timestamp, threadData.timeRecord).Uint64(),
                                   cycle, usage, freq, static_cast<uint16_t>(tempData.coreid), coreType);
    }
    if (processedData.empty()) {
        ERROR("FormatSampleBasedSummaryData data processing error.");
        return false;
    }
    return true;
}

bool PmuProcessor::SampleBasedSummaryProcess(
    const std::unordered_map<std::string, std::tuple<uint16_t, uint64_t>> &dbPathTable)
{
    INFO("SampleBasedSummary");
    bool flag = true;
    for (const auto& pair : dbPathTable) {
        auto oriData = GetSampleBasedSummaryData(pair.first);
        PSSFormat processedData;
        // 0 deviceId, 1 coreType
        if (!FormatSampleBasedSummaryData(oriData, processedData, std::get<0>(pair.second), std::get<1>(pair.second))) {
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
                                                const uint16_t deviceId, const uint64_t coreType)
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
                                   tempData.value, static_cast<uint16_t>(tempData.coreid), coreType);
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