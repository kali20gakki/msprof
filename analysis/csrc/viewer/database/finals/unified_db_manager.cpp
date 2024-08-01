/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_manager.cpp
 * Description        : db类型交付件生成
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/unified_db_manager.h"

#include <atomic>
#include <vector>

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/table_processor_factory.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;

namespace {
    const std::vector<std::string> PROCESSOR_NAME = {
        PROCESSOR_NAME_SESSION_TIME_INFO,
        PROCESSOR_NAME_NPU_INFO,
        PROCESSOR_NAME_HOST_INFO,
        PROCESSOR_NAME_TASK,
        PROCESSOR_NAME_COMPUTE_TASK_INFO,
        PROCESSOR_NAME_COMMUNICATION,
        PROCESSOR_NAME_API,
        PROCESSOR_NAME_ENUM,
        PROCESSOR_NAME_NPU_MEM,
        PROCESSOR_NAME_NPU_MODULE_MEM,
        PROCESSOR_NAME_NPU_OP_MEM,
        PROCESSOR_NAME_NIC,
        PROCESSOR_NAME_ROCE,
        PROCESSOR_NAME_HBM,
        PROCESSOR_NAME_DDR,
        PROCESSOR_NAME_PMU,
        PROCESSOR_NAME_LLC,
        PROCESSOR_NAME_PCIE,
        PROCESSOR_NAME_HCCS,
        PROCESSOR_NAME_ACC_PMU,
        PROCESSOR_NAME_SOC,
        PROCESSOR_NAME_META_DATA,
        PROCESSOR_NAME_AICORE_FREQ,
        PROCESSOR_NAME_MSTX
    };
}

bool UnifiedDBManager::CheckProfDirsValid(const std::string &output)
{
    if (output.empty()) {
        ERROR("The prof files in the current directory are empty. Please check your file path.");
        return false;
    }
    if (output.find("PROF") == std::string::npos) {
        auto files = Utils::File::GetOriginData(output, {"PROF"}, {""});
        profFolderPaths_.insert(files.begin(), files.end());
    } else {
        profFolderPaths_.insert(output);
    }
    if (profFolderPaths_.empty()) {
        ERROR("Can't find any PROF file.");
        return false;
    }
    return true;
}

std::string UnifiedDBManager::GetDBPath(const std::string& outputDir)
{
    return outputDir + "/" + DB_NAME_MSPROF_DB + "_" + Analysis::Utils::GetFormatLocalTime() + ".db";
}

bool UnifiedDBManager::Init()
{
    if (!CheckProfDirsValid(outputPath_)) {
        ERROR("Check UnifiedDBManager output path failed, path is %.", outputPath_);
        PRINT_ERROR("Check UnifiedDBManager output path failed, path is %. "
                    "Please check msprof_analysis_log in outputPath for more info.", outputPath_);
        return false;
    }
    Analysis::Association::Credential::IdPool::GetInstance();
    if (!Analysis::Parser::Environment::Context::GetInstance().Load(profFolderPaths_)) {
        ERROR("JSON parameter loading failed. Please check if the JSON data is complete.");
        PRINT_ERROR("JSON parameter loading failed. Please check if the JSON data is complete. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    return true;
}

bool UnifiedDBManager::RunOneProf(const std::string& profPath)
{
    const uint16_t tableProcessors = 5; // 最多有五个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    std::atomic<bool> retFlag(true);
    const auto dbPath = GetDBPath(profPath);
    for (const auto& name : PROCESSOR_NAME) {
        pool.AddTask([this, profPath, dbPath, name, &retFlag]() {
            std::shared_ptr<TableProcessor> processor =
                    TableProcessorFactory::CreateTableProcessor(name, dbPath, {profPath});
            if (processor == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = processor->Run() & retFlag;
        });
    }
    // 等待线程池内的线程执行完毕
    pool.WaitAllTasks();
    pool.Stop();
    // string_id table 要在其他所有table 全部生成之后再去生成
    std::shared_ptr<TableProcessor> processor =
            TableProcessorFactory::CreateTableProcessor(PROCESSOR_NAME_STRING_IDS, dbPath, {profPath});
    if (processor == nullptr || !processor->Run()) {
        ERROR("% error.", PROCESSOR_NAME_STRING_IDS);
        retFlag = false;
    }
    if (!retFlag) {
        ERROR("The % process failed to be executed.", dbPath);
        PRINT_ERROR("The % process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.", dbPath);
        return false;
    }
    return true;
}

bool UnifiedDBManager::Run()
{
    INFO("Start exporting the msprof.db!");
    PRINT_INFO("Start exporting the msprof.db!");
    bool runFlag = true;
    for (const auto& profPath : profFolderPaths_) {
        runFlag = RunOneProf(profPath) & runFlag;
    }
    if (!runFlag) {
        ERROR("The unified db process failed to be executed.");
        PRINT_ERROR("The unified db process failed to be executed. "
                    "Please check msprof_analysis_log in outputPath for more info.");
        return false;
    }
    PRINT_INFO("End exporting db output_file. The file is stored in the PROF file.");
    return true;
}

}  // Database
}  // Viewer
}  // Analysis
