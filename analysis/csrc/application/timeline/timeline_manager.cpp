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

const std::unordered_map<JsonProcess, std::string> JSON_TO_ASSEMBLER_TABLE{
    {JsonProcess::ASCEND,           PROCESS_TASK},
    {JsonProcess::ACC_PMU,          PROCESS_ACC_PMU},
    {JsonProcess::CANN,             PROCESS_API},
    {JsonProcess::DDR,              PROCESS_DDR},
    {JsonProcess::STARS_CHIP_TRANS, PROCESS_STARS_CHIP_TRANS},
    {JsonProcess::HBM,              PROCESS_HBM},
    {JsonProcess::HCCL,             PROCESS_HCCL},
    {JsonProcess::HCCS,             PROCESS_HCCS},
    {JsonProcess::NETWORK_USAGE,    PROCESS_NETWORK_USAGE},
    {JsonProcess::DISK_USAGE,       PROCESS_DISK_USAGE},
    {JsonProcess::MEMORY_USAGE,     PROCESS_MEMORY_USAGE},
    {JsonProcess::CPU_USAGE,        PROCESS_CPU_USAGE},
    {JsonProcess::MSPROFTX,         PROCESS_MSPROFTX},
    {JsonProcess::NPU_MEM,          PROCESS_NPU_MEM},
    {JsonProcess::OVERLAP_ANALYSE,  PROCESS_OVERLAP_ANALYSE},
    {JsonProcess::PCIE,             PROCESS_PCIE},
    {JsonProcess::SIO,              PROCESS_SIO},
    {JsonProcess::STARS_SOC,        PROCESS_STARS_SOC},
    {JsonProcess::STEP_TRACE,       PROCESS_STEP_TRACE},
    {JsonProcess::FREQ,             PROCESS_AI_CORE_FREQ},
    {JsonProcess::LLC,              PROCESS_LLC},
    {JsonProcess::NIC,              PROCESS_NIC},
    {JsonProcess::ROCE,             PROCESS_ROCE},
    {JsonProcess::QOS,              PROCESS_QOS},
    {JsonProcess::DEVICE_TX,        PROCESS_DEVICE_TX},
};
}

bool TimelineManager::ProcessTimeLine(DataInventory& dataInventory, const std::vector<JsonProcess>& jsonProcess)
{
    const uint16_t tableProcessors = 10; // 最多有10个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    std::atomic<bool> retFlag(true);
    std::vector<std::string> assemblerList = GetAssemblerList(jsonProcess);
    for (const auto& name : assemblerList) {
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
    tempFile.append("_").append(GetTimeStampStr()).append(JSON_SUFFIX);
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

bool TimelineManager::Run(DataInventory &dataInventory, const std::vector<JsonProcess>& jsonProcess)
{
    INFO("Start exporting timeline!");
    PRINT_INFO("Start exporting the timeline!");
    if (!PreDumpJson(dataInventory)) {
        WARN("Can't Get data from dataInventory after data process");
        PRINT_WARN("Can't export timeline, msprof_analysis_log in outputPath for more info");
        return true;
    }
    bool runFlag = ProcessTimeLine(dataInventory, jsonProcess);
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

std::vector<std::string> TimelineManager::GetAssemblerList(const std::vector<JsonProcess>& jsonProcess)
{
    std::vector<std::string> assemblerList;
    for (const auto& jsonEnum : jsonProcess) {
        assemblerList.push_back(JSON_TO_ASSEMBLER_TABLE.at(jsonEnum));
    }
    return assemblerList;
}

}
}

