/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-10
 */
 
#include "dyn_prof_mgr.h"
#include "utils/utils.h"
#include "config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "prof_acl_mgr.h"
#include "prof_manager.h"
#include "prof_ge_core.h"
#include "msprof_error_manager.h"
#include "cmd_log.h"
 
namespace Collector {
namespace Dvvp {
namespace DynProf {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::thread;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::host;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Msprofbin;
 
DynProfMgr::DynProfMgr() : delayTime_(0), durationTime_(0), durationSet_(false), isStarted_(false)
{
}
 
DynProfMgr::~DynProfMgr()
{
    Stop();
}
 
int DynProfMgr::StartDynProf()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (isStarted_) {
        MSPROF_LOGI("Dynamic profiling task has been started.");
        return PROFILING_SUCCESS;
    }
    MSPROF_EVENT("Start dynamic profiling task");
    std::string dynProfModeEnv = Utils::GetEnvString(PROFILING_MODE_ENV);
    if (dynProfModeEnv == DAYNAMIC_PROFILING_VALUE) { // 命令行设置dynamic，创建server端，与命令行client端建联交互
        if (StartDynProfSrv() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Start dynamic profiling server failed");
            return PROFILING_FAILED;
        }
        isStarted_ = true;
        return PROFILING_SUCCESS;
    } else if (dynProfModeEnv == DELAY_DURARION_PROFILING_VALUE) { // 命令行设置delay或duration，起线程按设置时间启停
        if (Start() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Start dynamic profiling thread failed");
            return PROFILING_FAILED;
        }
        isStarted_ = true;
        return PROFILING_SUCCESS;
    } else {
        MSPROF_LOGE("PROFILING_MODE: %s is invalid, will not start dynamic profiling task", dynProfModeEnv.c_str());
        return PROFILING_FAILED;
    }
}
 
void DynProfMgr::StopDynProf()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (!isStarted_) {
        MSPROF_LOGI("Dynamic profiling task has not been started");
        return;
    }
    if (dynProfSrv_ != nullptr) {
        dynProfSrv_->NotifyClientDisconnet("Server process exit");
        (void)dynProfSrv_->Stop();
    } else {
        threadStop_.notify_one();
    }
    isStarted_ = false;
    MSPROF_EVENT("Dynamic profiling task stoped");
}
 
int DynProfMgr::StartDynProfSrv()
{
    MSVP_MAKE_SHARED0_RET(dynProfSrv_, DynProfServer, PROFILING_FAILED);
    if (dynProfSrv_->Start() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling start server fail.");
        dynProfSrv_.reset();
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Dynamic profiling start server success.");
    return PROFILING_SUCCESS;
}
 
bool DynProfMgr::IsDynProfStarted()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    return isStarted_;
}
 
void DynProfMgr::SetDeviceInfo(ProfSetDevPara *data)
{
    if (dynProfSrv_ != nullptr) {
        dynProfSrv_->SaveDevicesInfo(*data);
    } else {
        devicesInfo_.push_back(*data);
    }
}
 
int DynProfMgr::Start()
{
    msprofEnvCfg_ = Utils::GetEnvString(PROFILER_SAMPLE_CONFIG_ENV);
    if (msprofEnvCfg_.empty()) {
        MSPROF_LOGE("Failed to get profiling sample config");
        return PROFILING_FAILED;
    }
    MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, PROFILING_FAILED);
    if (!params_->FromString(msprofEnvCfg_)) {
        MSPROF_LOGE("ProfileParams Parse Failed %s", msprofEnvCfg_.c_str());
        return PROFILING_FAILED;
    }
    if (params_->delayTime.empty() && params_->durationTime.empty()) {
        MSPROF_LOGE("Switch delay and duration both not set in dynamic profiling mode");
        return PROFILING_FAILED;
    }
    if (!params_->delayTime.empty() &&
        Utils::StrToUnsignedLong(delayTime_, params_->delayTime) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Switch delay value %s is invalid", params_->delayTime.c_str());
        return PROFILING_FAILED;
    }
    if (!params_->durationTime.empty()) {
        if (Utils::StrToUnsignedLong(durationTime_, params_->durationTime) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Switch duration value %s is invalid", params_->durationTime.c_str());
            return PROFILING_FAILED;
        }
        durationSet_ = true;
    }
    Thread::SetThreadName("MSVP_DYN_PROF");
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start dynamic profiling thread");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
 
void DynProfMgr::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    std::unique_lock<std::mutex> lk(startMtx_);
    MSPROF_LOGI("Wait dynamic profiling start after %us", delayTime_);
    auto status = threadStop_.wait_for(lk, std::chrono::seconds(delayTime_));
    if (status != std::cv_status::timeout) {
        MSPROF_LOGI("Receive profiling stop signal");
        return;
    }
    StartProfTask();
    if (!durationSet_) { // 未设置duration，直接返回，等待任务结束自动停止采集
        return;
    }
    status = threadStop_.wait_for(lk, std::chrono::seconds(durationTime_));
    if (status != std::cv_status::timeout) {
        MSPROF_LOGI("Receive profiling stop signal");
        StopProfTask();
        return;
    }
    MSPROF_LOGI("Stop prof after %us", durationTime_);
    StopProfTask();
}
 
int DynProfMgr::Stop()
{
    if (isStarted_) {
        if (Thread::Stop() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dynamic profiling thread stop failed");
            return PROFILING_FAILED;
        }
        MSPROF_LOGI("Stop dynamic profiling Thread success");
        return PROFILING_SUCCESS;
    }
    return PROFILING_SUCCESS;
}
 
 
int DynProfMgr::StartProfTask()
{
    if (devicesInfo_.empty()) {
        MSPROF_LOGW("No devices for dynamic profiling devices, maybe MsprofNotifySetdevice not called");
        return PROFILING_SUCCESS;
    }
    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitAclEnv(msprofEnvCfg_);
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling start MsprofInitAclEnv failed, ret=%d.", ret);
        return PROFILING_FAILED;
    }
 
    if (Msprofiler::Api::ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling failed to init acl manager");
        Msprofiler::Api::ProfAclMgr::instance()->SetModeToOff();
        return PROFILING_FAILED;
    }
    Msprofiler::Api::ProfAclMgr::instance()->MsprofHostHandle();
 
    for (auto &devInfo : devicesInfo_) {
        if (StartDevProfTask(devInfo) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dynamic profiling start device: %u failed", devInfo.deviceId);
            return PROFILING_FAILED;
        }
    }
    MSPROF_LOGI("Dynamic profiling start");
    return PROFILING_SUCCESS;
}
 
int DynProfMgr::StartDevProfTask(const ProfSetDevPara &devInfo)
{
    MSPROF_EVENT("Start device %u prof task, if open: %d", devInfo.deviceId, devInfo.isOpen);
    if (devInfo.isOpen) {
        std::string info;
        if (!(ProfManager::instance()->CheckIfDevicesOnline(std::to_string(devInfo.deviceId), info))) {
            MSPROF_LOGE("DevId:%u is invalid, error info:%s", devInfo.deviceId, info.c_str());
            return PROFILING_FAILED;
        }
        int ret = ge::GeOpenDeviceHandle(devInfo.deviceId);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("GeOpenDeviceHandle failed");
            return PROFILING_FAILED;
        }
    }
    MSPROF_LOGI("Succeed to start device %u prof task", devInfo.deviceId);
    return PROFILING_SUCCESS;
}
 
int DynProfMgr::StopProfTask()
{
    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling stop MsprofFinalizeHandle failed, ret=%d.", ret);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
 
} // DynProf
} // Dvvp
} // Collect