/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-10
 */
 
#include "dyn_prof_mgr.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "config.h"

namespace Collector {
namespace Dvvp {
namespace DynProf {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
 
DynProfMgr::DynProfMgr() : isStarted_(false)
{
}
 
DynProfMgr::~DynProfMgr()
{
    StopDynProf();
}
 
int DynProfMgr::StartDynProf()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (isStarted_) {
        MSPROF_LOGI("Dynamic profiling task has been started");
        return PROFILING_SUCCESS;
    }
    MSPROF_EVENT("Start dynamic profiling task");
    std::string dynProfModeEnv = Utils::GetEnvString(PROFILING_MODE_ENV);
    if (dynProfModeEnv == DYNAMIC_PROFILING_VALUE) { // 命令行设置dynamic，创建server端，与命令行client端建联交互
        MSVP_MAKE_SHARED0_RET(dynProfSrv_, DynProfServer, PROFILING_FAILED);
        if (dynProfSrv_->Start() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dynamic profiling start server fail");
            dynProfSrv_.reset();
            return PROFILING_FAILED;
        }
    } else if (dynProfModeEnv == DELAY_DURATION_PROFILING_VALUE) { // 命令行设置delay或duration，起线程按设置时间启停
        MSVP_MAKE_SHARED0_RET(dynProfThread_, DynProfThread, PROFILING_FAILED);
        if (dynProfThread_->Start() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dynamic profiling start thread fail");
            dynProfThread_.reset();
            return PROFILING_FAILED;
        }
    } else {
        MSPROF_LOGE("PROFILING_MODE: %s is invalid, will not start dynamic profiling task", dynProfModeEnv.c_str());
        return PROFILING_FAILED;
    }
    isStarted_ = true;
    return PROFILING_SUCCESS;
}
 
void DynProfMgr::StopDynProf()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (!isStarted_) {
        MSPROF_LOGI("Dynamic profiling task has not been started");
        return;
    }
    if (dynProfSrv_ != nullptr) {
        dynProfSrv_->NotifyClientDisconnect("Dynamic profiling Server exit");
        dynProfSrv_->Stop();
    } else if (dynProfThread_ != nullptr) {
        dynProfThread_->Stop();
    }
    isStarted_ = false;
    MSPROF_EVENT("Dynamic profiling task stopped");
}

void DynProfMgr::SaveDevicesInfo(ProfSetDevPara data)
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (dynProfSrv_ != nullptr) {
        dynProfSrv_->SaveDevicesInfo(data);
    } else if (dynProfThread_ != nullptr) {
        dynProfThread_->SaveDevicesInfo(data);
    } else {
        MSPROF_LOGW("Dynamic profiling not start, maybe MsprofInit has not called");
    }
}

bool DynProfMgr::IsDynProfStarted()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    return isStarted_;
}
} // DynProf
} // Dvvp
} // Collect