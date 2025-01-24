/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_db_dumper.cpp
 * Description        : hashData入库类实现
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"

namespace Analysis {
namespace Domain {
HashDBDumper::HashDBDumper(const std::string &hostFilePath) : BaseDumper<HashDBDumper>(hostFilePath, "GeHashInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, HashDB);
}

std::vector<std::tuple<std::string, std::string>> HashDBDumper::GenerateData(const HashDataMap &hashData)
{
    std::vector<std::tuple<std::string, std::string>> data;
    if (!Utils::Reserve(data, hashData.size())) {
        ERROR("Can't reserve vector");
        return data;
    }
    for (auto &pair: hashData) {
        data.emplace_back(std::to_string(pair.first), pair.second);
    }
    return data;
}
} // Domain
} // Analysis