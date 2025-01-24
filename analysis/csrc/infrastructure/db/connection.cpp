/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : connection.cpp
 * Description        : 数据库连接对象
 * Author             : msprof team
 * Creation Date      : 2023/11/3
 * *****************************************************************************
 */

#include "analysis/csrc/infrastructure/db/include/connection.h"

#include <sqlite3.h>
#include <stdexcept>
#include "analysis/csrc/infrastructure/utils//utils.h"

namespace Analysis {
namespace Infra {
using namespace Analysis::Utils;
using namespace Analysis;

namespace {
const std::string CREATE_TABLE = "create table";
const std::string CREATE_INDEX = "create index";
const std::string DROP_TABLE = "drop table";
const std::string DELETE = "delete";
const std::string UPDATE = "update";
const std::string CHECK = "check";
}

Connection::Connection(const std::string &path)
{
    int rc = sqlite3_open(path.c_str(), &db_); // 连接数据库
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_close_v2(db_);
        db_ = nullptr;
    } else {
        sqlite3_exec(db_, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);
    }
}

Connection::~Connection()
{
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
    if (db_) {
        int rc = sqlite3_close(db_);
        if (rc != SQLITE_OK) {
            ERROR("First close failed, err: % !", sqlite3_errmsg(db_));
            sqlite3_close_v2(db_);
        }
        db_ = nullptr;
    }
}

bool Connection::ExecuteSql(const std::string &sql, const std::string &sqlType)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to " + sqlType +": " + std::string(errMsg);
        if (sqlType == CHECK) {
            WARN("sqlite3_exec return %, %", rc, errorMsg);
        } else {
            ERROR("sqlite3_exec return %, %", rc, errorMsg);
        }
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::CheckTableExists(const std::string &tableName)
{
    std::string checkSql = "SELECT COUNT(1) FROM " + tableName + ";";
    return ExecuteSql(checkSql, CHECK);
}

bool Connection::ExecuteCreateTable(const std::string &sql)
{
    return ExecuteSql(sql, CREATE_TABLE);
}

bool Connection::ExecuteCreateIndex(const std::string &sql)
{
    return ExecuteSql(sql, CREATE_INDEX);
}

bool Connection::ExecuteDropTable(const std::string &sql)
{
    return ExecuteSql(sql, DROP_TABLE);
}

bool Connection::ExecuteDelete(const std::string &sql)
{
    return ExecuteSql(sql, DELETE);
}

bool Connection::ExecuteUpdate(const std::string &sql)
{
    return ExecuteSql(sql, UPDATE);
}

std::vector<TableColumn> Connection::ExecuteGetTableColumns(const std::string &tableName)
{
    std::vector <TableColumn> cols;
    std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_busy_timeout(db_, TIMEOUT);
    bool rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::string errorMsg = std::string(sqlite3_errmsg(db_));
        ERROR("% failed", sql);
        return cols;
    }
    while (sqlite3_step(stmt_) == SQLITE_ROW) {
        std::string name;
        std::string type;
        GetColumn(name);
        GetColumn(type);
        index_ = 0;
        cols.emplace_back(TableColumn(name, type));
    }
    return cols;
}

bool Connection::InsertCmd(const std::string &tableName, const uint32_t &colNum)
{
    std::string valueStr;
    for (uint32_t i = 0; i < colNum - 1; ++i) {
        valueStr += "?,";
    }
    valueStr = "(" + valueStr + "?)";
    std::string sql = "INSERT INTO " + tableName + " VALUES" + valueStr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    bool rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::string errorMsg = std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return false;
    }
    return true;
}

bool Connection::QueryCmd(const std::string &sql)
{
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to bind parameter: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_prepare_v2 run failed: %", errorMsg);
        return false;
    }
    return true;
}

void Connection::BindParameters(int64_t value)
{
    sqlite3_bind_int64(stmt_, ++index_, value);
}

void Connection::BindParameters(uint64_t value)
{
    sqlite3_bind_int64(stmt_, ++index_, value);
}

void Connection::BindParameters(int32_t value)
{
    sqlite3_bind_int(stmt_, ++index_, value);
}

void Connection::BindParameters(uint32_t value)
{
    sqlite3_bind_int64(stmt_, ++index_, value);
}

void Connection::BindParameters(double value)
{
    sqlite3_bind_double(stmt_, ++index_, value);
}

void Connection::BindParameters(std::string value)
{
    sqlite3_bind_text(stmt_, ++index_, value.c_str(), -1, nullptr);
}

void Connection::GetColumn(int64_t &value)
{
    value = sqlite3_column_int64(stmt_, ++index_);
}

void Connection::GetColumn(uint64_t &value)
{
    value = static_cast<uint64_t>(sqlite3_column_int64(stmt_, ++index_));
}

void Connection::GetColumn(int32_t &value)
{
    value = sqlite3_column_int(stmt_, ++index_);
}

void Connection::GetColumn(uint32_t &value)
{
    value = static_cast<uint32_t>(sqlite3_column_int64(stmt_, ++index_));
}

void Connection::GetColumn(double &value)
{
    value = sqlite3_column_double(stmt_, ++index_);
}

void Connection::GetColumn(std::string &value)
{
    auto valueStr = const_cast<unsigned char*>(sqlite3_column_text(stmt_, ++index_));
    value = std::string(ReinterpretConvert<CHAR_PTR>(valueStr));
}

void Connection::GetColumn(uint16_t &value)
{
    value = sqlite3_column_int(stmt_, ++index_);
}
} // Infra
} // Analysis
