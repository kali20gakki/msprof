/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_processor.cpp
 * Description        : 落盘枚举表数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */
#include "enum_processor.h"

#include "unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace {

const std::unordered_map<std::string, std::unordered_map<std::string, uint16_t>> ENUM_TABLE = {
    {TABLE_NAME_ENUM_API_TYPE, API_LEVEL_TABLE},
    {TABLE_NAME_ENUM_MODULE, MODULE_NAME_TABLE}
};
}

EnumProcessor::EnumProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool EnumProcessor::Run()
{
    INFO("EnumProcessor Run.");
    bool flag = Process();
    PrintProcessorResult(flag, PROCESSOR_NAME_ENUM);
    return flag;
}

bool EnumProcessor::Process(const std::string &fileDir)
{
    INFO("EnumProcessor Process.");
    bool flag = true;
    for (const auto& tableName : ENUM_TABLE) {
        flag &= SaveEnumData(tableName.first);
    }
    return flag;
}

bool EnumProcessor::SaveEnumData(const std::string &tableName)
{
    INFO("EnumProcessor: %.", tableName);
    auto table = ENUM_TABLE.find(tableName);
    // Process内保证不出现表外的表名,省去end判定
    EnumDataFormat enumData;
    if (!Utils::Reserve(enumData, table->second.size())) {
        ERROR("Reserve for % data failed.", tableName);
        return false;
    }
    for (const auto& record : table->second) {
        enumData.emplace_back(record.second, record.first);
    }
    return SaveData(enumData, tableName);
}

} // Database
} // Viewer
} // Analysis