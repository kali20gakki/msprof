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
    DBInfo() = default;
    DBInfo(std::string dbName, std::string tableName) : dbName(dbName), tableName(tableName) {};
};

// 该类用于定义处理父类
// 主要包括以下特性：用于规范各db处理流程
// 1、Run拉起processor整体流程
// 2、以PROF文件为粒度，分别拉起Process
// (不强制要求 Run, Process完成上述工作,具体根据子类的实际情况进行实现,比如不感知PROF文件的情况)
// 3、子类内部自行调用Save进行数据dump
class TableProcesser {
public:
    TableProcesser() = default;
    TableProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    TableProcesser(const std::string &reportDBPath);
    virtual bool Run();
    virtual ~TableProcesser() = default;
protected:
    virtual bool Process(const std::string &fileDir) = 0;
    template<typename... Args>
    bool SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName) const;
    static void PrintProcessorResult(bool result, const std::string &processorName);
    std::string reportDBPath_;
    std::set<std::string> profPaths_;
    DBInfo reportDB_;
}; // class TableProcessor

template<typename... Args>
bool TableProcesser::SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName) const
{
    INFO("Processor Save % Data.", tableName);
    if (data.empty()) {
        ERROR("% is empty.", tableName);
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
    if (!reportDB_.dbRunner->CreateTable(tableName, reportDB_.database->GetTableCols(tableName))) {
        ERROR("Create table: % failed", tableName);
        return false;
    }
    if (!reportDB_.dbRunner->InsertData(tableName, data)) {
        ERROR("Insert data into % failed", reportDBPath_);
        return false;
    }
    return true;
}

}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H
