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
#include "collector/dvvp/common/config/config.h"
#include "collector/dvvp/common/errno/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;
using namespace analysis::dvvp::common::error;

namespace {
    const std::vector<std::string> PROCESSOR_NAME = {
        PROCESSOR_NAME_SESSION_TIME_INFO,
        PROCESSOR_NAME_NPU_INFO,
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
    };
}

bool UnifiedDBManager::CheckProfDirsValid(const std::string& outputDir,
                                          const std::set<std::string> &profFolderPaths, std::string &errInfo)
{
    if (profFolderPaths.empty()) {
        errInfo = "The prof files in the current directory are empty. Please check your file path.";
        return false;
    }

    // 对于单PROF情况,不校验MsprofBinPid
    if (profFolderPaths.size() == 1) {
        return true;
    }

    if (!Context::GetInstance().Load(profFolderPaths)) {
        errInfo = "JSON parameter loading failed. Please check if the JSON data is complete.";
        return false;
    }

    int64_t preMsprofBinPid = analysis::dvvp::common::config::MSVP_MMPROCESS;
    for (const auto& path : profFolderPaths) {
        int64_t msprofBinPid = Context::GetInstance().GetMsBinPid(path);
        if (msprofBinPid == analysis::dvvp::common::config::MSVP_MMPROCESS) {
            errInfo = "The current msprofBinPid is an invalid value:" + std::to_string(msprofBinPid) +
                      ". Please check the value of your path:" + path + ".";
            return false;
        }
        if (preMsprofBinPid != analysis::dvvp::common::config::MSVP_MMPROCESS && preMsprofBinPid != msprofBinPid) {
            errInfo = "The profiling results under the " + outputDir + " path are not from "\
                       "the same data collection session. Please verify and rerun.";
            return false;
        }
        preMsprofBinPid = msprofBinPid;
    }
    return true;
}

int UnifiedDBManager::Init()
{
    Analysis::Log::GetInstance().Init(outputPath_);

    Analysis::Association::Credential::IdPool::GetInstance();

    if (!Analysis::Parser::Environment::Context::GetInstance().Load(ProfFolderPaths_)) {
        ERROR("JSON parameter loading failed. Please check if the JSON data is complete.");
        return PROFILING_FAILED;
    }

    // reportdb name and Path
    reportDBPath_ = outputPath_ + "/" +
                    DB_NAME_REPORT_DB + "_" + Analysis::Utils::GetFormatLocalTime() + ".db";

    return PROFILING_SUCCESS;
}

int UnifiedDBManager::Run()
{
    INFO("Start export report.db %", reportDBPath_);
    const uint16_t tableProcessors = 5; // 最多有五个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    std::atomic<bool> retFlag(true);
    for (const auto& name : PROCESSOR_NAME) {
        pool.AddTask([this, name, &retFlag]() {
            std::shared_ptr<TableProcessor> processor =
                TableProcessorFactory::CreateTableProcessor(name, reportDBPath_, ProfFolderPaths_);
            if (processor == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = processor->Run() & retFlag;
        });
    }

    pool.WaitAllTasks();
    pool.Stop();

    if (!retFlag) {
        ERROR("The unified db process failed to be executed");
    }

    // string_id table 要在其他所有table 全部生成之后再去生成
    std::shared_ptr<TableProcessor> processor =
        TableProcessorFactory::CreateTableProcessor(PROCESSOR_NAME_STRING_IDS, reportDBPath_, ProfFolderPaths_);
    if (processor == nullptr) {
        ERROR("% is not defined", PROCESSOR_NAME_STRING_IDS);
        return PROFILING_FAILED;
    } else {
        if (!processor->Run()) {
            ERROR("Dump StringId failed.");
            return PROFILING_FAILED;
        }
    }

    // PROCESSOR_NAME_STRING_IDS
    return PROFILING_SUCCESS;
}

}  // Database
}  // Viewer
}  // Analysis
