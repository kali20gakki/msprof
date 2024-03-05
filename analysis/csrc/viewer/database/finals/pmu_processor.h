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

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

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
// Original Task Format: 只取id + 对应字段的value
// stream_id, task_id, subtask_id, batch_id, value
using OTFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, double>>;

// Processed Sample Timeline Format
// deviceId, timestamp, totalCycle, usage, freq, coreId
using PSTFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint16_t>>;
// Processed Sample Summary Format
// deviceId, metric, value, coreId
using PSSFormat = std::vector<std::tuple<uint16_t, uint64_t, double, uint16_t>>;
// Processed Task Format
// globalTaskId, name, value
using PTFormat = std::vector<std::tuple<uint64_t, uint64_t, double>>;

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
    bool TaskBasedProcess(const std::string &fileDir);
    TableColumns GetAndCheckTableColumns(std::unordered_map<std::string, uint16_t> dbPathTable, DBInfo &metricDB);
    bool TaskBasedProcessByColumnName(const std::pair<const std::string, uint16_t> dbRecord,
                                      const std::string &columnName, DBInfo &metricDB);
    OTFormat GetTaskBasedData(const std::string &dbPath, const std::string &columnName, DBInfo &metricDB);
    bool FormatTaskBasedData(const OTFormat &oriData, PTFormat &processedData, const std::string &columnName,
                             const uint16_t &deviceId);

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


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_PMU_PROCESSOR_H
