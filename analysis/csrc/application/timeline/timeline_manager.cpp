/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : timeline_manager.cpp
 * Description        : timeline_manager，timeline导出调度
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/timeline_manager.h"
#include <atomic>
#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"

namespace Analysis {
namespace Application {
namespace {
const std::string PREFIX_CONTEXT = "[";
const std::string SUFFIX_CONTEXT = "]";
const int JSON_FILE_OFFSET = -1;
// 数据中心中最少也会有hash类数据
const size_t MIN_NUM_FOR_IOC = std::set<std::string>{PROCESSOR_NAME_HASH}.size();

const std::vector<std::string> ASSEMBLER_LIST{
    PROCESS_TASK,
    PROCESS_ACC_PMU,
    PROCESS_API,
    PROCESS_DDR,
    PROCESS_STARS_CHIP_TRANS,
    PROCESS_HBM,
    PROCESS_HCCL,
    PROCESS_HCCS,
    PROCESS_NETWORK_USAGE,
    PROCESS_DISK_USAGE,
    PROCESS_MEMORY_USAGE,
    PROCESS_CPU_USAGE,
    PROCESS_MSPROFTX,
    PROCESS_NPU_MEM,
    PROCESS_OVERLAP_ANALYSE,
    PROCESS_PCIE,
    PROCESS_SIO,
    PROCESS_STARS_SOC,
    PROCESS_STEP_TRACE,
    PROCESS_AI_CORE_FREQ,
    PROCESS_LLC,
    PROCESS_NIC,
    PROCESS_ROCE,
    PROCESS_QOS,
    PROCESS_DEVICE_TX
};
}

bool TimelineManager::ProcessTimeLine(Analysis::Infra::DataInventory& dataInventory)
{
    const uint16_t tableProcessors = 10; // 最多有10个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    std::atomic<bool> retFlag(true);
    for (const auto &name : ASSEMBLER_LIST) {
        pool.AddTask([this, &name, &retFlag, &dataInventory]() {
            auto assembler = TimelineFactory::GetAssemblerByName(name);
            if (assembler == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = assembler->Run(dataInventory, profPath_) && retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    if (!retFlag) {
        ERROR("The % for json assemble failed to be executed.", profPath_);
        PRINT_ERROR("The % for json assemble failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.", profPath_);
        return false;
    }
    return true;
}

void TimelineManager::WriteFile(const std::string &filePrefix, FileCategory category)
{
    auto tempFile = filePrefix;
    tempFile.append("_").append(timestampStr).append(JSON_SUFFIX);
    auto filePath = File::PathJoin({outputPath_, tempFile});
    DumpTool::WriteToFile(filePath, PREFIX_CONTEXT.c_str(), PREFIX_CONTEXT.size(), category);
    fileType_.emplace(category, filePath);
}

bool TimelineManager::PreDumpJson(DataInventory &dataInventory)
{
    if (dataInventory.Size() <= MIN_NUM_FOR_IOC) {
        return false;
    }
    WriteFile(MSPROF_JSON_FILE, FileCategory::MSPROF);
    if (dataInventory.GetPtr<std::vector<TrainTraceData>>()) {
        WriteFile(STEP_TRACE_FILE, FileCategory::STEP);
    }
    if (dataInventory.GetPtr<std::vector<MsprofTxHostData>>()) {
        WriteFile(MSPROF_TX_FILE, FileCategory::MSPROF_TX);
    }
    return true;
}

void TimelineManager::PostDumpJson()
{
    for (const auto &it : fileType_) {
        // 此处需要覆盖文件末尾的","，实测必须使用in、out、ate三种模式打开文件，才可以实现覆盖写入
        FileWriter writer(it.second, std::ios::in | std::ios::out | std::ios::ate);
        writer.WriteTextBack(SUFFIX_CONTEXT, JSON_FILE_OFFSET);
    }
}

bool TimelineManager::Run(DataInventory &dataInventory)
{
    INFO("Start exporting timeline!");
    PRINT_INFO("Start exporting the timeline!");
    if (!PreDumpJson(dataInventory)) {
        WARN("Can't Get data from dataInventory after data process");
        PRINT_WARN("Can't export timeline, msprof_analysis_log in outputPath for more info");
        return true;
    }
    bool runFlag = ProcessTimeLine(dataInventory);
    PostDumpJson();
    if (!runFlag) {
        ERROR("The unified timeline process failed to be executed.");
        PRINT_ERROR("The unified timeline process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    PRINT_INFO("End exporting timeline output_file. The file is stored in the PROF file.");
    return true;
}
}
}

