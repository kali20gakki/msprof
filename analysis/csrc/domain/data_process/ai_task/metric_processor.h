// Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
/* ******************************************************************************
 * File Name          : metric_processor.h
 * Description        : metric_processor处理metric pmu相关数据
 * Author             : msprof team
 * Creation Date      : 2024/11/9
 * *****************************************************************************
 */

#ifndef MSPROF_METRIC_PROCESSOR_H
#define MSPROF_METRIC_PROCESSOR_H

#include <utility>
#include <map>
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"


namespace Analysis {
namespace Domain {

class MetricProcessor : public DataProcessor {
public:
    MetricProcessor() = default;
    explicit MetricProcessor(const std::string &profPath);
private:
    bool Process(DataInventory &dataInventory) override;
    bool TaskBasedProcess(DataInventory& dataInventory);
    std::vector<std::string> GetAndCheckTableColumns(const std::unordered_map<std::string, uint16_t>& dbPathTable,
                                         DBInfo &metricDB);

    bool TaskBasedProcessByDevice(const std::pair<const std::string, uint16_t>& dbRecord,
                                  const std::vector<std::string> &headers, DBInfo &metricDB,
                                  std::map<TaskId, std::vector<std::vector<std::string>>> &dataInventory);

    std::vector<std::vector<std::string>> GetTaskBasedData(const std::string &dbPath,
                                                           const std::vector<std::string> &headers, DBInfo &metricDB);

    bool FormatTaskBasedData(const std::vector<std::vector<std::string>> &oriData,
                             std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                             const uint16_t &deviceId);

    std::vector<TableColumn> RemoveNeedlessColumns(std::vector<TableColumn> &tableColumns);

    bool AddMemoryBound(std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                        std::vector<std::string> &headers);

    bool CheckAndGetMemoryBoundRelatedDataIndex(const std::vector<std::string> &headers,
                                                std::vector<int> &neededDataIndex);

    bool ProcessData(const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID, DBInfo &metricDB,
                     std::vector<std::string> &headers, DataInventory &dataInventory);

    std::vector<std::string> ModifySummaryHeaders(const std::vector<std::string> &oriHeaders);

    void InsertLineDataToOriData(std::vector<std::vector<std::string>> &oriData,
                                 std::vector<std::tuple<std::string>> &lineData);
};

} // Domain
} // Analysis
#endif // MSPROF_METRIC_PROCESSOR_H