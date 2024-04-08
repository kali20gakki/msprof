/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: msopprof manager
 * Author: zyb
 * Create: 2023-09-18
 */

#include <string>
#include <csignal>
#include "utils/utils.h"
#include "cmd_log.h"
#include "env_manager.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "errno/error_code.h"
#include "msopprof_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Msopprof {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::App;
using namespace analysis::dvvp::common::config;
using namespace Collector::Dvvp::Msprofbin;

void SigHandler(int sig) {}

MsopprofManager::MsopprofManager()
{
    auto ascend_toolkit_home = Utils::GetEnvString("ASCEND_TOOLKIT_HOME");
    if (ascend_toolkit_home.empty()) {
        path_ = "";
    } else {
        std::string pathStr = ascend_toolkit_home;
        path_ = pathStr + "/tools/msopt/bin/msopprof";
    }
}

bool MsopprofManager::CheckInputDataValidity(int argc, CONST_CHAR_PTR argv[]) const
{
    if (argc > INPUT_MAX_LENTH || argv == nullptr) {
        CmdLog::instance()->CmdErrorLog("input data is invalid,"
            "please input argc less than %d and argv is not null", INPUT_MAX_LENTH);
        return false;
    }
    for (int i = 0; i < argc; i++) {
        if (strnlen(argv[i], INPUT_MAX_LENTH) == INPUT_MAX_LENTH) {
            CmdLog::instance()->CmdErrorLog("input data is invalid,"
                "please input the len of every argv less than %d", INPUT_MAX_LENTH);
            return false;
        }
    }
    return true;
}

int MsopprofManager::MsopprofProcess(int argc, CONST_CHAR_PTR argv[])
{
    if (!CheckInputDataValidity(argc, argv)) {
        return PROFILING_SUCCESS;
    }
    std::vector<std::string> op_argv;
    bool ret = CheckMsopprofIfExist(argc, argv, op_argv);
    if (!ret) {
        return PROFILING_FAILED;
    }
    if (!path_.empty() && (ParamValidation::instance()->CheckMsopprofBinValid(path_) == PROFILING_SUCCESS)) {
        ExecMsopprof(path_, op_argv);
    }
    return PROFILING_SUCCESS;
}

bool MsopprofManager::CheckMsopprofIfExist(int argc, CONST_CHAR_PTR argv[], std::vector<std::string> &op_argv) const
{
    bool ret = false;
    if (argc > 1 && strncmp(argv[1], "op", INPUT_MAX_LENTH) == 0) {
        ret = true;
    }
    if (ret) {
        if (path_.empty()) {
            CmdLog::instance()->CmdErrorLog("cannot find msopprof,"
            "because not set environment variable ASCEND_TOOLKIT_HOME,"
            "Maybe you should set setenv.sh.");
        } else {
            for (int i = 2; i < argc; i++) {
                op_argv.push_back(argv[i]);
            }
        }
    }
    return ret;
}

int MsopprofManager::ExecMsopprof(std::string path, std::vector<std::string> argsVec)
{
    signal(SIGINT, SigHandler);
    std::string cmd = path;
    ExecCmdParams execCmdParams(cmd, true, "");
    std::vector<std::string> envsVec = EnvManager::instance()->GetGlobalEnv();
    int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
    int ret = Utils::ExecCmd(execCmdParams, argsVec, envsVec, exitCode, opProcess_);
    if (ret != PROFILING_SUCCESS) {
        CMD_LOGE("Failed to launch msopprof");
        return PROFILING_FAILED;
    }
    bool isExited = false;
    ret = Utils::WaitProcess(opProcess_, isExited, exitCode, true);
    if (ret != PROFILING_SUCCESS) {
        CMD_LOGE("Failed to wait msopprof process");
        return PROFILING_FAILED;
    }
    return ret;
}
}
}
}
