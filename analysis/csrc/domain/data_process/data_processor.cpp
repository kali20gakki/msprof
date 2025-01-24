/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_processor.cpp
 * Description        : 数据处理父类
 * Author             : msprof team
 * Creation Date      : 2024/07/19
 * *****************************************************************************
 */

#include "data_processor.h"
#include <unordered_map>
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Viewer::Database;
namespace {
const uint64_t INVALID_DB_SIZE = 0;
}
DataProcessor::DataProcessor(const std::string &profPath) : profPath_(profPath) {}

bool DataProcessor::Run(DataInventory& dataInventory, const std::string &processorName)
{
    INFO("% Run. Dir is %", processorName, profPath_);
    auto retFlag = Process(dataInventory);
    if (!retFlag) {
        PRINT_ERROR("% run failed!", processorName);
    }
    return retFlag;
}

uint8_t DataProcessor::CheckPathAndTable(const std::string& path, const DBInfo& dbInfo, bool enableStrictCheck)
{
    if (!Utils::File::Exist(path) || Utils::File::Size(path) == INVALID_DB_SIZE) {
        WARN("Can't find the db, the path is %.", path);
        return NOT_EXIST;
    }
    if (!Utils::FileReader::Check(path, MAX_DB_BYTES)) {
        ERROR("Check % failed.", path);
        return CHECK_FAILED;
    }
    if (!dbInfo.dbRunner->CheckTableExists(dbInfo.tableName)) {
        if (enableStrictCheck) {
            ERROR("Check % not exists", dbInfo.tableName);
            return CHECK_FAILED;
        } else {
            WARN("Check % not exists", dbInfo.tableName);
            return NOT_EXIST;
        }
    }
    INFO("Check % and % success.", path, dbInfo.tableName);
    return CHECK_SUCCESS;
}

uint16_t DataProcessor::GetEnumTypeValue(const std::string &key, const std::string &tableName,
                                         const std::unordered_map<std::string, uint16_t> &enumTable)
{
    auto it = enumTable.find(key);
    if (it == enumTable.end()) {
        ERROR("Unknown enum key: %, table is: %.", key, tableName);
        uint16_t res;
        if (Utils::StrToU16(res, key) != ANALYSIS_OK) {
            ERROR("Unknown enum key: %, it will be set %.", key, UINT16_MAX);
            return UINT16_MAX;
        }
        return res;
    }
    return it->second;
}
}
}