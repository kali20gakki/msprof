/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor.h
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

#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/viewer/database/finals/msprof_db.h"
#include "analysis/csrc/infrastructure/db/include/db_info.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Infra;
// 存放各processor中的各线程数据
struct ThreadData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t profId = UINT32_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    uint64_t deviceMonotonic = UINT64_MAX;
    Utils::ProfTimeRecord timeRecord;
};

// GeHash表映射map结构
using GeHashMap = std::unordered_map<std::string, std::string>;
using DBInfo = Analysis::Infra::DBInfo;
const uint8_t CHECK_SUCCESS = 0;
const uint8_t NOT_EXIST = 1;
const uint8_t CHECK_FAILED = 2;
// 该类用于定义处理父类
// 主要包括以下特性：用于规范各db处理流程
// 1、Run拉起processor整体流程
// 2、以PROF文件为粒度，分别拉起Process
// (不强制要求 Run, Process完成上述工作,具体根据子类的实际情况进行实现,比如不感知PROF文件的情况)
// 3、子类内部自行调用Save进行数据dump
class TableProcessor {
public:
    TableProcessor() = default;
    TableProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    TableProcessor(const std::string &msprofDBPath);
    virtual bool Run();
    virtual ~TableProcessor() = default;
protected:
    virtual bool Process(const std::string &fileDir) = 0;
    template<typename... Args>
    bool SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName) const;
    bool CreateTableIndex(const std::string &tableName, const std::string &indexName,
                          const std::vector<std::string> &colNames) const;
    static void PrintProcessorResult(bool result, const std::string &processorName);
    static bool GetGeHashMap(GeHashMap &hashMap, const std::string &fileDir);
    static uint16_t GetEnumTypeValue(const std::string &key, const std::string &tableName,
                                     const std::unordered_map<std::string, uint16_t> &enumTable);
    static uint8_t CheckPath(const std::string& path);
    static uint8_t CheckPathAndTable(const std::string& path, DBInfo dbInfo);
    std::string msprofDBPath_;
    std::set<std::string> profPaths_;
    DBInfo msprofDB_;
}; // class TableProcessor

template<typename... Args>
bool TableProcessor::SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName) const
{
    INFO("Processor Save % Data.", tableName);
    if (data.empty()) {
        ERROR("% is empty.", tableName);
        return false;
    }
    if (msprofDB_.database == nullptr) {
        ERROR("Msprof db database is nullptr.");
        return false;
    }
    if (msprofDB_.dbRunner == nullptr) {
        ERROR("Msprof db runner is nullptr.");
        return false;
    }
    if (!msprofDB_.dbRunner->CreateTable(tableName, msprofDB_.database->GetTableCols(tableName))) {
        ERROR("Create table: % failed", tableName);
        return false;
    }
    if (!msprofDB_.dbRunner->InsertData(tableName, data)) {
        ERROR("Insert data into % failed", msprofDBPath_);
        return false;
    }
    return true;
}

}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_H
