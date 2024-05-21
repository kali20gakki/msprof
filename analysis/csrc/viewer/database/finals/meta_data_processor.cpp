/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : meta_data_processor.cpp
 * Description        : 元信息表
 * Author             : msprof team
 * Creation Date      : 2024/5/14
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/meta_data_processor.h"

#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace {

const std::string SCHEMA_VERSION_MAJOR = "1";
const std::string SCHEMA_VERSION_MINOR = "0";
const std::string SCHEMA_VERSION_MICRO = "0";
const std::string SCHEMA_VERSION = Utils::Join(".", SCHEMA_VERSION_MAJOR, SCHEMA_VERSION_MINOR, SCHEMA_VERSION_MICRO);

// 版本号相关 SCHEMA_VERSION
// SCHEMA_VERSION_MAJOR，对于数据库的整体格式，只有在整体db格式发生重大重写或重构时，进行更改
// SCHEMA_VERSION_MINOR，表示存在非兼容性修改，需要做适配。例如，当表名或字段类型发生变动时。
// SCHEMA_VERSION_MICRO，表示版本变动，每次变动代表支持兼容性新特性，对原有功能不存在修改，不存在影响和中断原有脚本和数据读取

const std::unordered_map<std::string, std::string> META_DATA = {
    {NAME_STR(SCHEMA_VERSION), SCHEMA_VERSION},
    {NAME_STR(SCHEMA_VERSION_MAJOR), SCHEMA_VERSION_MAJOR},
    {NAME_STR(SCHEMA_VERSION_MINOR), SCHEMA_VERSION_MINOR},
    {NAME_STR(SCHEMA_VERSION_MICRO), SCHEMA_VERSION_MICRO}
};
}

MetaDataProcessor::MetaDataProcessor(const std::string &msprofDBPath)
    : TableProcessor(msprofDBPath) {}

bool MetaDataProcessor::Run()
{
    INFO("MetaDataProcessor Run.");
    bool flag = Process();
    PrintProcessorResult(flag, PROCESSOR_NAME_META_DATA);
    return flag;
}

bool MetaDataProcessor::Process(const std::string &fileDir)
{
    INFO("MetaDataProcessor Process.");
    return SaveMetaData();
}

bool MetaDataProcessor::SaveMetaData()
{
    INFO("MetaDataProcessor SaveMetaData.");
    DataFormat metaData;
    if (!Utils::Reserve(metaData, META_DATA.size())) {
        ERROR("Reserve for meta data failed.");
        return false;
    }
    for (const auto& record : META_DATA) {
        metaData.emplace_back(record.first, record.second);
    }
    return SaveData(metaData, TABLE_NAME_META_DATA);
}

} // Database
} // Viewer
} // Analysis