/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_manager.h
 * Description        : summary_manager，summary导出调度
 * Author             : msprof team
 * Creation Date      : 2025/5/23
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/summary_manager.h"
#include <atomic>
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/application/summary/summary_factory.h"

namespace Analysis {
namespace Application {
namespace {
using namespace Utils;
const uint8_t PROCESSOR_POOL_NUM = 5;
const std::vector<std::string> DATA_ASSEMBLE_LIST{
    PROCESSOR_OP_SUMMARY,
    PROCESSOR_NAME_NPU_MEM,
    PROCESSOR_NAME_NPU_MODULE_MEM,
    PROCESSOR_NAME_API,
    PROCESSOR_NAME_FUSION_OP,
    PROCESSOR_TASK_TIME_SUMMARY,
    PROCESSOR_NAME_STEP_TRACE
};

const std::vector<std::string> SUMMARY_DATA_PROCESS_LIST{
    PROCESSOR_NAME_COMMUNICATION,
    PROCESSOR_NAME_COMPUTE_TASK_INFO,
    PROCESSOR_NAME_TASK,
    PROCESSOR_PMU,
    PROCESSOR_NAME_NPU_MEM,
    PROCESSOR_NAME_NPU_MODULE_MEM,
    PROCESSOR_NAME_API,
    PROCESSOR_NAME_FUSION_OP,
    PROCESSOR_NAME_MODEL_NAME,
    PROCESSOR_NAME_STEP_TRACE
};
}

bool SummaryManager::Run(DataInventory& dataInventory)
{
    INFO("Start exporting summary!");
    PRINT_INFO("Start exporting the summary!");
    bool runFlag = ProcessSummary(dataInventory);
    if (!runFlag) {
        ERROR("The unified summary process failed to be executed.");
        PRINT_ERROR("The unified summary process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    PRINT_INFO("End exporting summary output_file. The file is stored in the PROF file.");
    return true;
}

bool SummaryManager::ProcessSummary(Analysis::Infra::DataInventory& dataInventory)
{
    Analysis::Utils::ThreadPool pool(PROCESSOR_POOL_NUM);
    pool.Start();
    std::atomic<bool> retFlag(true);
    for (const auto& name : DATA_ASSEMBLE_LIST) {
        pool.AddTask([this, &name, &retFlag, &dataInventory]() {
            auto assembler = SummaryFactory::GetAssemblerByName(name, profPath_);
            if (assembler == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = assembler->Run(dataInventory) && retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    if (!retFlag) {
        ERROR("The % for summary assemble failed to be executed.", profPath_);
        PRINT_ERROR("The % for summary assemble failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    return true;
}

std::vector<std::string> SummaryManager::GetProcessList()
{
    return SUMMARY_DATA_PROCESS_LIST;
}
}
}
