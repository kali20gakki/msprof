/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_manager.h
 * Description        : export模块统一控制类
 * Author             : msprof team
 * Creation Date      : 2024/11/1
 * *****************************************************************************
 */

#include "analysis/csrc/application/include/export_manager.h"
#include <atomic>
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/application/timeline/timeline_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/dfx/error_code.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
namespace {
const std::vector<std::string> DATA_PROCESS_LIST{
        PROCESSOR_NAME_API,
        PROCESSOR_NAME_COMMUNICATION,
        PROCESSOR_NAME_COMPUTE_TASK_INFO,
        PROCESSOR_NAME_KFC_TASK,
        PROCESSOR_NAME_KFC_COMM,
        PROCESSOR_NAME_DEVICE_TX,
        PROCESSOR_NAME_MSTX,
        PROCESSOR_NAME_STEP_TRACE,
        PROCESSOR_NAME_TASK,
        PROCESSOR_NAME_ACC_PMU,
        PROCESSOR_NAME_AICORE_FREQ,
        PROCESSOR_NAME_CHIP_TRAINS,
        PROCESSOR_NAME_DDR,
        PROCESSOR_NAME_HBM,
        PROCESSOR_NAME_HCCS,
        PROCESSOR_NAME_CPU_USAGE,
        PROCESSOR_NAME_MEM_USAGE,
        PROCESSOR_NAME_DISK_USAGE,
        PROCESSOR_NAME_NETWORK_USAGE,
        PROCESSOR_NAME_LLC,
        PROCESSOR_NAME_NPU_MEM,
        PROCESSOR_NAME_PCIE,
        PROCESSOR_NAME_SIO,
        PROCESSOR_NAME_SOC,
        PROCESSOR_NAME_NIC,
        PROCESSOR_NAME_ROCE,
        PROCESSOR_NAME_QOS,
        PROCESSOR_MC2_COMM_INFO,
        PROCESSOR_NAME_MEMCPY_INFO
};
}

bool ExportManager::ProcessData(DataInventory &dataInventory)
{
    // hash数据作为其他流程的依赖数据，需要优先加载
    HashInitProcessor hashProcessor(profPath_);
    hashProcessor.Run(dataInventory, PROCESSOR_NAME_HASH);
    const uint16_t tableProcessors = 10; // 最多有10个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
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

bool ExportManager::CheckProfDirsValid()
{
    if (profPath_.find("PROF") == std::string::npos) {
        ERROR("The path: % is not contains PROF, please check", profPath_);
        return false;
    }
    return true;
}

bool ExportManager::Init()
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

bool ExportManager::Run()
{
    INFO("Start to load data!");
    PRINT_INFO("Start to load data!");
    if (!Init()) {
        return false;
    }
    DataInventory dataInventory;
    bool runFlag = ProcessData(dataInventory);
    std::string outputPath = File::PathJoin({profPath_, OUTPUT_PATH});
    if (!File::Exist(outputPath) && !File::CreateDir(outputPath)) {
        ERROR("Create mindstudio_profiler_output error, can't export data");
        PRINT_ERROR("Create mindstudio_profiler_output error, can't export data");
        return false;
    }
    TimelineManager timelineManager(profPath_, outputPath);
    std::vector<JsonProcess> jsonProcesses = GetProcessEnum();
    runFlag = timelineManager.Run(dataInventory, jsonProcesses) && runFlag;
    return runFlag;
}

std::vector<JsonProcess> ExportManager::GetProcessEnum()
{
    if (jsonPath_.empty()) {
        INFO("The report parameter is not used.");
        PRINT_INFO("The report parameter is not used.");
        return allProcesses;
    }
    FileReader fd(jsonPath_);
    nlohmann::json config;
    if (fd.ReadJson(config) != ANALYSIS_OK) {
        ERROR("Load report config failed: '%'.", jsonPath_);
        PRINT_ERROR("Load report config failed: '%'.", jsonPath_);
        return allProcesses;
    }
    auto jsonProcessConfig = config["json_process"];
    if (jsonProcessConfig.is_null() || !jsonProcessConfig.is_object() || jsonProcessConfig.empty()) {
        INFO("The json_process is not exist.");
        PRINT_INFO("The json_process is not exist.");
        return allProcesses;
    }
    std::vector<JsonProcess> jsonProcesses;
    for (nlohmann::json::iterator it = jsonProcessConfig.begin(); it != jsonProcessConfig.end(); ++it) {
        if (strToJsonProcess.find(it.key()) == strToJsonProcess.end()) {
            ERROR("Json process contains invalid: '%'.", it.key());
            PRINT_ERROR("Json process contains invalid: '%'.", it.key());
            return allProcesses;
        }
        if (!it.value().is_boolean()) {
            ERROR("Json contains invalid value: '%', only the bool type is supported.", it.value());
            PRINT_ERROR("Json contains invalid value: '%', only the bool type is supported.", it.value());
            return allProcesses;
        }
        if (it.value()) {
            auto processEnum = strToJsonProcess.at(it.key());
            jsonProcesses.push_back(processEnum);
        }
    }
    return std::move(jsonProcesses);
}

}
}