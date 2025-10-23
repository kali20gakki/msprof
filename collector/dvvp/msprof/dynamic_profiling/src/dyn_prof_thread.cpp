/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling thread
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-28
 */

#include "dyn_prof_thread.h"
#include "config.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "message/prof_params.h"
#include "prof_acl_mgr.h"
#include "prof_manager.h"
#include "prof_ge_core.h"
#include "msprof_dlog.h"

namespace Collector {
namespace Dvvp {
namespace DynProf {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::thread;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::host;

DynProfThread::DynProfThread() : delayTime_(0), durationTime_(0), durationSet_(false),
    started_(false), profHasStarted_(false)
{
}

DynProfThread::~DynProfThread()
{
    Stop();
}

int DynProfThread::GetDelayAndDurationTime()
{
    msprofEnvCfg_ = Utils::GetEnvString(PROFILER_SAMPLE_CONFIG_ENV);
    if (msprofEnvCfg_.empty()) {
        MSPROF_LOGE("Failed to get profiling sample config");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_RET(params, analysis::dvvp::message::ProfileParams, PROFILING_FAILED);
    if (!params->FromString(msprofEnvCfg_)) {
        MSPROF_LOGE("ProfileParams Parse Failed %s", msprofEnvCfg_.c_str());
        return PROFILING_FAILED;
    }
    if (params->delayTime.empty() && params->durationTime.empty()) {
        MSPROF_LOGE("Switch delay and duration both not set in dynamic profiling mode");
        return PROFILING_FAILED;
    }
    if (!params->delayTime.empty() && Utils::StrToUint32(delayTime_, params->delayTime) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Switch delay value %s is invalid", params->delayTime.c_str());
        return PROFILING_FAILED;
    }
    if (!params->durationTime.empty()) {
        if (Utils::StrToUint32(durationTime_, params->durationTime) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Switch duration value %s is invalid", params->durationTime.c_str());
            return PROFILING_FAILED;
        }
        durationSet_ = true;
    }
    MSPROF_LOGI("Dynamic profiling delay time: %us, duration time: %us", delayTime_, durationTime_);
    return PROFILING_SUCCESS;
}

int DynProfThread::Start()
{
    if (started_) {
        MSPROF_LOGW("Dynamic profiling thread has been started!");
        return PROFILING_SUCCESS;
    }
    if (GetDelayAndDurationTime() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    Thread::SetThreadName("MSVP_DYN_PROF");
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start dynamic profiling thread");
        return PROFILING_FAILED;
    }
    started_ = true;
    return PROFILING_SUCCESS;
}

void DynProfThread::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    std::unique_lock<std::mutex> lk(threadStopMtx_);
    MSPROF_LOGI("Wait dynamic profiling start after %us", delayTime_);
    auto status = cvThreadStop_.wait_for(lk, std::chrono::seconds(delayTime_));
    if (status != std::cv_status::timeout) {
        MSPROF_LOGI("Receive profiling stop signal");
        started_ = false;
        return;
    }
    StartProfTask();
    if (!durationSet_) { // 未设置duration，直接返回，等待任务结束自动停止采集
        started_ = false;
        return;
    }
    status = cvThreadStop_.wait_for(lk, std::chrono::seconds(durationTime_));
    if (status != std::cv_status::timeout) {
        MSPROF_LOGI("Receive profiling stop signal");
        StopProfTask();
        started_ = false;
        return;
    }
    MSPROF_LOGI("Stop prof after %us", durationTime_);
    StopProfTask();
    started_ = false;
}

int DynProfThread::Stop()
{
    if (started_) {
        {
            std::lock_guard<std::mutex> lk(threadStopMtx_);
            cvThreadStop_.notify_one();
        }
        if (Thread::Stop() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dynamic profiling thread stop failed");
        }
        started_ = false;
    }
    MSPROF_LOGI("Stop dynamic profiling Thread");
    return PROFILING_SUCCESS;
}

int DynProfThread::StartProfTask()
{
    if (deviceInfos_.empty()) {
        MSPROF_LOGW("No devices for dynamic profiling devices, maybe MsprofNotifySetdevice not called");
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

    {
        std::lock_guard<std::mutex> lk(deviceMtx_);
        std::vector<uint32_t> executeDevIdStrs;
        for (auto &devInfo : deviceInfos_) {
            executeDevIdStrs.emplace_back(devInfo.first);
            if (HandleDevProfTask(devInfo.second) != PROFILING_SUCCESS) {
                MSPROF_LOGE("Dynamic profiling start device: %u failed", devInfo.second);
                ReleaseProfTask(executeDevIdStrs);
                return PROFILING_FAILED;
            }
        }
    }
    profHasStarted_.store(true);
    MSPROF_LOGI("Dynamic profiling start");
    return PROFILING_SUCCESS;
}

void DynProfThread::ReleaseProfTask(const std::vector<uint32_t>& devIds)
{
    for (const auto& devId : devIds) {
        ge::EraseDevRecord(devId);
        if (Msprofiler::Api::ProfAclMgr::instance()->MsprofResetDeviceHandle(devId) != MSPROF_ERROR_NONE) {
            MSPROF_LOGE("Dynamic profiling reset device id[%u] failed", devId);
        }
    }
}
 
int DynProfThread::HandleDevProfTask(const ProfSetDevPara &devInfo)
{
    MSPROF_EVENT("Handle device %u prof task, if open: %d", devInfo.deviceId, devInfo.isOpen);
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
    } else {
        if (Msprofiler::Api::ProfAclMgr::instance()->MsprofResetDeviceHandle(devInfo.deviceId) != PROFILING_SUCCESS) {
            MSPROF_LOGE("MsprofResetDeviceHandle failed");
            return PROFILING_FAILED;
        }
    }
    profHasStarted_.store(false);
    MSPROF_LOGI("Succeed to Handle device %u prof task", devInfo.deviceId);
    return PROFILING_SUCCESS;
}
 
int DynProfThread::StopProfTask()
{
    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling stop MsprofFinalizeHandle failed, ret=%d.", ret);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void DynProfThread::SaveDevicesInfo(ProfSetDevPara data)
{
    if (profHasStarted_.load()) { // prof already start, start task for this device
        if (HandleDevProfTask(data) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to start device %u prof task", data.deviceId);
        }
    }
    std::lock_guard<std::mutex> lk(deviceMtx_);
    if (data.isOpen) {
        deviceInfos_.insert({data.deviceId, data});
    } else {
        deviceInfos_.erase(data.deviceId);
    }
}
} // DynProf
} // Dvvp
} // Collector
