/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric_summary_persistence.cpp
 * Description        : PMU数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/5/28
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/metric_summary_persistence.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/association/include/pmu_association.h"

namespace Analysis {
namespace Domain {
namespace {
const int CHIP4_DEFAULT_COLUMN_NUM = 6;
const int OTHER_CHIP_DEFAULT_COLUMN_NUM = 5;
const int CHIP4_PMU_TIME_NUM = 2;
const int OTHER_CHIP_PMU_TIME_NUM = 1;
const std::string AIC_PREFIX = "aic_";
const std::string AIV_PREFIX = "aiv_";
}

bool MetricSummaryPersistence::BindParametersAndExecuteInsert(std::vector<uint64_t>& ids, std::vector<double>& pmu,
                                                              sqlite3_stmt *stmt, sqlite3* db)
{
    int index = 0;
    for (auto& value : ids) {
        sqlite3_bind_int64(stmt, ++index, value);
    }
    for (auto& value : pmu) {
        sqlite3_bind_double(stmt, ++index, value);
    }
    auto rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        // 插入失败，回滚事务
        if (sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr)) {
            ERROR("sqlite3_exec return %: rollback failed!", rc);
        }
        std::string errorMsg = "Failed to insert data: " + std::string(sqlite3_errmsg(db));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        return false;
    }
    return true;
}

bool MetricSummaryPersistence::ConstructData(std::vector<uint64_t>& ids, std::vector<double>& result, DeviceTask& task,
                                             int aicLength, int aivLength)
{
    std::vector<double> aicPmuResult(aicLength, 0.0);
    std::vector<double> aivPmuResult(aivLength, 0.0);
    if (task.acceleratorType == MIX_AIV || task.acceleratorType == MIX_AIC) {
        auto pmu = dynamic_cast<PmuInfoMixAccelerator*>(task.pmuInfo.get());
        ids.push_back(pmu->aicTotalCycles);
        ids.push_back(pmu->aivTotalCycles);
        result.push_back(pmu->aiCoreTime);
        result.push_back(pmu->aivTime);
        result.insert(result.end(), pmu->aicPmuResult.begin(), pmu->aicPmuResult.end());
        result.insert(result.end(), pmu->aivPmuResult.begin(), pmu->aivPmuResult.end());
    } else if (task.acceleratorType == AIV) {
        auto pmu = dynamic_cast<PmuInfoSingleAccelerator*>(task.pmuInfo.get());
        ids.push_back(0);  // AIC置0
        ids.push_back(pmu->totalCycles);
        result.push_back(0.0);
        result.push_back(pmu->totalTime);
        result.insert(result.end(), aicPmuResult.begin(), aicPmuResult.end());
        result.insert(result.end(), pmu->pmuResult.begin(), pmu->pmuResult.end());
    } else if (task.acceleratorType == AIC) {
        auto pmu = dynamic_cast<PmuInfoSingleAccelerator*>(task.pmuInfo.get());
        ids.push_back(pmu->totalCycles);
        ids.push_back(0);  // AIV置0
        result.push_back(pmu->totalTime);
        result.push_back(0.0);
        result.insert(result.end(), pmu->pmuResult.begin(), pmu->pmuResult.end());
        result.insert(result.end(), aivPmuResult.begin(), aivPmuResult.end());
    } else {
        return false;
    }
    return true;
}

MetricSummaryDB::MetricSummaryDB(TableColumns columns)
{
    dbName_ = "metric_summary.db";
    tableColNames_["MetricSummary"] = columns;
}

TableColumns MetricSummaryPersistence::GetTableColumn(const DeviceContext& context)
{
    TableColumns res;
    if (context.GetChipID() == CHIP_V4_1_0) {
        auto aicHeader = aicCalculator_->GetPmuHeader();
        aicLength_ = static_cast<int>(aicHeader.size());
        auto aivHeader = aivCalculator_->GetPmuHeader();
        aivLength_ = static_cast<int>(aivHeader.size());
        res.emplace_back("task_id", SQL_INTEGER_TYPE);
        res.emplace_back("stream_id", SQL_INTEGER_TYPE);
        res.emplace_back("subtask_id", SQL_INTEGER_TYPE);
        res.emplace_back("batch_id", SQL_INTEGER_TYPE);
        res.emplace_back("aic_total_cycles", SQL_NUMERIC_TYPE);
        res.emplace_back("aiv_total_cycles", SQL_NUMERIC_TYPE);
        res.emplace_back("aic_total_times", SQL_NUMERIC_TYPE);
        res.emplace_back("aiv_total_times", SQL_NUMERIC_TYPE);
        for (auto& str : aicHeader) {
            res.emplace_back((AIC_PREFIX + str), SQL_NUMERIC_TYPE);
        }
        for (auto& str : aivHeader) {
            res.emplace_back((AIV_PREFIX + str), SQL_NUMERIC_TYPE);
        }
    }
    return res;
}

uint32_t MetricSummaryPersistence::GenerateAndSavePmuData(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                                          sqlite3_stmt *stmt, sqlite3 *db)
{
    std::vector<uint64_t> ids;
    ids.reserve(CHIP4_DEFAULT_COLUMN_NUM);
    std::vector<double> result;
    result.reserve(CHIP4_PMU_TIME_NUM + aicLength_ + aivLength_);
    for (auto& it : deviceTask) {
        ids.clear();
        ids.push_back(it.first.taskId);
        ids.push_back(it.first.streamId);
        ids.push_back(it.first.contextId);
        ids.push_back(it.first.batchId);
        for (auto& task : it.second) {
            sqlite3_reset(stmt);
            if (!ConstructData(ids, result, task, aicLength_, aivLength_)) {
                continue;
            }
            if (BindParametersAndExecuteInsert(ids, result, stmt, db)) {
                // 保留4个id:task-stream-context-batch id
                ids.erase(std::next(ids.begin(), 4), std::next(ids.begin(), CHIP4_DEFAULT_COLUMN_NUM));
                result.clear();
            } else {
                ERROR("Transaction is error, rollback");
                return ANALYSIS_ERROR;
            }
        }
    }
    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    return ANALYSIS_OK;
}

uint32_t MetricSummaryPersistence::SaveDataToDb(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                                std::string& dbPath, DBInfo& dbInfo)
{
    std::shared_ptr<Connection> conn;
    MAKE_SHARED_RETURN_VALUE(conn, Connection, false, dbPath);
    auto db = conn->GetSqliteConn();
    auto stmt = conn->GetSqliteStmt();
    if (db == nullptr) {
        ERROR("Get DB fail!");
        return ANALYSIS_ERROR;
    }
    int colNum = static_cast<int>(dbInfo.database->GetTableCols(dbInfo.tableName).size());
    // 开启事务
    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
    std::string valueStr;
    for (int i = 0; i < colNum - 1; ++i) {
        valueStr += "?,";
    }
    valueStr = "(" + valueStr + "?)";
    std::string sql = "INSERT INTO " + dbInfo.tableName + " VALUES" + valueStr;
    sqlite3_busy_timeout(db, TIMEOUT);
    bool rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string errorMsg = std::string(sqlite3_errmsg(db));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return ANALYSIS_ERROR;
    }
    if (GenerateAndSavePmuData(deviceTask, stmt, db) == ANALYSIS_OK) {
        return ANALYSIS_OK;
    } else {
        ERROR("Insert data to % failed", dbInfo.tableName);
        return ANALYSIS_ERROR;
    }
}


uint32_t MetricSummaryPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto deviceTask = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    SampleInfo sampleInfo;
    deviceContext.Getter(sampleInfo);
    aicCalculator_ = MetricCalculatorFactory::GetAicCalculator(sampleInfo.aiCoreMetrics);
    aivCalculator_ = MetricCalculatorFactory::GetAivCalculator(sampleInfo.aivMetrics);
    DBInfo metricSummary("metric_summary.db", "MetricSummary");
    auto columns = GetTableColumn(deviceContext);
    MAKE_SHARED_NO_OPERATION(metricSummary.database, MetricSummaryDB, columns);
    std::string dbPath = Utils::GetDBPath({deviceContext.GetDeviceFilePath(), SQLITE, metricSummary.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(metricSummary.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    if (!metricSummary.dbRunner->CreateTable(metricSummary.tableName,
                                             metricSummary.database->GetTableCols(metricSummary.tableName))) {
        ERROR("Create table: % failed", metricSummary.tableName);
        return ANALYSIS_ERROR;
    }
    if (SaveDataToDb(*deviceTask, dbPath, metricSummary) == ANALYSIS_OK) {
        INFO("Process % done!", metricSummary.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}

REGISTER_PROCESS_SEQUENCE(MetricSummaryPersistence, true, PmuAssociation);
REGISTER_PROCESS_DEPENDENT_DATA(MetricSummaryPersistence, std::map<TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(MetricSummaryPersistence, CHIP_ID_ALL);
}
}
