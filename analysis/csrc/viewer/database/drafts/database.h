/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_config.h
 * Description        : DB相关常量映射
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_DATABASE_H
#define ANALYSIS_VIEWER_DATABASE_DATABASE_H

#include <vector>
#include <unordered_map>
#include <string>
#include "connection.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {

using TableColumn = Analysis::Viewer::Database::TableColumn;

// DB中Table映射基类
class Database {
public:
    Database() = default;

    // 获取该DB实际落盘的文件名
    std::string GetDBName() const;

    // 获取该DB指定表中字段名
    std::vector<TableColumn> GetTableCols(const std::string &tableName);

protected:
    std::string dbName_;
    std::unordered_map<std::string, std::vector<TableColumn>> tableColNames_;
    const std::string sqlTextType = "TEXT";
    const std::string sqlIntegerType = "INTEGER";
    const std::string sqlNumericType = "NUMERIC";
    const std::string sqlRealType = "REAL";
};

class ApiEventDB : public Database {
public:
    ApiEventDB();
};

class RuntimeDB : public Database {
public:
    RuntimeDB();
};

class GEInfoDB : public Database {
public:
    GEInfoDB();
};

class HashDB : public Database {
public:
    HashDB();
};

class HCCLDB : public Database {
public:
    HCCLDB();
};

class RtsTrackDB : public Database {
public:
    RtsTrackDB();
};
} // namespace Drafts
} // namespace Database
} // namespace Viewer
} // namespace Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DATABASE_H
