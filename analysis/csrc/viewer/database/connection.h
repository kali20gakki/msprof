/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : connection.h
 * Description        : 数据库连接对象
 * Author             : msprof team
 * Creation Date      : 2023/11/3
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_CONNECTION_H
#define ANALYSIS_VIEWER_DATABASE_CONNECTION_H

#include <sqlite3.h>
#include <vector>
#include <string>

namespace Analysis {
namespace Viewer {
namespace Database {

const std::string SQL_TEXT_TYPE = "TEXT";
const std::string SQL_INTEGER_TYPE = "INTEGER";
const std::string SQL_NUMERIC_TYPE = "NUMERIC";

// 表列
struct TableColumn {
    std::string name;
    std::string type;
    bool isPrimary = false;
    TableColumn(std::string &name, std::string &type) noexcept: name(name), type(type)
    {}
    std::string ToString() const
    {
        std::string base = name + " " + type;
        auto str = isPrimary ? base + " PRIMARY KEY" : base;
        return str;
    }
};

// 数据库连接对象，当前版本以sqlite3实现
class Connection {
public:
    explicit Connection(const std::string &path);

    ~Connection();

    int CreateTable(const std::string &tableName, std::vector<TableColumn> &cols);

    int DropTable(const std::string &tableName);

    // 数据插入接口，支持不同类型数据的插入
    template<typename... Args>
    int InsertData(const std::string &table, const std::vector<std::tuple<Args...>> &data);

private:
    sqlite3 *db_ = nullptr;
};
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_CONNECTION_H
