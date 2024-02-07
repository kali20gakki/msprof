/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_api_level_processor.cpp
 * Description        : 处理api相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */
#include "enum_processor.h"

#include "unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {

EnumProcessor::EnumProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool EnumProcessor::Run()
{
    INFO("EnumProcessor Run.");
    bool flag = Process();
    PrintProcessorResult(flag, TABLE_NAME_ENUM);
    return flag;
}

bool EnumProcessor::Process(const std::string &fileDir)
{
    INFO("EnumProcessor Process.");
    bool flag = GetApiLevelData();
    flag &= GetIOTypeData();
    flag &= GetNpuModuleData();
    return flag;
}

bool EnumProcessor::GetApiLevelData()
{
    INFO("EnumProcessor GetApiLevelData.");
    EnumFormat apiLevelData;
    if (!Utils::Reserve(apiLevelData, API_LEVEL_TABLE.size())) {
        ERROR("Reserve for api level data failed.");
        return false;
    }
    for (const auto& record : API_LEVEL_TABLE) {
        apiLevelData.emplace_back(record.second, record.first);
    }
    return SaveData(apiLevelData, TABLE_NAME_ENUM_API_LEVEL);
}

bool EnumProcessor::GetIOTypeData()
{
    INFO("EnumProcessor GetIOTypeData.");
    EnumFormat ioTypeData;
    if (!Utils::Reserve(ioTypeData, IO_TYPE_TABLE.size())) {
        ERROR("Reserve for IO type data failed.");
        return false;
    }
    for (const auto& record : IO_TYPE_TABLE) {
        ioTypeData.emplace_back(record.second, record.first);
    }
    return SaveData(ioTypeData, TABLE_NAME_ENUM_IO_TYPE);
}

bool EnumProcessor::GetNpuModuleData()
{
    INFO("EnumProcessor GetNpuModuleData.");
    EnumFormat npuModuleData;
    if (!Utils::Reserve(npuModuleData, NPU_MODULE_NAME_TABLE.size())) {
        ERROR("Reserve for NPU module data failed.");
        return false;
    }
    for (const auto& record : NPU_MODULE_NAME_TABLE) {
        npuModuleData.emplace_back(record.second, record.first);
    }
    return SaveData(npuModuleData, TABLE_NAME_ENUM_NPU_MODULE);
}


} // Database
} // Viewer
} // Analysis