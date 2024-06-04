/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : persistence_utils.h
 * Description        : 保存数据通用操作
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H

#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
using namespace Utils;
struct DBInfo {
    std::string dbName;
    std::string tableName;
    std::shared_ptr<Database> database;
    std::shared_ptr<DBRunner> dbRunner;
    DBInfo() = default;
    DBInfo(std::string dbName, std::string tableName) : dbName(dbName), tableName(tableName) {};
    virtual ~DBInfo() = default;
};

template<typename... Args>
bool SaveData(const std::vector<std::tuple<Args...>> &data, DBInfo& dbInfo, std::string& dbPath)
{
    INFO("Processor Save % Data.", dbInfo.tableName);
    if (data.empty()) {
        ERROR("% is empty.", dbInfo.tableName);
        return false;
    }
    if (dbInfo.database == nullptr) {
        ERROR("Msprof db database is nullptr.");
        return false;
    }
    if (dbInfo.dbRunner == nullptr) {
        ERROR("Msprof db runner is nullptr.");
        return false;
    }
    if (!dbInfo.dbRunner->CreateTable(dbInfo.tableName, dbInfo.database->GetTableCols(dbInfo.tableName))) {
        ERROR("Create table: % failed", dbInfo.tableName);
        return false;
    }
    if (!dbInfo.dbRunner->InsertData(dbInfo.tableName, data)) {
        ERROR("Insert data into % failed", dbPath);
        return false;
    }
    return true;
}

SyscntConversionParams GenerateSyscntConversionParams(const DeviceContext& context);
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_PERSISTENCE_UTILS_H
