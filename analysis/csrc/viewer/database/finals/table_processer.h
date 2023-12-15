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

#include "db_runner.h"
#include "report_db.h"
#include "utils.h"

#include <string>
#include <vector>

namespace Analysis {
namespace Viewer {
namespace Database {
const std::string HOST = "host";
const std::string DEVICE_PREFIX = "device_";
const std::string SQLITE = "sqlite";

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
    TableProcesser(const std::string &reportDBPath, const std::vector<std::string> &profPaths)
        : reportDBPath_(reportDBPath), profPaths_(profPaths)
    {
        reportDB_.database = Utils::MakeShared<ReportDB>();
        reportDB_.dbRunner = Utils::MakeShared<DBRunner>(reportDBPath_);
    };
    bool Run();
    virtual ~TableProcesser() = default;
protected:
    virtual void Process(const std::string &fileDir) = 0;
    std::string reportDBPath_;
    std::vector<std::string> profPaths_;
    DBInfo reportDB_;
}; // class TableProcesser


class TableProcesserFactory {
public:
    static std::shared_ptr<TableProcesser> CreateTableProcessor(const std::string &dbName,
                                                                const std::string &outputDir,
                                                                const std::vector<std::string> &profPaths);
    ~TableProcesserFactory() {};
};  // class TableProcesserFactory


}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H
