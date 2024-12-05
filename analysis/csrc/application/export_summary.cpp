/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_summary.h
 * Description        : 导出op_summary
 * Author             : msprof team
 * Creation Date      : 2024/12/3
 * *****************************************************************************
 */

#include "analysis/csrc/application/include/export_summary.h"
#include <atomic>
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/application/summary/op_summary_assembler.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Utils;
namespace {
const uint8_t PROCESSOR_POOL_NUM = 5;
const std::vector<std::string> DATA_PROCESS_LIST{
        PROCESSOR_NAME_COMMUNICATION,
        PROCESSOR_NAME_COMPUTE_TASK_INFO,
        PROCESSOR_NAME_TASK,
};
}
bool ExportSummary::ProcessData(DataInventory &dataInventory)
{
    // hash数据作为其他流程的依赖数据，需要优先加载
    HashInitProcessor hashProcessor(profPath_);
    hashProcessor.Run(dataInventory, PROCESSOR_NAME_HASH);
    Analysis::Utils::ThreadPool pool(PROCESSOR_POOL_NUM);
    pool.Start();
    std::atomic<bool> retFlag(true);
    for (const auto &name : DATA_PROCESS_LIST) {
        pool.AddTask([this, &name, &retFlag, &dataInventory]() {
            auto processor = DataProcessorFactory::GetDataProcessByName(profPath_, name);
            if (processor == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = processor->Run(dataInventory, name) && retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    if (!retFlag) {
        ERROR("The % for data process failed to be executed.", profPath_);
        PRINT_ERROR("The % for data process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    return true;
}

bool ExportSummary::CheckProfDirsValid()
{
    if (profPath_.find("PROF") == std::string::npos) {
        ERROR("The path: % is not contains PROF, please check", profPath_);
        return false;
    }
    return true;
}

bool ExportSummary::Init()
{
    if (!CheckProfDirsValid()) {
        ERROR("Check TimelineManager output path failed, path is %.", profPath_);
        PRINT_ERROR("Check TimelineManager output path failed, path is %. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    if (!Analysis::Parser::Environment::Context::GetInstance().Load({profPath_})) {
        ERROR("JSON parameter loading failed. Please check if the JSON data is complete.");
        PRINT_ERROR("JSON parameter loading failed. Please check if the JSON data is complete. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    return true;
}

bool ExportSummary::Run()
{
    INFO("Start to load data!");
    PRINT_INFO("Start to load data!");
    if (!Init()) {
        return false;
    }
    DataInventory dataInventory;
    bool runFlag = ProcessData(dataInventory);
    const auto outputPath = File::PathJoin({profPath_, OUTPUT_PATH});
    if (!File::Exist(outputPath) && !File::CreateDir(outputPath)) {
        ERROR("Create mindstudio_profiler_output error, can't export data");
        PRINT_ERROR("Create mindstudio_profiler_output error, can't export data");
        return false;
    }
    OpSummaryAssembler assembler(PROCESSOR_OP_SUMMARY, profPath_);
    runFlag = assembler.Run(dataInventory) && runFlag;
    return runFlag;
}
}
}
