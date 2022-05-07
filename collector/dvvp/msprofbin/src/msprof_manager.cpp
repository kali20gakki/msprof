/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof manager
 * Author: ly
 * Create: 2020-12-10
 */

#include "msprof_manager.h"
#include "errno/error_code.h"
#include "cmd_log.h"
#include "message/prof_params.h"
#include "platform/platform.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Collector::Dvvp::Msprofbin;
using namespace Analysis::Dvvp::Common::Platform;

MsprofManager::MsprofManager() {}

MsprofManager::~MsprofManager() {}

int MsprofManager::Init(SHARED_PTR_ALIA<ProfileParams> params)
{
    MSPROF_LOGI("init MsprofManager begin.");
    if (params == nullptr) {
        MSPROF_LOGE("MsprofManager init failed because of params is null.");
        return PROFILING_FAILED;
    }
    params_ = params;
    if (GenerateRunningMode() != PROFILING_SUCCESS) {
        MSPROF_LOGE("MsprofManager init failed because of generating running mode error.");
        return PROFILING_FAILED;
    }
    if (ParamsCheck() != PROFILING_SUCCESS) {
        MSPROF_LOGE("MsprofManager init failed because of checking params failed.");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("init MsprofManager end.");
    return PROFILING_SUCCESS;
}

int MsprofManager::UnInit()
{
    params_.reset();
    rMode_.reset();
    return PROFILING_SUCCESS;
}

int MsprofManager::MsProcessCmd() const
{
    if (rMode_ == nullptr) {
        MSPROF_LOGE("[MsprocessCmd] No running mode found!");
        return PROFILING_FAILED;
    }
    return rMode_->RunModeTasks();
}

void MsprofManager::StopNoWait() const
{
    if (rMode_ == nullptr) {
        return;
    }
    CmdLog::instance()->CmdWarningLog("Receive stop singal.");
    rMode_->StopRunningTasks();
    rMode_->UpdateOutputDirInfo();
    rMode_->isQuit_ = true;
}

SHARED_PTR_ALIA<ProfTask> MsprofManager::GetTask(const std::string &jobId)
{
    if (rMode_ == nullptr) {
        MSPROF_LOGE("[MsprocessCmd] Get running mode failed");
        return nullptr;
    }
    return rMode_->GetRunningTask(jobId);
}

int MsprofManager::GenerateRunningMode()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[MsprocessCmd] Invalid params!");
        return PROFILING_FAILED;
    }
    if (!params_->app.empty()) {
        MSVP_MAKE_SHARED2_RET(rMode_, AppMode, "application", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    if (!params_->devices.empty()) {
        if (Platform::instance()->PlatformIsHelperHostSide()) {
            CmdLog::instance()->CmdErrorLog("sys-device mode not support in helper");
            return PROFILING_FAILED;
        }
        MSVP_MAKE_SHARED2_RET(rMode_, SystemMode, "sys-devices", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    if (!params_->host_sys.empty()) {
        if (Platform::instance()->PlatformIsHelperHostSide()) {
            CmdLog::instance()->CmdErrorLog("sys-host_sys mode not support in helper");
            return PROFILING_FAILED;
        }
        MSVP_MAKE_SHARED2_RET(rMode_, SystemMode, "host-sys", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    if (params_->parseSwitch == MSVP_PROF_ON) {
        MSVP_MAKE_SHARED2_RET(rMode_, ParseMode, "parse", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    if (params_->querySwitch == MSVP_PROF_ON) {
        MSVP_MAKE_SHARED2_RET(rMode_, QueryMode, "query", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    if (params_->exportSwitch == MSVP_PROF_ON) {
        MSVP_MAKE_SHARED2_RET(rMode_, ExportMode, "export", params_, PROFILING_FAILED);
        return PROFILING_SUCCESS;
    }
    CmdLog::instance()->CmdErrorLog("No valid argument found in --application "
    "--sys-devices --host-sys --parse --query --export");
    return PROFILING_FAILED;
}

int MsprofManager::ParamsCheck() const
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[MsprocessCmd] Invalid params!");
        return PROFILING_FAILED;
    }
    if (rMode_ == nullptr) {
        MSPROF_LOGE("[MsprocessCmd] Invalid running mode!");
        return PROFILING_FAILED;
    }
    if (rMode_->ModeParamsCheck() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[MsprocessCmd] Invalid input arguments!");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
}
}
}
