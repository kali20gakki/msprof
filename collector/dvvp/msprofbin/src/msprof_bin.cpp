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
#include "input_parser.h"
#include "env_manager.h"
#include "platform/platform.h"
#include "config/config.h"

using namespace Analysis::Dvvp::App;
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Msprofbin;


#ifdef __PROF_UT
int LltMain(int argc, const char **argv, const char **envp)
#else
int main(int argc, const char **argv, const char **envp)
#endif
{
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
    auto params = parser.MsprofGetOpts(argc, argv);
    if (params == nullptr) {
        return PROFILING_FAILED;
    }
    if (parser.HasHelpParamOnly()) {
        return PROFILING_SUCCESS;
    }
    int ret = MsprofManager::instance()->Init(params);
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Start profiling failed");
        return PROFILING_FAILED;
    }
    signal(SIGINT, [](int signum) {
        MsprofManager::instance()->StopNoWait();
    });
    CmdLog::instance()->CmdInfoLog("Start profiling....");
    ret = MsprofManager::instance()->MsProcessCmd();
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Running profiling failed.Please check slog for more info.");
        return ret;
    } else {
        CmdLog::instance()->CmdInfoLog("Profiling finished.");
        if (MsprofManager::instance()->rMode_ != nullptr &&
            !MsprofManager::instance()->rMode_->jobResultDir_.empty()) {
            CmdLog::instance()->CmdInfoLog("Process profiling data complete. Data is saved in %s",
                MsprofManager::instance()->rMode_->jobResultDir_.c_str());
        }
    }
    return PROFILING_SUCCESS;
}