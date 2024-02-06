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
#include <iostream>

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/table_processor_factory.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "collector/dvvp/common/errno/error_code.h"
#include "collector/dvvp/common/config/config.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;
using namespace analysis::dvvp::common::error;

namespace {
    const std::vector<std::string> DB_NAME = {
        TABLE_NAME_TARGET_INFO_SESSION_TIME,
        TABLE_NAME_TARGET_INFO_NPU,
        TABLE_NAME_ENUM_API_LEVEL,
        TABLE_NAME_TASK,
        TABLE_NAME_COMPUTE_TASK_INFO,
        TABLE_NAME_COMMUNICATION_TASK_INFO,
        TABLE_NAME_API
    };
}

bool UnifiedDBManager::CheckProfDirsValid(const std::string outputDir,
                                          const std::set<std::string> &profFolderPaths, std::string &errInfo)
{
    if (profFolderPaths.empty()) {
        errInfo = "The prof files in the current directory are empty. Please check your file path.";
        return false;
    }

    if (Context::GetInstance().Load(profFolderPaths) == false) {
        errInfo = "JSON parameter loading failed. Please check if the JSON data is complete.";
        return false;
    }

    // 获取第一个元素
    auto it = profFolderPaths.begin();
    int64_t msprofBinPid = Context::GetInstance().GetMsBinPid(*it);
    if (msprofBinPid == analysis::dvvp::common::config::MSVP_MMPROCESS) {
        errInfo = "The current msprofBinPid is an invalid value:" + std::to_string(msprofBinPid) +
                  ". Please check the value of your path:" + *it + ".";
        return false;
    }

    // 从第二个元素开始遍历
    ++it;
    for (; it != profFolderPaths.end(); ++it) {
        if (Context::GetInstance().GetMsBinPid(*it) != msprofBinPid) {
            errInfo = "The profiling results under the " + outputDir + " path are not from "\
                       "the same data collection session. Please verify and rerun.";
            return false;
        }
    }
    return true;
}

int UnifiedDBManager::Init()
{
    Analysis::Log::GetInstance().Init(outputPath_);

    Analysis::Association::Credential::IdPool::GetInstance();

    if (Analysis::Parser::Environment::Context::GetInstance().Load(ProfFolderPaths_) == false) {
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
    for (auto name : DB_NAME) {
        pool.AddTask([this, name, &retFlag]() {
            std::shared_ptr<TableProcessor> processor =
                TableProcessorFactory::CreateTableProcessor(name, reportDBPath_, ProfFolderPaths_);
            if (processor == nullptr) {
                ERROR("% is not defined", name);
                retFlag = false;
                return;
            }
            retFlag = processor->Run() && retFlag;
        });
    }

    pool.WaitAllTasks();
    pool.Stop();

    if (retFlag == false) {
        ERROR("“The unified db process failed to be executed");
    }

    // string_id table 要在其他所有table 全部生成之后再去生成
    std::shared_ptr<TableProcessor> processor =
        TableProcessorFactory::CreateTableProcessor(TABLE_NAME_STRING_IDS, reportDBPath_, ProfFolderPaths_);
    if (processor == nullptr) {
        ERROR("% is not defined", TABLE_NAME_STRING_IDS);
        return PROFILING_FAILED;
    } else {
        if (!processor->Run()) {
            ERROR("Dump StringId failed.");
            return PROFILING_FAILED;
        }
    }

    // TABLE_NAME_STRING_IDS
    return PROFILING_SUCCESS;
}

}  // Database
}  // Viewer
}  // Analysis
