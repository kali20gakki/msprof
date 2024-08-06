/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_init_processor.cpp
 * Description        : 从host目录下初始化Hash数据
 * Author             : msprof team
 * Creation Date      : 2024/7/30
 * *****************************************************************************
 */

#include "hash_init_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
HashInitProcessor::HashInitProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool HashInitProcessor::Process(DataInventory &dataInventory)
{
    INFO("Init Hash data, dir is %", profPath_);
    DBInfo hashDB("ge_hash.db", "GeHashInfo");
    GeHashFormat hashData;
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, hashDB.dbName});
    if (!hashDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    GeHashMap hashMap;
    std::shared_ptr<GeHashMap> res;
    // 并不是所有场景都有ge hash数据
    auto flag = CheckPathAndTable(dbPath, hashDB);
    if (flag != CHECK_SUCCESS) {
        MAKE_SHARED_RETURN_VALUE(res, GeHashMap, false, std::move(hashMap));
        dataInventory.Inject(res);
        return flag != CHECK_FAILED;
    }
    std::string sql = "SELECT hash_key, hash_value FROM " + hashDB.tableName;
    if (!hashDB.dbRunner->QueryData(sql, hashData)) {
        ERROR("Query hash data failed, db path is %.", dbPath);
        return false;
    }
    for (auto item : hashData) {
        hashMap[std::get<0>(item)] = std::get<1>(item);
    }
    MAKE_SHARED_RETURN_VALUE(res, GeHashMap, false, std::move(hashMap));
    dataInventory.Inject(res);
    return true;
}
}
}
