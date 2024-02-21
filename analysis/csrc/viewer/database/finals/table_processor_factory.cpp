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
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/viewer/database/finals/string_ids_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_info_processor.h"
#include "analysis/csrc/viewer/database/finals/session_time_info_processor.h"
#include "analysis/csrc/viewer/database/finals/enum_processor.h"
#include "analysis/csrc/viewer/database/finals/task_processor.h"
#include "analysis/csrc/viewer/database/finals/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/viewer/database/finals/api_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_module_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_op_mem_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {

std::shared_ptr<TableProcessor> TableProcessorFactory::CreateTableProcessor(
    const std::string &processorName,
    const std::string &reportDBPath,
    const std::set<std::string> &profPaths)
{
    std::shared_ptr<TableProcessor> processor = nullptr;
    if (processorName == PROCESSOR_NAME_STRING_IDS) {
        MAKE_SHARED_RETURN_VALUE(processor, StringIdsProcessor, nullptr, reportDBPath);
    }
    if (processorName == PROCESSOR_NAME_SESSION_TIME_INFO) {
        MAKE_SHARED_RETURN_VALUE(processor, NpuInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_NPU_INFO) {
        MAKE_SHARED_RETURN_VALUE(processor, SessionTimeInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_ENUM) {
        MAKE_SHARED_RETURN_VALUE(processor, EnumProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_TASK) {
        MAKE_SHARED_RETURN_VALUE(processor, TaskProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_COMPUTE_TASK_INFO) {
        MAKE_SHARED_RETURN_VALUE(processor, ComputeTaskInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_COMMUNICATION) {
        MAKE_SHARED_RETURN_VALUE(processor, CommunicationInfoProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_API) {
        MAKE_SHARED_RETURN_VALUE(processor, ApiProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_NPU_MEM) {
        MAKE_SHARED_RETURN_VALUE(processor, NpuMemProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_NPU_MODULE_MEM) {
        MAKE_SHARED_RETURN_VALUE(processor, NpuModuleMemProcessor, nullptr, reportDBPath, profPaths);
    }
    if (processorName == PROCESSOR_NAME_NPU_OP_MEM) {
        MAKE_SHARED_RETURN_VALUE(processor, NpuOpMemProcessor, nullptr, reportDBPath, profPaths);
    }
    return processor;
}


} // Database
} // Viewer
} // Analysis