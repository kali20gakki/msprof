/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor.cpp
 * Description        : processor的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */
#include <atomic>

#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace {
using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
const uint32_t POOLNUM = 2;
}

TableProcessor::TableProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : reportDBPath_(reportDBPath), profPaths_(profPaths)
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

TableProcessor::TableProcessor(const std::string &reportDBPath)
    : reportDBPath_(reportDBPath)
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

bool TableProcessor::Run()
{
    std::atomic<bool> retFlag(true);
    Analysis::Utils::ThreadPool pool(POOLNUM);
    pool.Start();
    for (const auto& fileDir : profPaths_) {
        pool.AddTask([this, fileDir, &retFlag]() {
            retFlag = Process(fileDir) & retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    return retFlag;
}

void TableProcessor::PrintProcessorResult(bool result, const std::string &processorName)
{
    if (!result) {
        PRINT_ERROR("% run failed!", processorName);
    }
}

bool TableProcessor::GetGeHashMap(GeHashMap &hashMap, const std::string &fileDir)
{
    INFO("GetGeHashData, dir is %", fileDir);
    DBInfo hashDB("ge_hash.db", "GeHashInfo");
    GeHashFormat hashData;
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, hashDB.dbName});
    // 并不是所有场景都有ge hash数据
    auto flag = CheckPath(dbPath);
    if (flag != CHECK_SUCCESS) {
        return false;
    }
    MAKE_SHARED_RETURN_VALUE(hashDB.dbRunner, DBRunner, false, dbPath);
    if (hashDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    std::string sql = "SELECT hash_key, hash_value FROM " + hashDB.tableName;
    if (!hashDB.dbRunner->QueryData(sql, hashData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return false;
    }
    for (auto item : hashData) {
        hashMap[std::get<0>(item)] = std::get<1>(item);
    }
    return true;
}

uint8_t TableProcessor::CheckPath(const std::string& path)
{
    if (!Utils::File::Exist(path)) {
        WARN("Can't find the db, the path is %.", path);
        return NOT_EXIST;
    }
    if (!Utils::FileReader::Check(path, MAX_DB_BYTES)) {
        ERROR("Check % failed.", path);
        return CHECK_FAILED;
    }
    return CHECK_SUCCESS;
}

} // Database
} // Viewer
} // Analysis