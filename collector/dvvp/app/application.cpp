/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "application.h"
#include <fstream>
#include "config/config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "utils/utils.h"
#include "validation/param_validation.h"
#include "message/codec.h"
#include "env_manager.h"
namespace analysis {
namespace dvvp {
namespace app {
using namespace Analysis::Dvvp::App;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::validation;

int Application::PrepareLaunchAppCmd(std::stringstream &ssCmdApp,
                                     SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params->app_dir.empty()) {
        MSPROF_LOGE("app_dir is empty");
        return PROFILING_FAILED;
    }
    if (!ParamValidation::instance()->CheckAppNameIsValid(params->app)) {
        MSPROF_LOGE("app name(%s) is not valid.", params->app.c_str());
        return PROFILING_FAILED;
    }
    if (!ParamValidation::instance()->CheckNameContainsDangerCharacter(params->app_parameters)) {
        MSPROF_LOGE("app_parameters(%s) contains not allowed character.", params->app_parameters.c_str());
        return PROFILING_FAILED;
    }

    ssCmdApp << params->app_dir;
    ssCmdApp << MSVP_SLASH;
    ssCmdApp << params->app;
    if (!params->app_parameters.empty()) {
        ssCmdApp << " ";
        ssCmdApp << params->app_parameters;
    }

    return PROFILING_SUCCESS;
}

void Application::PrepareAppArgs(const std::vector<std::string> &params, std::vector<std::string> &argsV)
{
    for (unsigned int i = 1; i < params.size(); ++i) {
        if (!params[i].empty()) {
            argsV.push_back(params[i]);
        }
    }
}

int Application::PrepareAppEnvs(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    std::vector<std::string> &envsV)
{
    if (params == nullptr) {
        MSPROF_LOGE("params is nullptr");
        return PROFILING_FAILED;
    }

    // acl app
    SetAppEnv(params, envsV);

    return PROFILING_SUCCESS;
}

int Application::LaunchApp(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    mmProcess &appProcess)
{
    if (params == nullptr) {
        MSPROF_LOGE("[LaunchApp]params is empty.");
        return PROFILING_FAILED;
    }
    std::stringstream ssCmdApp;  // cmd
    if (PROFILING_SUCCESS != PrepareLaunchAppCmd(ssCmdApp, params)) {
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("launch app cmd: %s", ssCmdApp.str().c_str());
    std::vector<std::string> paramsCmd =
        analysis::dvvp::common::utils::Utils::Split(ssCmdApp.str());
    if (paramsCmd.empty()) {
        MSPROF_LOGE("[LaunchApp]paramsCmd is empty.");
        return PROFILING_FAILED;
    }
    if (analysis::dvvp::common::utils::Utils::IsSoftLink(paramsCmd[0])) {
        MSPROF_LOGE("app_dir(%s) is soft link.", Utils::BaseName(paramsCmd[0]).c_str());
        return PROFILING_FAILED;
    }
    std::string cmd = analysis::dvvp::common::utils::Utils::CanonicalizePath(paramsCmd[0]);
    if (cmd.empty()) {
        MSPROF_LOGE("app_dir(%s) is not valid.", Utils::BaseName(paramsCmd[0]).c_str());
        return PROFILING_FAILED;
    }

    std::string changeworkdirpath = analysis::dvvp::common::utils::Utils::GetChangeWorkDirPath(paramsCmd);
    if (changeworkdirpath.empty()) {
        MSPROF_LOGE("app_work_dir(%s) is not valid.", Utils::BaseName(changeworkdirpath).c_str());
        return PROFILING_FAILED;
    }
    std::vector<std::string> argsVec;
    std::vector<std::string> envsVec;
    PrepareAppArgs(paramsCmd, argsVec);  // args
    int ret = PrepareAppEnvs(params, envsVec);  // envs
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to PrepareAppEnvs, cmd:%s", cmd.c_str());
        return PROFILING_FAILED;
    }
    appProcess = MSVP_MMPROCESS;  // run
    if (analysis::dvvp::common::utils::Utils::ChangeWorkDir(changeworkdirpath) == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }

    int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
    ExecCmdParams execCmdParams(cmd, true, "");
    ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsVec, envsVec, exitCode, appProcess);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to launch app: %s", cmd.c_str());
        return PROFILING_FAILED;
    }
    return ret;
}


void Application::SetAppEnv(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    std::vector<std::string> &envsV)
{
    MSPROF_LOGI("Handle app_env param %s", params->app_env.c_str());
    std::vector<std::string> appEnvs = analysis::dvvp::common::utils::Utils::Split(params->app_env, false, "", ";");
    for (auto appEnv : appEnvs) {
        if (!appEnv.empty()) {
            if (appEnv.find("=") == std::string::npos) {
                MSPROF_LOGW("Invalid app_env params %s", appEnv.c_str());
                continue;
            }
            envsV.push_back(appEnv);
        }
    }
    auto envList = EnvManager::instance()->GetGlobalEnv();
    for (auto env : envList) {
        envsV.push_back(env);
    }
    std::string paramEnv = PROFILER_SAMPLE_CONFIG_ENV;
    paramEnv.append("=");
    paramEnv.append(params->ToString());
    envsV.push_back(paramEnv);
}
}  // namespace app
}  // namespace dvvp
}  // namespace analysis
