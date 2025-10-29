/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof bin
 * Author: ly
 * Create: 2020-12-10
 */

#include "msprof_bin.h"
#include <iostream>
#include <fstream>
#include <string>
#include "message/prof_params.h"
#include "errno/error_code.h"
#include "msprof_manager.h"
#include "msopprof_manager.h"
#include "input_parser.h"
#include "env_manager.h"
#include "platform/platform.h"
#include "config/config.h"
#include "dyn_prof_client.h"

using namespace Analysis::Dvvp::App;
using namespace Analysis::Dvvp::Msprof;
using namespace Analysis::Dvvp::Msopprof;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Msprofbin;
using namespace Collector::Dvvp::DynProf;

static void PrintStartLogInfo(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    std::string logInfo = "Start profiling";
    if (DynProfMngCli::instance()->IsEnableMode()) {
        logInfo += " in dynamic mode";
    } else {
        logInfo += !params->delayTime.empty() ? (" after " + params->delayTime + "s") : "";
        logInfo += !params->durationTime.empty() ? (" for " + params->durationTime + "s") : "";
    }
    logInfo += "...";
    CmdLog::instance()->CmdInfoLog(logInfo.c_str());
}

#ifdef __PROF_UT
int LltMain(int argc, const char **argv, const char **envp)
#else
int main(int argc, const char **argv, const char **envp)
#endif
{
    if (getuid() == 0) {
        CmdLog::instance()->CmdWarningLog("msprof is running as root. For security reasons,"
                                          " use a regular user account instead.");
    }
    std::vector<std::string> envpList;
    if (EnvManager::instance()->SetEnvList(envp, envpList)) {
        CmdLog::instance()->CmdErrorLog("Truncate env params due to exceeding limit!");
    }
    EnvManager::instance()->SetGlobalEnv(envpList);
    InputParser parser = InputParser();
    if (argc <= 1) {
        parser.MsprofCmdUsage("msprof needs input parameter.");
        return PROFILING_FAILED;
    }
    int ret = MsopprofManager::instance()->MsopprofProcess(argc, argv);
    if (ret == PROFILING_SUCCESS) {
        return PROFILING_SUCCESS;
    }
    auto params = parser.MsprofGetOpts(argc, argv);
    if (params == nullptr) {
        return PROFILING_FAILED;
    }
    if (parser.HasHelpParamOnly()) {
        return PROFILING_SUCCESS;
    }
    ret = MsprofManager::instance()->Init(params);
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Start profiling failed");
        return PROFILING_FAILED;
    }
    signal(SIGINT, [](int signum) {
        MsprofManager::instance()->StopNoWait();
    });
    PrintStartLogInfo(params);
    ret = MsprofManager::instance()->MsProcessCmd();
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Running profiling failed.Please check log for more info.");
        return PROFILING_FAILED;
    }
    // process the end of the msprofbin
    if (!DynProfMngCli::instance()->IsEnableMode()) {
        CmdLog::instance()->CmdInfoLog("Profiling finished.");
        if (MsprofManager::instance()->rMode_ && !MsprofManager::instance()->rMode_->jobResultDir_.empty()) {
            CmdLog::instance()->CmdInfoLog("Process profiling data complete. Data is saved in %s",
                MsprofManager::instance()->rMode_->jobResultDir_.c_str());
        }
    }
    return PROFILING_SUCCESS;
}