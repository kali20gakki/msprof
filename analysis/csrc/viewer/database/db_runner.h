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
#include "connection.h"

namespace Analysis {
namespace Viewer {
namespace Database {

// 该类用于定义数据库模块的对外接口
// 主要包括以下特性：
// 1. 提供统一的增删改查接口
// 2. 屏蔽db选型
class DBRunner {
public:
    explicit DBRunner(const std::string &dbPath);

    int CreateTable(const std::string &tableName, const std::vector<TableColumn> &cols);

    int DropTable(const std::string &tableName);

    // 数据插入接口，支持不同类型数据的插入
    template<typename... Args>
    int InsertData(const std::string &table, const std::vector<std::tuple<Args...>> &data)
    {}

private:
    std::shared_ptr<Connection> conn_ = nullptr;
};
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_DB_RUNNER_H
