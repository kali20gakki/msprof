/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor_factory.cpp
 * Description        : processor的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/table_processor_factory.h"

#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/viewer/database/finals/string_ids_processor.h"
#include "analysis/csrc/viewer/database/finals/target_info_npu_processor.h"
#include "analysis/csrc/viewer/database/finals/target_info_session_time_processor.h"
#include "analysis/csrc/viewer/database/finals/enum_api_level_processor.h"
#include "analysis/csrc/viewer/database/finals/task_processor.h"
#include "analysis/csrc/viewer/database/finals/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/viewer/database/finals/api_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {

std::shared_ptr<TableProcessor> TableProcessorFactory::CreateTableProcessor(
    const std::string &tableName,
    const std::string &reportDBPath,
    const std::set<std::string> &profPaths)
{
    std::shared_ptr<TableProcessor> processor = nullptr;
    if (tableName == TABLE_NAME_STRING_IDS) {
        MAKE_SHARED_RETURN_VALUE(processor, StringIdsProcessor, nullptr, reportDBPath);
    }
    if (tableName == TABLE_NAME_TARGET_INFO_SESSION_TIME) {
        MAKE_SHARED_RETURN_VALUE(processor, TargetInfoNpuProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_TARGET_INFO_NPU) {
        MAKE_SHARED_RETURN_VALUE(processor, TargetInfoSessionTimeProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_ENUM_API_LEVEL) {
        MAKE_SHARED_RETURN_VALUE(processor, EnumApiLevelProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_TASK) {
        MAKE_SHARED_RETURN_VALUE(processor, TaskProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_COMPUTE_TASK_INFO) {
        MAKE_SHARED_RETURN_VALUE(processor, ComputeTaskInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_COMMUNICATION_TASK_INFO) {
        MAKE_SHARED_RETURN_VALUE(processor, CommunicationInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (tableName == TABLE_NAME_API) {
        MAKE_SHARED_RETURN_VALUE(processor, ApiProcessor, nullptr, reportDBPath, profPaths);
    }
    return processor;
}


} // Database
} // Viewer
} // Analysis