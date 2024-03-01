/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_processor.h
 * Description        : pmu_processor，处理task-based, sample-based pmu相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/24
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_PMU_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_PMU_PROCESSOR_H

#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using TableColumns = std::vector<TableColumn>;
// 该类用于定义处理pmu的相关数据：包括task-based和sample-based两种情况
class PmuProcessor : public TableProcessor {
// Original Sample Timeline Format:aicore/ai_vector_core中的AICoreOriginalData
// timestamp, task_cyc, coreid
using OSTFormat = std::vector<std::tuple<uint64_t, std::string, uint32_t>>;
// Original Sample Summary Format:aicore/ai_vector_core中的MetricSummary
// metric, value, coreid
using OSSFormat = std::vector<std::tuple<std::string, double, uint32_t>>;

// Processed Sample Timeline Format
// deviceId, timestamp, totalCycle, usage, freq, coreId
using PSTFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint16_t>>;
// Processed Sample Summary Format
// deviceId, metric, value, coreId
using PSSFormat = std::vector<std::tuple<uint16_t, uint64_t, double, uint16_t>>;

struct ThreadData {
    uint16_t deviceId = UINT16_MAX;
    double freq = 0.0;
    Utils::ProfTimeRecord timeRecord;
};

public:
    PmuProcessor() = default;
    PmuProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    template<typename... Args>
    bool TaskBasedProcess(const std::string &fileDir);
    TableColumns GetAndCheckTableColumns(std::unordered_map<std::string, uint16_t> dbPathTable);
    template<typename... Args>
    std::vector<std::tuple<Args...>> GetTaskBasedData(const std::string &dbPath);
    bool CreateMetricSummaryTable(const TableColumns &tableColumns);

    bool SampleBasedProcess(const std::string &fileDir);
    bool SampleBasedTimelineProcess(const std::unordered_map<std::string, uint16_t> &dbPathTable,
                                    const std::string &fileDir);
    OSTFormat GetSampleBasedTimelineData(const std::string &dbPath);
    bool FormatSampleBasedTimelineData(const OSTFormat &oriData, PSTFormat &processedData,
                                       const ThreadData &threadData);

    bool SampleBasedSummaryProcess(const std::unordered_map<std::string, uint16_t> &dbPathTable);
    OSSFormat GetSampleBasedSummaryData(const std::string &dbPath);
    bool FormatSampleBasedSummaryData(const OSSFormat &oriData, PSSFormat &processedData, const uint16_t deviceId);
};

template<typename... Args>
std::vector<std::tuple<Args...>> PmuProcessor::GetTaskBasedData(const std::string &dbPath)
{
    INFO("Start GetTaskBasedData.");
    std::vector<std::tuple<Args...>> result;
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, result, dbPath);
    if (dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return result;
    }
    std::string sql = "SELECT * FROM MetricSummary;";
    result = dbRunner->QueryData(sql);
    return result;
}


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_PMU_PROCESSOR_H
