/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_runner.cpp
 * Description        : 数据库模块对外接口
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */
#include <string>
#include <algorithm>
#include "utils.h"
#include "db_runner.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis;

static std::string GetColumnsString(const std::vector <TableColumn> &cols)
{
    std::vector <std::string> result(cols.size());
    std::transform(cols.begin(), cols.end(), result.begin(), [](const TableColumn &col) {
        return col.ToString();
    });
    return Join(result, ",");
}

bool DBRunner::CreateTable(const std::string &tableName, const std::vector <TableColumn> &cols) const
{
    INFO("Start create %", tableName);
    std::string valuesStr = GetColumnsString(cols);
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (" + valuesStr + ");";
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteCreateTable(sql)) {
        ERROR("Create % failed", tableName);
        return false;
    }
    INFO("create % success", tableName);
    return true;
}

bool DBRunner::DropTable(const std::string &tableName) const
{
    INFO("Start drop %", tableName);
    std::string sql = "DROP TABLE " + tableName + ";";
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteDropTable(sql)) {
        ERROR("Drop % failed", tableName);
        return false;
    }
    INFO("Drop % success", tableName);
    return true;
}

bool DBRunner::DeleteData(const std::string &sql) const
{
    INFO("Start delete data");
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteDelete(sql)) {
        ERROR("Delete data failed: %", sql);
        return false;
    }
    INFO("Delete data success");
    return true;
}

bool DBRunner::UpdateData(const std::string &sql) const
{
    INFO("Start update data");
    auto conn = std::make_shared<Connection>(path_);
    if (!conn) {
        ERROR("Connection init failed");
        return false;
    }
    if (!conn->ExecuteUpdate(sql)) {
        ERROR("Update data failed: %", sql);
        return false;
    }
    INFO("Update data success");
    return true;
}
}
}
}
