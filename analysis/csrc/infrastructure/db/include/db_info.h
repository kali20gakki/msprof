/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H
#define ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H

#include <utility>

#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Infra {
// 该结构体用于区分原始db和msprof db 所需的对象和属性
// 规定了 db名字， table名字，和对应的database和dbRunner对象
struct DBInfo {
    std::string dbName;
    std::string tableName;
    std::shared_ptr<Database> database;
    std::shared_ptr<DBRunner> dbRunner;
    DBInfo() = default;
    DBInfo(std::string dbName, std::string tableName) : dbName(std::move(dbName)), tableName(std::move(tableName)) {};

    bool ConstructDBRunner(std::string& dbPath)
    {
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        return true;
    }
};
}
}

#endif // ANALYSIS_INFRASTRUCTURE_DB_DB_INFO_H
