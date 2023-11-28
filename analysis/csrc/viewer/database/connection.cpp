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
#include <sqlite3.h>
#include <iostream>
#include "connection.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis;

Connection::Connection(const std::string &path)
{
    int rc = sqlite3_open(path.c_str(), &db_); // 连接数据库
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to open database: " + std::string(sqlite3_errmsg(db_));
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
    }
    sqlite3_exec(db_, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);
}

Connection::~Connection()
{
    if (db_) {
        sqlite3_close(db_);
    }
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

bool Connection::ExecuteCreateTable(const std::string &sql)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to create table: " + std::string(errMsg);
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::ExecuteDropTable(const std::string &sql)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to drop table: " + std::string(errMsg);
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::ExecuteDelete(const std::string &sql)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to delete: " + std::string(errMsg);
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::ExecuteUpdate(const std::string &sql)
{
    CHAR_PTR errMsg = nullptr;
    sqlite3_busy_timeout(db_, TIMEOUT);
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorMsg = "Failed to update table: " + std::string(errMsg);
        ERROR("sqlite3_exec return %, %", rc, errorMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Connection::InsertCmd(const std::string &tableName, const int &colNum)
{
    std::string valueStr;
    for (int i = 0; i < colNum - 1; ++i) {
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
    value = sqlite3_column_int64(stmt_, ++index_);
}

void Connection::GetColumn(int32_t &value)
{
    value = sqlite3_column_int(stmt_, ++index_);
}

void Connection::GetColumn(uint32_t &value)
{
    value = sqlite3_column_int64(stmt_, ++index_);
}

void Connection::GetColumn(double &value)
{
    value = sqlite3_column_double(stmt_, ++index_);
}

void Connection::GetColumn(std::string &value)
{
    auto valueStr = const_cast<unsigned char*>(sqlite3_column_text(stmt_, ++index_));
    value = std::string(reinterpret_cast<CHAR_PTR>(valueStr));
}
} // Database
} // Viewer
} // Analysis
