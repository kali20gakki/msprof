/**
* @file job_device_soc.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "job_device_soc.h"

#include "ai_drv_prof_api.h"
#include "ai_drv_dev_api.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "prof_channel_manager.h"
#include "prof_peripheral_job.h"
#include "prof_host_job.h"
#include "proto/profiler.pb.h"
#include "task_relationship_mgr.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::TaskHandle;

JobDeviceSoc::JobDeviceSoc(int devIndexId)
    : devIndexId_(devIndexId),
      isStarted_(false)
{
    CollectionJobV_.fill(CollectionJobT());
}

JobDeviceSoc::~JobDeviceSoc()
{
    params_ = nullptr;
    collectionjobComnCfg_ = nullptr;
}

void JobDeviceSoc::GetAndStoreStartTime(int hostProfiling)
{
    if (hostProfiling) {
        return;
    }
    analysis::dvvp::common::utils::Utils::GetTime(startRealtime_, startMono_, cntvct_);
    analysis::dvvp::driver::DrvGetDeviceTime(devIndexId_, deviceStartMono_, deviceCntvct_);
    MSPROF_LOGI("devId:%d, startRealtime=%llu ns, startMono=%llu ns, cntvct=%llu, deviceStartMono=%llu ns,"
        " deviceCntvct=%llu", devIndexId_, startRealtime_, startMono_, cntvct_, deviceStartMono_, deviceCntvct_);
    std::string deviceId = std::to_string(
        TaskRelationshipMgr::instance()->GetFlushSuffixDevId(params_->job_id, devIndexId_));
    std::stringstream timeData;
    timeData << "[" << std::string(DEVICE_TAG_KEY) << deviceId << "]" << std::endl;
    std::string startTime = Utils::GenerateStartTime(startRealtime_, startMono_, cntvct_);
    timeData << startTime;
    std::string fileName = "host_start.log." + deviceId;
    int ret = StoreTime(fileName, timeData.str());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[JobDeviceSoc]Failed to upload data for %s", fileName.c_str());
        return;
    }
    fileName = "dev_start.log." + deviceId;
    startTime = Utils::GenerateStartTime(startRealtime_, deviceStartMono_, deviceCntvct_);
    ret = StoreTime(fileName, startTime);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[JobDeviceSoc]Failed to upload data for %s", fileName.c_str());
        return;
    }
}

int JobDeviceSoc::StoreTime(const std::string &fileName, const std::string &startTime)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->dev_id = std::to_string(devIndexId_);
    jobCtx->job_id = params_->job_id;
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    MSPROF_LOGI("[JobDeviceSoc]storeTime.id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(params_->job_id,
        startTime, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[JobDeviceSoc]Failed to upload data for %s", fileName.c_str());
    }
    return ret;
}

int JobDeviceSoc::StartProfHandle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    params_ = params;
    tmpResultDir_ = params_->result_dir;
    MSVP_MAKE_SHARED0_RET(collectionjobComnCfg_, CollectionJobCommonParams, PROFILING_FAILED);
    collectionjobComnCfg_->devId = devIndexId_;
    collectionjobComnCfg_->devIdOnHost = TaskRelationshipMgr::instance()->GetHostIdByDevId(devIndexId_);
    collectionjobComnCfg_->devIdFlush =
        TaskRelationshipMgr::instance()->GetFlushSuffixDevId(params_->job_id, devIndexId_);
    MSVP_MAKE_SHARED0_RET(collectionjobComnCfg_->params, analysis::dvvp::message::ProfileParams, PROFILING_FAILED);
    collectionjobComnCfg_->params = params;
    if (tmpResultDir_.length() != 0) {
        std::string dataDir = tmpResultDir_ + MSVP_SLASH + "data";
        int ret = analysis::dvvp::common::utils::Utils::CreateDir(dataDir);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Creating dir: %s err!", analysis::dvvp::common::utils::Utils::BaseName(dataDir).c_str());
            analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        }
    }    
    CreateCollectionJobArray();
    GetAndStoreStartTime(params_->host_profiling);
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::StartProf(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    status_.status = analysis::dvvp::message::ERR;
    status_.info = "Start prof failed";
    do {
        MSPROF_LOGI("JobDeviceSoc StartProf checking params");
        if (isStarted_ || params == nullptr || !(ParamValidation::instance()->CheckProfilingParams(params))) {
            MSPROF_LOGE("[JobDeviceSoc::StartProf]Failed to check params");
            status_.info = "Start flag is true or parmas is invalid";
            break;
        }
        int ret = StartProfHandle(params);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[JobDeviceSoc::StartProf]Failed to StartProfParmasAdapt, devIndexId: %d", devIndexId_);
            status_.info = "Start profiling, parmas handle failed";
            break;
        }
        if (ProfChannelManager::instance()->Init() != PROFILING_SUCCESS) {
            MSPROF_LOGE("[JobDeviceSoc::StartProf]Failed to init channel poll");
            status_.info = "Init prof channel manager failed";
            break;
        }
        if (!(params_->host_profiling)
            && analysis::dvvp::driver::DrvChannelsMgr::instance()->GetAllChannels(devIndexId_) != PROFILING_SUCCESS) {
            MSPROF_LOGE("[JobDeviceSoc::StartProf]Failed to GetAllChannels, devIndexId: %d", devIndexId_);
            status_.info = "Get all prof channels failed";
            break;
        }
        MSVP_MAKE_SHARED0_BREAK(collectionjobComnCfg_->jobCtx, analysis::dvvp::message::JobContext);
        collectionjobComnCfg_->jobCtx->dev_id = std::to_string(collectionjobComnCfg_->devIdFlush);
        collectionjobComnCfg_->jobCtx->job_id = params_->job_id;
        collectionjobComnCfg_->tmpResultDir = params->result_dir;
        ret = ParsePmuConfig(CreatePmuEventConfig(params, devIndexId_));
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[JobDeviceSoc::StartProf]Failed to ParsePmuConfig, devIndexId: %d", devIndexId_);
            status_.info = "Parse pmu config failed";
            break;
        }
        ret = RegisterCollectionJobs();
        if (ret != PROFILING_SUCCESS) {
            status_.info = "Check hbm events failed";
            break;
        }
        status_.status = analysis::dvvp::message::SUCCESS;
        isStarted_ = true;
        return PROFILING_SUCCESS;
    } while (0);

    return PROFILING_FAILED;
}

int JobDeviceSoc::ParsePmuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    int ret = ParseTsCpuConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseAiCoreConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseControlCpuConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseLlcConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseDdrCpuConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseAivConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = ParseHbmConfig(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseTsCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->tsCPUEvents.size() > 0) {
        if (!ParamValidation::instance()->CheckTsCpuEventIsValid(cfg->tsCPUEvents)) {
            MSPROF_LOGE("[JobDeviceSoc::ParseTsCpuConfig]tsCpuEvent is not valid!");
            return PROFILING_FAILED;
        }
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->tsCPUEvents;
        CollectionJobV_[TS_CPU_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        MSPROF_LOGI("tsCpuEvent:%s", Utils::GetEventsStr(cfg->tsCPUEvents).c_str());
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseHbmConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->hbmEvents.size() > 0) {
        if (!ParamValidation::instance()->CheckHbmEventsIsValid(cfg->hbmEvents)) {
            MSPROF_LOGE("[JobDeviceSoc::ParseHbmConfig]hbmEvents is not valid!");
            return PROFILING_FAILED;
        }
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->hbmEvents;
        CollectionJobV_[HBM_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        MSPROF_LOGI("hbmEvents:%s", Utils::GetEventsStr(cfg->hbmEvents).c_str());
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseAiCoreConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    MSPROF_LOGI("aiCoreEvents:%s", Utils::GetEventsStr(cfg->aiCoreEvents).c_str());
    MSPROF_LOGI("aiCoreIdSize:%d", cfg->aiCoreEventsCoreIds.size());
    if (cfg->aiCoreEvents.size() > 0 &&
        !ParamValidation::instance()->CheckAiCoreEventsIsValid(cfg->aiCoreEvents)) {
        MSPROF_LOGE("[JobDeviceSoc::ParseAiCoreConfig]aiCoreEvent is not valid!");
        return PROFILING_FAILED;
    }
    if (cfg->aiCoreEventsCoreIds.size() > 0
        && !ParamValidation::instance()->CheckAiCoreEventCoresIsValid(cfg->aiCoreEventsCoreIds)) {
        MSPROF_LOGE("[JobDeviceSoc::ParseAiCoreConfig]aiCoreEventCores is not valid!");
        return PROFILING_FAILED;
    }
    if (cfg->aiCoreEventsCoreIds.size() > 0) {
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->aiCoreEvents;
        SHARED_PTR_ALIA<std::vector<int>> cores;
        MSVP_MAKE_SHARED0_RET(cores, std::vector<int>, PROFILING_FAILED);
        *cores = cfg->aiCoreEventsCoreIds;
        CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = cores;
        CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = cores;
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.events = events;
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.cores = cores;
    }

    if (CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg != nullptr &&
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.cores == nullptr) {
            MSVP_MAKE_SHARED0_RET(CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.cores,
                std::vector<int>, PROFILING_FAILED);
    }

    if (CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg != nullptr &&
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.events == nullptr) {
            MSVP_MAKE_SHARED0_RET(CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.events,
                std::vector<std::string>, PROFILING_FAILED);
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseAivConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->aivEvents.size() > 0 &&
        !ParamValidation::instance()->CheckAivEventsIsValid(cfg->aivEvents)) {
        MSPROF_LOGE("[JobDeviceSoc::ParseAivConfig]aivEvents is not valid!");
        return PROFILING_FAILED;
    }
    if (cfg->aivEventsCoreIds.size() > 0
        && !ParamValidation::instance()->CheckAivEventCoresIsValid(cfg->aivEventsCoreIds)) {
        MSPROF_LOGE("[JobDeviceSoc::ParseAivConfig]aivEventsCoreIds is not valid!");
        return PROFILING_FAILED;
    }
    if (cfg->aivEventsCoreIds.size() > 0) {
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->aivEvents;
        SHARED_PTR_ALIA<std::vector<int>> cores;
        MSVP_MAKE_SHARED0_RET(cores, std::vector<int>, PROFILING_FAILED);
        *cores = cfg->aivEventsCoreIds;
        CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = cores;
        CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
        CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = cores;
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivEvents = events;
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivCores = cores;
    }

    if (CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg != nullptr &&
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivCores == nullptr) {
            MSVP_MAKE_SHARED0_RET(CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivCores,
                std::vector<int>, PROFILING_FAILED);
    }

    if (CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg != nullptr &&
        CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivEvents == nullptr) {
            MSVP_MAKE_SHARED0_RET(CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg->jobParams.aivEvents,
                std::vector<std::string>, PROFILING_FAILED);
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseControlCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->ctrlCPUEvents.size() > 0) {
        if (!ParamValidation::instance()->CheckCtrlCpuEventIsValid(cfg->ctrlCPUEvents)) {
            MSPROF_LOGE("[JobDeviceSoc::ParseControlCpuConfig]ctrlCpuEvent is not valid!");
            return PROFILING_FAILED;
        }
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->ctrlCPUEvents;
        CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].jobCfg->jobParams.events = events;
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseDdrCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->ddrEvents.size() > 0) {
        if (!ParamValidation::instance()->CheckDdrEventsIsValid(cfg->ddrEvents)) {
            MSPROF_LOGE("[JobDeviceSoc::ParseDdrCpuConfig]ddrEvent is not valid!");
            return PROFILING_FAILED;
        }
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->ddrEvents;
        CollectionJobV_[DDR_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::ParseLlcConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg)
{
    if (cfg->llcEvents.size() > 0) {
        UtilsStringBuilder<std::string> builder;
        std::string eventStr = builder.Join(cfg->llcEvents, ",");
        if (!ParamValidation::instance()->CheckLlcEventsIsValid(eventStr)) {
            MSPROF_LOGE("[JobDeviceSoc::ParseLlcConfig]llcEvent is not valid!");
            return PROFILING_FAILED;
        }
        SHARED_PTR_ALIA<std::vector<std::string>> events;
        MSVP_MAKE_SHARED0_RET(events, std::vector<std::string>, PROFILING_FAILED);
        *events = cfg->llcEvents;
        CollectionJobV_[LLC_DRV_COLLECTION_JOB].jobCfg->jobParams.events = events;
    }
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::StopProf(void)
{
    MSPROF_LOGI("Stop profiling begin");
    if (!isStarted_ || collectionjobComnCfg_ == nullptr) {
        status_.status = analysis::dvvp::message::ERR;
        MSPROF_LOGE("Stop profiling failed");
        return PROFILING_FAILED;
    }
    UnRegisterCollectionJobs();
    collectionjobComnCfg_->jobCtx.reset();
    status_.status = analysis::dvvp::message::SUCCESS;
    MSPROF_LOGI("Stop profiling success");
    return PROFILING_SUCCESS;
}

int JobDeviceSoc::RegisterCollectionJobs() const
{
    MSPROF_LOGI("Start to register collction job:%s", collectionjobComnCfg_->params->job_id.c_str());
    int registerCnt = 0;
    std::vector<int> registered;
    for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
        MSPROF_LOGD("Collect Start jobId %d ", cnt);
        int ret = CollectionJobV_[cnt].collectionJob->Init(CollectionJobV_[cnt].jobCfg);
        if (ret == PROFILING_SUCCESS) {
            MSPROF_LOGD("[JobDeviceSoc]Collection Job %d Register", cnt);
            ret = CollectionRegisterMgr::instance()->CollectionJobRegisterAndRun(
                collectionjobComnCfg_->devId, CollectionJobV_[cnt].jobTag, CollectionJobV_[cnt].collectionJob);
        }
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGD("[JobDeviceSoc]Collection Job %d No Run; Total: %d", CollectionJobV_[cnt].jobTag, registerCnt);
            registerCnt++;
        } else {
            registered.push_back(cnt);
        }
    }
    UtilsStringBuilder<int> intBuilder;
    MSPROF_LOGI("Total count of job registered: %s", intBuilder.Join(registered, ",").c_str());
    return PROFILING_SUCCESS;
}

void JobDeviceSoc::UnRegisterCollectionJobs()
{
    do {
        for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
            int retn = CollectionRegisterMgr::instance()->CollectionJobUnregisterAndStop(
                collectionjobComnCfg_->devId, CollectionJobV_[cnt].jobTag);
            CollectionJobV_[cnt].jobCfg->jobParams.events.reset();
            CollectionJobV_[cnt].jobCfg->jobParams.cores.reset();
            if (retn != PROFILING_SUCCESS) {
                MSPROF_LOGD("Device %d Collection Job %d Unregister", collectionjobComnCfg_->devIdOnHost,
                            CollectionJobV_[cnt].jobTag);
            }
        }
        ProfChannelManager::instance()->UnInit();
        std::string perfDataDir =
            Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(collectionjobComnCfg_->devId);
        MSPROF_LOGI("Removing collected perf data: \"%s\"", Utils::BaseName(perfDataDir).c_str());
        analysis::dvvp::common::utils::Utils::RemoveDir(perfDataDir);
    } while (0);
}

void JobDeviceSoc::CreateCollectionJobArray()
{
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[DDR_DRV_COLLECTION_JOB].collectionJob, ProfDdrJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HBM_DRV_COLLECTION_JOB].collectionJob, ProfHbmJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[DVPP_COLLECTION_JOB].collectionJob, ProfDvppJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[PCIE_DRV_COLLECTION_JOB].collectionJob, ProfPcieJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[NIC_COLLECTION_JOB].collectionJob, ProfNicJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HCCS_DRV_COLLECTION_JOB].collectionJob, ProfHccsJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[LLC_DRV_COLLECTION_JOB].collectionJob, ProfLlcJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[ROCE_DRV_COLLECTION_JOB].collectionJob, ProfRoceJob);
    // for ts
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[TS_CPU_DRV_COLLECTION_JOB].collectionJob, ProfTscpuJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_TS_TRACK_DRV_COLLECTION_JOB].collectionJob, ProfAivTsTrackJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].collectionJob, ProfAicoreJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].collectionJob, ProfAicoreTaskBasedJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].collectionJob, ProfAivJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[TS_TRACK_DRV_COLLECTION_JOB].collectionJob, ProfTsTrackJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].collectionJob, ProfAivTaskBasedJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_HWTS_LOG_COLLECTION_JOB].collectionJob, ProfAivHwtsLogJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[FMK_COLLECTION_JOB].collectionJob, ProfFmkJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[L2_CACHE_TASK_COLLECTION_JOB].collectionJob, ProfL2CacheTaskJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[STARS_SOC_LOG_COLLECTION_JOB].collectionJob, ProfStarsSocLogJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[STARS_BLOCK_LOG_COLLECTION_JOB].collectionJob, ProfStarsBlockLogJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HWTS_LOG_COLLECTION_JOB].collectionJob, ProfHwtsLogJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[STARS_SOC_PROFILE_COLLECTION_JOB].collectionJob, ProfStarsSocProfileJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].collectionJob, ProfFftsProfileJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[BIU_COLLECTION_JOB].collectionJob, ProfBiuPerfJob);
    // for system
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].collectionJob, ProfCtrlcpuJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSSTAT_PROC_COLLECTION_JOB].collectionJob, ProfSysStatJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSMEM_PROC_COLLECTION_JOB].collectionJob, ProfSysMemJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[ALLPID_PROC_COLLECTION_JOB].collectionJob, ProfAllPidsJob);
    // for host system
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_SYSCALLS_COLLECTION_JOB].collectionJob, ProfHostSysCallsJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_PTHREAD_COLLECTION_JOB].collectionJob, ProfHostPthreadJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_DISKIO_COLLECTION_JOB].collectionJob, ProfHostDiskJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_CPU_COLLECTION_JOB].collectionJob, ProfHostCpuJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_MEM_COLLECTION_JOB].collectionJob, ProfHostMemJob);
    MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HOST_NETWORK_COLLECTION_JOB].collectionJob, ProfHostNetworkJob);

    for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[cnt].jobCfg, CollectionJobCfg);
        CollectionJobV_[cnt].jobTag = static_cast<ProfCollectionJobE>(cnt);
        CollectionJobV_[cnt].jobCfg->jobParams.jobTag = static_cast<ProfCollectionJobE>(cnt);
        if (COLLECTION_JOB_FILENAME[cnt].size() > 0) {
            CollectionJobV_[cnt].jobCfg->jobParams.dataPath = tmpResultDir_ + MSVP_SLASH + COLLECTION_JOB_FILENAME[cnt];
        }
        CollectionJobV_[cnt].jobCfg->comParams = collectionjobComnCfg_;
    }
}

int JobDeviceSoc::SendData(const std::string &fileName, const std::string &data)
{
    if (params_->host_profiling) {
        return PROFILING_SUCCESS;
    }
    if (data.empty()) {
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->dev_id = std::to_string(
        TaskRelationshipMgr::instance()->GetFlushSuffixDevId(params_->job_id, devIndexId_));
    jobCtx->job_id = params_->job_id;

    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
            analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(params_->job_id, data,
        fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
    }
    return ret;
}

std::string JobDeviceSoc::GenerateFileName(const std::string &fileName)
{
    std::string ret = fileName + "." + std::to_string(collectionjobComnCfg_->devIdFlush);
    return ret;
}
}}}
