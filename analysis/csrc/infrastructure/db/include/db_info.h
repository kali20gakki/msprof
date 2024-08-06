/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_info.h
 * Description        : DB处理封装类
 * Author             : msprof team
 * Creation Date      : 2024/7/19
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H
#define ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H

#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Infra {
using namespace Analysis::Viewer::Database;
// 该结构体用于区分原始db和msprof db 所需的对象和属性
// 规定了 db名字， table名字，和对应的database和dbRunner对象
struct DBInfo {
    std::string dbName;
    std::string tableName;
    std::shared_ptr<DBRunner> dbRunner;
    DBInfo() = default;
    DBInfo(std::string dbName, std::string tableName) : dbName(dbName), tableName(tableName) {};

    bool ConstructDBRunner(std::string& dbPath)
    {
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        return true;
    }
};
}
}

#endif // ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H
