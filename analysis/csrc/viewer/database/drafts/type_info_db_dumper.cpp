/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
* File Name          : type_info_dumper.cpp
* Description        : typeInfo数据入库接口
* Author             : msprof team
* Creation Date      : 2023/11/16
* *****************************************************************************
*/

#include "analysis/csrc/viewer/database/drafts/type_info_db_dumper.h"

#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {

TypeInfoDBDumper::TypeInfoDBDumper(const std::string &hostFilePath) : BaseDumper<TypeInfoDBDumper>(hostFilePath,
                                                                                                   "TypeHashInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, HashDB);
}

std::vector<std::tuple<std::string, std::string, std::string>> TypeInfoDBDumper::GenerateData(
    const TypeInfoData &typeInfoData)
{
    std::vector<std::tuple<std::string, std::string, std::string>> data;
    if (!Utils::Reserve(data, typeInfoData.size())) {
        ERROR("Can't reserve vector");
        return data;
    }
    for (const auto &devices: typeInfoData) {
        for (const auto &hashPair: devices.second) {
            data.emplace_back(std::to_string(hashPair.first), hashPair.second,
                              NumberMapping::Get(NumberMapping::MappingType::LEVEL, devices.first));
        }
    }
    return data;
}
}
} // Database
} // Viewer
} // Analysis