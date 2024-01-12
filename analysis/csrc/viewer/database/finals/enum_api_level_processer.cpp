/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_api_level_processer.cpp
 * Description        : 处理api相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */
#include "enum_api_level_processer.h"

#include "unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {

EnumApiLevelProcesser::EnumApiLevelProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcesser(reportDBPath, profPaths)
{
    reportDB_.tableName = TABLE_NAME_ENUM_API_LEVEL;
};

bool EnumApiLevelProcesser::Run()
{
    return Process();
}

bool EnumApiLevelProcesser::Process(const std::string &fileDir)
{
    EnumApiLevelDataFormat enumApiData = GetData();
    return SaveData(enumApiData);
}

EnumApiLevelProcesser::EnumApiLevelDataFormat EnumApiLevelProcesser::GetData()
{
    EnumApiLevelDataFormat apiLevelData;
    if (!Utils::Reserve(apiLevelData, API_LEVEL_TABLE.size())) {
        ERROR("Reserve for api level data failed.");
        return apiLevelData;
    }
    for (const auto& record : API_LEVEL_TABLE) {
        apiLevelData.emplace_back(record.second, record.first);
    }
    return apiLevelData;
}

} // Database
} // Viewer
} // Analysis