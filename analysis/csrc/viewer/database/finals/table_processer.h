/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processer.h
 * Description        : db工厂类
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H
#define ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H

#include <string>
#include <vector>
#include <set>

#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/viewer/database/finals/report_db.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该结构体用于区分原始db和report db 所需的对象和属性
// 规定了 db名字， table名字，和对应的database和dbRunner对象
struct DBInfo {
    std::string dbName;
    std::string tableName;
    std::shared_ptr<Database> database;
    std::shared_ptr<DBRunner> dbRunner;
};

// 该类用于定义处理父类
// 主要包括以下特性：
// 1. 设计各db处理流程
class TableProcesser {
public:
    TableProcesser() = default;
    TableProcesser(std::string reportDBPath, const std::set<std::string> &profPaths);
    virtual bool Run();
    virtual ~TableProcesser() = default;
protected:
    virtual bool Process(const std::string &fileDir) = 0;
    template<typename... Args>
    bool SaveData(const std::vector<std::tuple<Args...>> &data) const;
    std::string reportDBPath_;
    std::set<std::string> profPaths_;
    DBInfo reportDB_;
}; // class TableProcesser

template<typename... Args>
bool TableProcesser::SaveData(const std::vector<std::tuple<Args...>> &data) const
{
    if (data.empty()) {
        ERROR("% is empty.", reportDB_.tableName);
        return false;
    }
    if (reportDB_.database == nullptr) {
        ERROR("Report db database is nullptr.");
        return false;
    }
    if (reportDB_.dbRunner == nullptr) {
        ERROR("Report db runner is nullptr.");
        return false;
    }
    if (!reportDB_.dbRunner->CreateTable(reportDB_.tableName, reportDB_.database->GetTableCols(reportDB_.tableName))) {
        ERROR("Create table: % failed", reportDB_.tableName);
        return false;
    }
    if (!reportDB_.dbRunner->InsertData(reportDB_.tableName, data)) {
        ERROR("Insert data into % failed", reportDBPath_);
        return false;
    }
    return true;
}

}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H
