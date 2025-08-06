/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: adprof job interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-07-22
 */
#include "prof_adprof_job.h"
#include "tsdclient_plugin.h"
#include "driver_plugin.h"
#include "ai_drv_dev_api.h"
#include "platform/platform.h"
#include "config/config_manager.h"
#include "utils/utils.h"
#include "mmpa/mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

const std::string ADPROF_EVENT_MSG_GRP_NAME = "prof_adprof_grp";
constexpr uint32_t PROCESS_NUM = 1;

ProfAdprofJob::ProfAdprofJob() : channelId_(PROF_CHANNEL_ADPROF),
    procStatusParam_{0, TSD_SUB_PROC_ADPROF, SUB_PROCESS_STATUS_MAX},
    eventAttr_{0, channelId_, ADPROF_COLLECTION_JOB, false, false, false, false, 0, false, false, ""},
    processCount_(0), phyId_(0)
{
}

ProfAdprofJob::~ProfAdprofJob()
{
}

int ProfAdprofJob::InitAdprof()
{
    bool result = false;
    uint32_t ret = TsdClientPlugin::instance()->MsprofTsdCapabilityGet(collectionJobCfg_->comParams->devId,
        TSD_CAPABILITY_ADPROF, reinterpret_cast<uint64_t>(&result));
    if (ret != TSD_OK || !result) {
        MSPROF_LOGW("Tsd client not support adprof");
        return PROFILING_FAILED;
    }
    eventAttr_.deviceId = static_cast<uint32_t>(collectionJobCfg_->comParams->devId);
    eventAttr_.grpName = ADPROF_EVENT_MSG_GRP_NAME;
    if (profDrvEvent_.SubscribeEventThreadInit(&eventAttr_) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    ProcOpenArgs procOpenArgs = {TSD_SUB_PROC_ADPROF, nullptr, 0, nullptr, 0, nullptr, 0, nullptr};
    BuildSysProfCmdArg(procOpenArgs);
    pid_t adprofPid = 0;
    procOpenArgs.subPid = &adprofPid;
    ret = TsdClientPlugin::instance()->MsprofTsdProcessOpen(collectionJobCfg_->comParams->devId, &procOpenArgs);
    if (ret != TSD_OK) {
        MSPROF_LOGE("Call TsdProcessOpen failed, ret:%d, devId:%d", ret, collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }
    procStatusParam_.pid = adprofPid;
    ret = TsdClientPlugin::instance()->MsprofTsdGetProcListStatus(collectionJobCfg_->comParams->devId,
        &procStatusParam_, 1);
    if (ret != TSD_OK) {
        MSPROF_LOGE("Call TsdGetProcListStatus failed, ret:%d, devId:%d", ret, collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }
    if (procStatusParam_.curStat != SUB_PROCESS_STATUS_NORMAL) {
        MSPROF_LOGE("Adprof status '%d' not normal, devId:%d",
            static_cast<int32_t>(procStatusParam_.curStat), collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

void ProfAdprofJob::BuildSysProfCmdArg(ProcOpenArgs &procOpenArgs)
{
    cmdVec_.emplace_back("adprof");
    cmdVec_.emplace_back("host_pid:" + std::to_string(Utils::GetPid()));
    cmdVec_.emplace_back("dev_id:" + std::to_string(phyId_)); // set physical device id
    cmdVec_.emplace_back("profiling_period:" + std::to_string(collectionJobCfg_->comParams->params->profiling_period));
    if (collectionJobCfg_->comParams->params->cpu_profiling == MSVP_PROF_ON) {
        cmdVec_.emplace_back("cpu_profiling:" + collectionJobCfg_->comParams->params->cpu_profiling);
        cmdVec_.emplace_back("aiCtrlCpuProfiling:" + collectionJobCfg_->comParams->params->aiCtrlCpuProfiling);
        cmdVec_.emplace_back("ai_ctrl_cpu_profiling_events:" +
            collectionJobCfg_->comParams->params->ai_ctrl_cpu_profiling_events);
        cmdVec_.emplace_back("cpu_sampling_interval:" +
            std::to_string(collectionJobCfg_->comParams->params->cpu_sampling_interval));
    }
    if (collectionJobCfg_->comParams->params->sys_profiling == MSVP_PROF_ON) {
        cmdVec_.emplace_back("sys_profiling:" + collectionJobCfg_->comParams->params->sys_profiling);
        cmdVec_.emplace_back("sys_sampling_interval:" +
            std::to_string(collectionJobCfg_->comParams->params->sys_sampling_interval));
    }
    if (collectionJobCfg_->comParams->params->pid_profiling == MSVP_PROF_ON) {
        cmdVec_.emplace_back("pid_profiling:" + collectionJobCfg_->comParams->params->pid_profiling);
        cmdVec_.emplace_back("pid_sampling_interval:" +
            std::to_string(collectionJobCfg_->comParams->params->pid_sampling_interval));
    }
    params_.reserve(cmdVec_.size());
    for (uint32_t i = 0; i < cmdVec_.size(); i++) {
        params_[i].paramInfo = cmdVec_[i].c_str();
        params_[i].paramLen = cmdVec_[i].size();
    }

    procOpenArgs.extParamList = params_.data();
    procOpenArgs.extParamCnt = cmdVec_.size();
}

int ProfAdprofJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (!cfg->comParams->params->app.empty()) {
        MSPROF_LOGI("App mode not collect system level data");
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->cpu_profiling != MSVP_PROF_ON &&
        cfg->comParams->params->sys_profiling != MSVP_PROF_ON &&
        cfg->comParams->params->pid_profiling != MSVP_PROF_ON) {
        MSPROF_LOGW("Switch sys_profiling & cpu_profiling & pid_profiling not enable");
        return PROFILING_FAILED;
    }
    uint32_t device = static_cast<uint32_t>(cfg->comParams->devId);
    if (!DrvIsSupportAdprof(device) ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::CHIP_V4_2_0) {
        MSPROF_LOGW("Drv does not support adprof in current platform");
        return PROFILING_FAILED;
    }
    drvError_t ret = DriverPlugin::instance()->MsprofDrvDeviceGetPhyIdByIndex(device, &phyId_);
    if (ret != DRV_ERROR_NONE) {
        if (ret == DRV_ERROR_NOT_SUPPORT || ret == DRV_ERROR_INVALID_HANDLE) {
            MSPROF_LOGW("Driver not support drvDeviceGetPhyIdByIndex interface.");
            phyId_ = device;
        } else {
            MSPROF_LOGE("Failed to get phyId by devId: %u, ret: %d.", device, ret);
            return PROFILING_FAILED;
        }
    }
    collectionJobCfg_ = cfg;
    MSPROF_LOGI("Adprof get phyId: %u by devId: %u.", phyId_, device);
    if (InitAdprof() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init adprof");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfAdprofJob::Process()
{
    if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (!eventAttr_.isChannelValid) {
        MSPROF_LOGI("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId, channelId_);
        return PROFILING_SUCCESS;
    }
    // ensure only done once
    uint8_t expected = 0;
    if (!processCount_.compare_exchange_strong(expected, 1)) {
        MSPROF_LOGI("Only process once");
        return PROFILING_SUCCESS;
    }
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_, filePath);

    int32_t drvRet = DrvAdprofStart(collectionJobCfg_->comParams->devId, channelId_);
    FUNRET_CHECK_FAIL_PRINT(drvRet, PROFILING_SUCCESS);
    eventAttr_.isProcessRun = true;
    MSPROF_LOGI("Start profiling adprof, ret=%d", drvRet);
    return drvRet;
}

void ProfAdprofJob::CloseAdprof()
{
    uint32_t ret = TsdClientPlugin::instance()->MsprofProcessCloseSubProcList(collectionJobCfg_->comParams->devId,
        &procStatusParam_, PROCESS_NUM);
    if (ret != TSD_OK && ret != TSD_HDC_CLIENT_CLOSED_EXTERNAL) {
        MSPROF_LOGW("Close adprof process unexpectedly, ret:%u devId:%d", ret, collectionJobCfg_->comParams->devId);
    }
}


int ProfAdprofJob::Uninit()
{
    if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    eventAttr_.isExit = true;
    if (eventAttr_.isThreadStart) {
        if (MmJoinTask(&eventAttr_.handle) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Event thread not exist");
        } else {
            MSPROF_LOGI("Adprof event thread exit");
        }
    }
    if (eventAttr_.isAttachDevice) {
        profDrvEvent_.SubscribeEventThreadUninit(static_cast<uint32_t>(collectionJobCfg_->comParams->devId));
    }

    if (!eventAttr_.isProcessRun) {
        MSPROF_LOGI("ProfAdprofJob Process is not run, return");
        return PROFILING_SUCCESS;
    }

    int32_t ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    MSPROF_LOGI("Stop profiling Channel %d data, ret=%d", static_cast<int32_t>(channelId_), ret);

    CloseAdprof();

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);
    return ret;
}

}
}
}