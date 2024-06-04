/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric_summary_persistence.h
 * Description        : PMU数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/5/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/services/persistence/persistence_utils.h"
#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Viewer::Database;
class MetricSummaryPersistence : public Process {
private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
    TableColumns GetTableColumn(const DeviceContext& context);
    uint32_t GenerateAndSavePmuData(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, sqlite3_stmt *stmt,
                                    sqlite3* db);
    uint32_t SaveDataToDb(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, std::string& dbPath, DBInfo& dbInfo);
    bool BindParametersAndExecuteInsert(std::vector<uint64_t>& ids, std::vector<double>& pmu,
                                        sqlite3_stmt *stmt, sqlite3* db);
    bool ConstructData(std::vector<uint64_t>& ids, std::vector<double>& result, DeviceTask& task, int aicLength,
                       int aivLength);
private:
    std::unique_ptr<MetricCalculator> aicCalculator_;
    std::unique_ptr<MetricCalculator> aivCalculator_;
    int aicLength_ = 0;
    int aivLength_ = 0;
};

class MetricSummaryDB : public Database {
public:
    MetricSummaryDB(TableColumns columns);
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_METRIC_SUMMARY_PERSISTENCE_H
