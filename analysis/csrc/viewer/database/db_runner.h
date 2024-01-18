/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_runner.h
 * Description        : 数据库模块对外接口
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H
#define ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H

#include <string>
#include <memory>
#include <vector>

#include "analysis/csrc/viewer/database/connection.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis;
// 该类用于定义数据库模块的对外接口
// 主要包括以下特性：
// 1. 提供统一的增删改查接口
// 2. 屏蔽db选型
class DBRunner {
public:
    explicit DBRunner(const std::string &dbPath): path_(dbPath) {};
    bool CreateTable(const std::string &tableName, const std::vector<TableColumn> &cols) const;
    bool DropTable(const std::string &tableName) const;
    // 数据插入接口，支持不同类型数据的插入
    template<typename... Args>
    bool InsertData(const std::string &tableName, const std::vector<std::tuple<Args...>> &data) const;
    bool DeleteData(const std::string &sql) const;
    template<typename... Args>
    bool QueryData(const std::string &sql, std::vector<std::tuple<Args...>> &result) const;
    bool UpdateData(const std::string &sql) const;
private:
    std::string path_;
};

template<typename... Args>
bool DBRunner::InsertData(const std::string &tableName, const std::vector<std::tuple<Args...>> &data) const
{
    if (tableName.empty()) {
        ERROR("The tableName is empty string");
        return false;
    }
    INFO("Start insert data to %", tableName);
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteInsert(tableName, data)) {
        ERROR("Insert data to % failed", tableName);
        return false;
    }
    INFO("insert data to % success", tableName);
    return true;
}

template<typename... Args>
bool DBRunner::QueryData(const std::string &sql, std::vector<std::tuple<Args...>> &result) const
{
    INFO("Start query data");
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteQuery(sql, result)) {
        ERROR("Query data failed: %", sql);
        return false;
    }
    INFO("Query data success: %", sql);
    return true;
}
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H
