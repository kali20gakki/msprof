/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: the job class.
 * Create: 2019.05.16
 */
#include "prof_job.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "prof_channel_manager.h"
#include "proto/profiler.pb.h"
#include "securec.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "platform/platform.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::MsprofErrMgr;

ProfDrvJob::ProfDrvJob()
{
}
ProfDrvJob::~ProfDrvJob()
{
}

void ProfDrvJob::AddReader(const std::string &key, int devId, AI_DRV_CHANNEL channelId, const std::string &filePath)
{
    std::string relativePath;
    (void)analysis::dvvp::common::utils::Utils::RelativePath(filePath,
        collectionJobCfg_->comParams->tmpResultDir,
        relativePath);

    SHARED_PTR_ALIA<ChannelReader> reader;
    MSVP_MAKE_SHARED4_VOID(reader, ChannelReader, devId, channelId,
        relativePath, collectionJobCfg_->comParams->jobCtx);
    int ret = reader->Init();
    if (ret != PROFILING_SUCCESS) {
        return;
    }
    SHARED_PTR_ALIA<ChannelPoll> poll = ProfChannelManager::instance()->GetChannelPoller();
    if (poll != nullptr) {
        (void)poll->AddReader(devId, channelId, reader);
    } else {
        MSPROF_LOGI("ProfDrvJob AddReader failed, key:%s, devId:%d, channel:%d, filepath:%s",
                    key.c_str(), devId, channelId, Utils::BaseName(filePath).c_str());
    }
}

void ProfDrvJob::RemoveReader(const std::string &key, int devId, AI_DRV_CHANNEL channelId)
{
    MSPROF_LOGI("ProfDrvJob RemoveReader, key:%s, devId:%d, channel:%d", key.c_str(), devId, channelId);
    SHARED_PTR_ALIA<ChannelPoll> poll = ProfChannelManager::instance()->GetChannelPoller();
    if (poll != nullptr) {
        (void)poll->RemoveReader(devId, channelId);
    }
}

unsigned int ProfDrvJob::GetEventSize(const std::vector<std::string> &events)
{
    unsigned int eventSize = 0;
    for (size_t i = 0; i < events.size(); i++) {
        if ((events[i].compare("read") == 0) || (events[i].compare("write") == 0)) {
            eventSize++;
        }
    }
    return eventSize;
}

std::string ProfDrvJob::GetEventsStr(const std::vector<std::string> &events,
    const std::string &separator /* = "," */)
{
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;

    return builder.Join(events, separator);
}

std::string ProfDrvJob::GenerateFileName(const std::string &fileName, int devId)
{
    std::stringstream ssProfDataFilePath;

    ssProfDataFilePath << fileName;
    ssProfDataFilePath << ".";
    ssProfDataFilePath << devId;

    return ssProfDataFilePath.str();
}

std::string ProfDrvJob::BindFileWithChannel(const std::string &fileName) const
{
    std::stringstream ssProfDataFilePath;

    ssProfDataFilePath << fileName;

    return ssProfDataFilePath.str();
}

ProfTscpuJob::ProfTscpuJob()
{
}
ProfTscpuJob::~ProfTscpuJob()
{
}

int ProfTscpuJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfTscpuJob::Process()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_TS_CPU)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_TS_CPU);
        return PROFILING_SUCCESS;
    }

    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    MSPROF_LOGI("Begin to start profiling ts cpu, events:%s", eventsStr.c_str());

    int tsCpuPeriod = 10;
    if (collectionJobCfg_->comParams->params->cpu_sampling_interval > 0) {
        tsCpuPeriod = collectionJobCfg_->comParams->params->cpu_sampling_interval;
    }

    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_TS_CPU, filePath);

    DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = collectionJobCfg_->comParams->devId;
    drvPeripheralProfileCfg.profChannel = PROF_CHANNEL_TS_CPU;
    drvPeripheralProfileCfg.profSamplePeriod = tsCpuPeriod;  // int prof_sample_period,
    drvPeripheralProfileCfg.profDataFilePath = "";

    int ret = DrvTscpuStart(drvPeripheralProfileCfg,
                            *collectionJobCfg_->jobParams.events);

    MSPROF_LOGI("start profiling ts cpu, events:%s, ret=%d", eventsStr.c_str(), ret);
    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfTscpuJob::Uninit()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_TS_CPU)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_TS_CPU);
        return PROFILING_SUCCESS;
    }

    std::string eventsStr = GetEventsStr(*(collectionJobCfg_->jobParams.events));

    int ret = DrvStop(collectionJobCfg_->comParams->devId, PROF_CHANNEL_TS_CPU);

    MSPROF_LOGI("stop profiling ts cpu, events:%s, ret=%d", eventsStr.c_str(), ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_TS_CPU);
    collectionJobCfg_->jobParams.events.reset();
    return PROFILING_SUCCESS;
}

ProfTsTrackJob::ProfTsTrackJob() : channelId_(PROF_CHANNEL_TS_FW)
{
}
ProfTsTrackJob::~ProfTsTrackJob()
{
}

int ProfTsTrackJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_COMMON_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->ts_task_track.compare("on") != 0 &&
        cfg->comParams->params->ts_cpu_usage.compare("on") != 0 &&
        cfg->comParams->params->ai_core_status.compare("on") != 0 &&
        cfg->comParams->params->ts_timeline.compare("on") != 0 &&
        cfg->comParams->params->ts_keypoint.compare("on") != 0 &&
        cfg->comParams->params->ts_memcpy.compare("on") != 0) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfTsTrackJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    int cpuProfilingInterval = 10;
    if (collectionJobCfg_->comParams->params->cpu_sampling_interval > 0) {
        cpuProfilingInterval = collectionJobCfg_->comParams->params->cpu_sampling_interval;
    }
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId, channelId_);
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("Begin to start profiling ts track, devId: %d", collectionJobCfg_->comParams->devId);
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        channelId_, filePath);
    DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = collectionJobCfg_->comParams->devId;
    drvPeripheralProfileCfg.profChannel = channelId_;
    drvPeripheralProfileCfg.profSamplePeriod = cpuProfilingInterval;
    drvPeripheralProfileCfg.profDataFilePath = "";
    int ret = DrvTsFwStart(drvPeripheralProfileCfg, collectionJobCfg_->comParams->params);
    MSPROF_LOGI("start profiling ts track, ret=%d", ret);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfTsTrackJob]Process, DrvTsFwStart failed");
        MSPROF_INNER_ERROR("EK9999", "Process, DrvTsFwStart failed");
    }
    return ret;
}

int ProfTsTrackJob::Uninit()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);

    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    MSPROF_LOGI("stop profiling ts track data, ret=%d", ret);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfTsTrackJob]Uninit, DrvStop failed");
        MSPROF_INNER_ERROR("EK9999", "Uninit, DrvStop failed");
    }
    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);

    return ret;
}

ProfStarsSocLogJob::ProfStarsSocLogJob() : channelId_(PROF_CHANNEL_STARS_SOC_LOG)
{
}
ProfStarsSocLogJob::~ProfStarsSocLogJob()
{
}

int ProfStarsSocLogJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_COMMON_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfStarsSocLogJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);

    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId, channelId_);
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("Begin to start profiling stars soc log, devId: %d", collectionJobCfg_->comParams->devId);
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        channelId_, filePath);
    DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = collectionJobCfg_->comParams->devId;
    drvPeripheralProfileCfg.profChannel = channelId_;
    drvPeripheralProfileCfg.profSamplePeriod = 0;
    int ret = DrvStarsSocLogStart(drvPeripheralProfileCfg, collectionJobCfg_->comParams->params);
    MSPROF_LOGI("start profiling stars soc log, ret=%d", ret);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfStarsSocLogJob]Process, DrvStarsSocLogStart failed");
        MSPROF_INNER_ERROR("EK9999", "Process, DrvStarsSocLogStart failed");
    }
    return ret;
}

int ProfStarsSocLogJob::Uninit()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);

    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    MSPROF_LOGI("stop profiling stars soc log, ret=%d", ret);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfStarsSocLogJob]Uninit, DrvStop failed");
        MSPROF_INNER_ERROR("EK9999", "Uninit, DrvStop failed");
    }
    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);

    return ret;
}

ProfStarsBlockLogJob::ProfStarsBlockLogJob() : channelId_(PROF_CHANNEL_STARS_BLOCK_LOG)
{
}
ProfStarsBlockLogJob::~ProfStarsBlockLogJob()
{
}

int ProfStarsBlockLogJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_COMMON_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfStarsBlockLogJob::Process()
{
    /* reserved for 630 */
    return PROFILING_SUCCESS;
}

int ProfStarsBlockLogJob::Uninit()
{
    /* reserved for 630 */
    return PROFILING_SUCCESS;
}

ProfFftsProfileJob::ProfFftsProfileJob()
    : channelId_(PROF_CHANNEL_UNKNOWN), cfgMode_(0), aicMode_(0), aivMode_(0),
      aicPeriod_(DEFAULT_PERIOD_TIME), aivPeriod_(DEFAULT_PERIOD_TIME)
{
}
ProfFftsProfileJob::~ProfFftsProfileJob()
{
}

int ProfFftsProfileJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    cfgMode_ = 0;
    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->ai_core_profiling.compare("on") != 0 &&
        collectionJobCfg_->comParams->params->aiv_profiling.compare("on") != 0) {
        MSPROF_LOGI("Aicore not enable, devId:%d", collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }

    if (collectionJobCfg_->comParams->params->ai_core_profiling.compare("on") == 0) {
        cfgMode_ |= 1 << FFTS_PROF_MODE_AIC;
    }
    if (collectionJobCfg_->comParams->params->aiv_profiling.compare("on") == 0) {
        cfgMode_ |= 1 << FFTS_PROF_MODE_AIV;
    }

    if (collectionJobCfg_->comParams->params->aicore_sampling_interval > 0) {
        aicPeriod_ = collectionJobCfg_->comParams->params->aicore_sampling_interval;
    }
    if (collectionJobCfg_->comParams->params->aiv_sampling_interval > 0) {
        aivPeriod_ = collectionJobCfg_->comParams->params->aiv_sampling_interval;
    }

    if (collectionJobCfg_->comParams->params->ai_core_profiling_mode.compare("sample-based") == 0 &&
        collectionJobCfg_->comParams->params->aiv_profiling_mode.compare("sample-based") == 0) {
        aicMode_ = 1 << FFTS_PROF_TYPE_SAMPLE_BASE;
        aivMode_ = 1 << FFTS_PROF_TYPE_SAMPLE_BASE;
        channelId_ = PROF_CHANNEL_FFTS_PROFILIE_SAMPLE;
    } else {
        /* default mode - task based */
        aicMode_ = 1 << FFTS_PROF_TYPE_TASK_BASE;
        aivMode_ = 1 << FFTS_PROF_TYPE_TASK_BASE;
        channelId_ = PROF_CHANNEL_FFTS_PROFILIE_TASK;
    }
    // set sub task on
    aicMode_ = aicMode_ | (1 << FFTS_PROF_TYPE_SUBTASK);
    aivMode_ = aivMode_ | (1 << FFTS_PROF_TYPE_SUBTASK);

    MSPROF_LOGI("ffts profile init success, channelId_(0x%x), cfgMode(0x%x), aicMode(0x%x), aicPeriod(%u),"
        "aivMode(0x%x), aivPeriod(%u)", channelId_, cfgMode_, aicMode_, aicPeriod_, aivMode_, aivPeriod_);
    return PROFILING_SUCCESS;
}

int ProfFftsProfileJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId, channelId_);
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("Begin to start ffts profile buffer, devId: %d", collectionJobCfg_->comParams->devId);
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_, filePath);
    DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = collectionJobCfg_->comParams->devId;
    drvPeripheralProfileCfg.profChannel = channelId_;
    drvPeripheralProfileCfg.profSamplePeriod = aicPeriod_;
    drvPeripheralProfileCfg.profSamplePeriodHi = aivPeriod_;
    drvPeripheralProfileCfg.cfgMode = cfgMode_;
    drvPeripheralProfileCfg.aicMode = aicMode_;
    drvPeripheralProfileCfg.aivMode = aivMode_;
    int ret = DrvFftsProfileStart(drvPeripheralProfileCfg,
                                  *collectionJobCfg_->jobParams.cores,
                                  *collectionJobCfg_->jobParams.events,
                                  *collectionJobCfg_->jobParams.aivCores,
                                  *collectionJobCfg_->jobParams.aivEvents);
    MSPROF_LOGI("start ffts profile buffer, ret=%d", ret);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfFftsProfileJob]Process, DrvFftsProfileStart failed");
        MSPROF_INNER_ERROR("EK9999", "Process, DrvFftsProfileStart failed");
    }
    return ret;
}

int ProfFftsProfileJob::Uninit()
{
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProfFftsProfileJob]Uninit failed, ret:%d", ret);
        MSPROF_INNER_ERROR("EK9999", "Uninit failed, ret:%d", ret);
    }
    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);
    collectionJobCfg_->jobParams.events.reset();

    return PROFILING_SUCCESS;
}

ProfAivTsTrackJob::ProfAivTsTrackJob() {}

ProfAivTsTrackJob::~ProfAivTsTrackJob() {}

int ProfAivTsTrackJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_COMMON_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->ts_task_track.compare("on") != 0 &&
        cfg->comParams->params->ts_cpu_usage.compare("on") != 0 &&
        cfg->comParams->params->ai_core_status.compare("on") != 0 &&
        cfg->comParams->params->ts_timeline.compare("on") != 0) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    channelId_ = PROF_CHANNEL_AIV_TS_FW;
    collectionJobCfg_->comParams->params->ts_keypoint = "off";
    return PROFILING_SUCCESS;
}

ProfAicoreJob::ProfAicoreJob()
    : period_(DEFAULT_PERIOD_TIME), channelId_(PROF_CHANNEL_AI_CORE) // 10 is the default period
{
}
ProfAicoreJob::~ProfAicoreJob()
{
}


int ProfAicoreJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->ai_core_profiling.compare("on") != 0 ||
        collectionJobCfg_->comParams->params->ai_core_profiling_mode.compare("sample-based") != 0) {
        MSPROF_LOGI("Aicore sample-based not enable, devId:%d", collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }
    taskType_ = PROF_AICORE_SAMPLE;
    if (collectionJobCfg_->comParams->params->aicore_sampling_interval > 0) {
        period_ = collectionJobCfg_->comParams->params->aicore_sampling_interval;
    }
    return PROFILING_SUCCESS;
}

int ProfAicoreJob::Process()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId_:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    std::string coresStr = analysis::dvvp::common::utils::Utils::GetCoresStr(*collectionJobCfg_->jobParams.cores);
    MSPROF_LOGI("Begin to start profiling ai core, taskType:%s, events:%s, cores:%s",
        taskType_.c_str(), eventsStr.c_str(), coresStr.c_str());

    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_, filePath);

    DrvPeripheralProfileCfg drvPeripheralProfileCfg;
    drvPeripheralProfileCfg.profDeviceId = collectionJobCfg_->comParams->devId;
    drvPeripheralProfileCfg.profChannel = channelId_;
    drvPeripheralProfileCfg.profSamplePeriod = period_;  // int prof_sample_period,
    std::string fileName = GenerateFileName(filePath,
        collectionJobCfg_->comParams->devIdOnHost);
    drvPeripheralProfileCfg.profDataFilePath = "";

    int ret = DrvAicoreStart(drvPeripheralProfileCfg,
                             *collectionJobCfg_->jobParams.cores,  // const std::vector<int>& prof_cores,
                             *collectionJobCfg_->jobParams.events);  // std::vector<std::string> &prof_events,

    MSPROF_LOGI("start profiling ai core, taskType:%s, events:%s, cores:%s, ret=%d", taskType_.c_str(),
        eventsStr.c_str(), coresStr.c_str(), ret);

    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfAicoreJob::Uninit()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);

    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("begin to stop profiling %s, events:%s", taskType_.c_str(), eventsStr.c_str());

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);

    MSPROF_LOGI("stop profiling %s, events:%s, ret=%d", taskType_.c_str(), eventsStr.c_str(), ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);
    collectionJobCfg_->jobParams.cores.reset();
    collectionJobCfg_->jobParams.events.reset();
    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

ProfAivJob::ProfAivJob()
{
}
ProfAivJob::~ProfAivJob()
{
}


int ProfAivJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->aiv_profiling.compare("on") != 0 ||
        collectionJobCfg_->comParams->params->aiv_profiling_mode.compare("sample-based") != 0) {
        MSPROF_LOGI("Aivector core sample-based not enable, devId:%d", collectionJobCfg_->comParams->devId);
        return PROFILING_FAILED;
    }
    taskType_ = PROF_AIV_SAMPLE;
    period_ = DEFAULT_PERIOD_TIME; // 10 is the default period
    channelId_ = PROF_CHANNEL_AIV_CORE;
    if (collectionJobCfg_->comParams->params->aiv_sampling_interval > 0) {
        period_ = collectionJobCfg_->comParams->params->aiv_sampling_interval;
    }
    return PROFILING_SUCCESS;
}


ProfAicoreTaskBasedJob::ProfAicoreTaskBasedJob()
    : channelId_(PROF_CHANNEL_AI_CORE)
{
}
ProfAicoreTaskBasedJob::~ProfAicoreTaskBasedJob()
{
}

int ProfAicoreTaskBasedJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    MSPROF_LOGI("ProfAicoreTaskBasedJob init");
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    if (cfg->comParams->params->ai_core_profiling.compare("on") != 0 ||
        cfg->comParams->params->ai_core_profiling_mode.compare("task-based") != 0) {
        MSPROF_LOGI("Aicore task-based not enable, devId:%d", cfg->comParams->devId);
        return PROFILING_FAILED;
    }
    taskType_ = PROF_AICORE_TASK;
    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfAicoreTaskBasedJob::Process()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    MSPROF_LOGI("Begin to start profiling AicoreTaskBase, taskType:%s, events:%s",
        taskType_.c_str(), eventsStr.c_str());

    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_, filePath);
    int ret = DrvAicoreTaskBasedStart(collectionJobCfg_->comParams->devId, channelId_,
        *collectionJobCfg_->jobParams.events);

    MSPROF_LOGI("start profiling AicoreTaskBase, taskType:%s, events:%s, ret=%d",
        taskType_.c_str(), eventsStr.c_str(), ret);

    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfAicoreTaskBasedJob::Uninit()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    MSPROF_LOGI("stop profiling AicoreTaskBase, taskType:%s, events:%s, ret=%d",
                taskType_.c_str(), eventsStr.c_str(), ret);
    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);
    collectionJobCfg_->jobParams.events.reset();

    return PROFILING_SUCCESS;
}

ProfAivTaskBasedJob::ProfAivTaskBasedJob()
{
}
ProfAivTaskBasedJob::~ProfAivTaskBasedJob()
{
}

int ProfAivTaskBasedJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    MSPROF_LOGI("ProfAivTaskBasedJob init");
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    if (cfg->comParams->params->aiv_profiling.compare("on") != 0 ||
        cfg->comParams->params->aiv_profiling_mode.compare("task-based") != 0) {
        MSPROF_LOGI("Aivector core task-based not enable, devId:%d", cfg->comParams->devId);
        return PROFILING_FAILED;
    }
    taskType_ = PROF_AIV_TASK;
    channelId_ = PROF_CHANNEL_AIV_CORE;
    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

ProfCtrlcpuJob::ProfCtrlcpuJob()
    : ctrlcpuProcess_(MSVP_MMPROCESS)
{
}

ProfCtrlcpuJob::~ProfCtrlcpuJob()
{
}

int ProfCtrlcpuJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_EVENT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (!Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("Not in device Side, aiCtrlcpu Profiling not enabled");
        return PROFILING_FAILED;
    }
    using namespace Analysis::Dvvp::Common::Config;
    collectionJobCfg_ = cfg;
    collectionJobCfg_->comParams->tmpResultDir =
        ConfigManager::instance()->GetPerfDataDir(collectionJobCfg_->comParams->devId);
    int ret = analysis::dvvp::common::utils::Utils::CreateDir(collectionJobCfg_->comParams->tmpResultDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Creating dir failed: %s", Utils::BaseName(collectionJobCfg_->comParams->tmpResultDir).c_str());
        MSPROF_INNER_ERROR("EK9999", "Creating dir failed: %s",
            Utils::BaseName(collectionJobCfg_->comParams->tmpResultDir).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return ret;
    }
    // perf script
    static const unsigned int PERF_DATA_BUF_SIZE_M = 262144 + 1; // 262144 + 1, 256K Byte + '\0'
    MSVP_MAKE_SHARED4_RET(perfExtraTask_, PerfExtraTask, PERF_DATA_BUF_SIZE_M,
        collectionJobCfg_->comParams->tmpResultDir, collectionJobCfg_->comParams->jobCtx,
        collectionJobCfg_->comParams->params, PROFILING_FAILED);
    return PROFILING_SUCCESS;
}

int ProfCtrlcpuJob::Process()
{
    std::string profCtrlcpuCmd;
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (collectionJobCfg_->jobParams.events != nullptr &&
        collectionJobCfg_->jobParams.events->size()) {
        GetCollectCtrlCpuEventCmd(*(collectionJobCfg_->jobParams.events), profCtrlcpuCmd);
    }

    if (profCtrlcpuCmd.size() > 0) {
        MSPROF_LOGI("start profiling ctrl cpu, cmd=%s", profCtrlcpuCmd.c_str());

        std::vector<std::string> params =
            analysis::dvvp::common::utils::Utils::Split(profCtrlcpuCmd.c_str());
        if (params.empty()) {
            MSPROF_LOGE("ProfCtrlcpuJob params empty");
            MSPROF_INNER_ERROR("EK9999", "ProfCtrlcpuJob params empty");
            return PROFILING_FAILED;
        }
        MSPROF_LOGI("Begin to start profiling ctrl cpu");

        std::string cmd = params[0];
        std::vector<std::string> argsV;
        std::vector<std::string> envsV;
        if (params.size() > 1) {
            argsV.assign(params.begin() + 1, params.end());
        }
        envsV.push_back("PATH=/usr/bin:/usr/sbin:/var");

        int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
        ctrlcpuProcess_ = MSVP_MMPROCESS;
        std::string perfLog =
            ConfigManager::instance()->GetPerfDataDir(collectionJobCfg_->comParams->devId) + MSVP_SLASH + "perf.log";
        analysis::dvvp::common::utils::ExecCmdParams execCmdParams(cmd, true, perfLog);
        int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, ctrlcpuProcess_);
        MSPROF_LOGI("start profiling ctrl cpu, pid=%u, ret=%d",
                    (unsigned int)ctrlcpuProcess_, ret);
        FUNRET_CHECK_FAIL_RET_VALUE(ret, PROFILING_SUCCESS, ret);
        if (perfExtraTask_ && perfExtraTask_->Init() == PROFILING_SUCCESS) {
            // perf script start
            perfExtraTask_->SetJobCtx(collectionJobCfg_->comParams->jobCtx);
            perfExtraTask_->SetThreadName(analysis::dvvp::common::config::MSVP_COLLECT_PERF_SCRIPT_THREAD_NAME);
            ret = perfExtraTask_->Start();
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("[ProfCtrlcpuJob::Process]Failed to start perfExtraTask_ thread, ret=%d", ret);
                MSPROF_INNER_ERROR("EK9999", "Failed to start perfExtraTask_ thread, ret=%d", ret);
                return ret;
            }
        }
    }
    return PROFILING_SUCCESS;
}

int ProfCtrlcpuJob::GetCollectCtrlCpuEventCmd(const std::vector<std::string> &events,
    std::string &profCtrlcpuCmd)
{
    const int CHANGE_FORM_MS_TO_S = 1000;
    if (events.empty() || !ParamValidation::instance()->CheckCtrlCpuEventIsValid(events)) {
        return PROFILING_FAILED;
    }
    std::string cpuDataFile;
    int ret = PrepareDataDir(cpuDataFile);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    std::stringstream ssCombined;
    ssCombined << "{";
    for (size_t jj = 0; jj < events.size(); ++jj) {
        if (jj != 0) {
            ssCombined << ",";
        }
        // replace "0x" to "r" at beginning
        std::string event = events[jj];
        const int EVENT_REPLACE_LEN = 2;
        ssCombined << event.replace(0, EVENT_REPLACE_LEN, "r");
    }
    ssCombined << "}";
    std::stringstream ssPerfCmd;

    int cpuProfilingInterval = 10; // Profile deltas every 10ms defaultly
    if (collectionJobCfg_->comParams->params->cpu_sampling_interval > 0) {
        cpuProfilingInterval = collectionJobCfg_->comParams->params->cpu_sampling_interval;
    }

    ssPerfCmd << "sudo perf record -o ";
    ssPerfCmd << cpuDataFile;
    ssPerfCmd << " -F ";
    ssPerfCmd << analysis::dvvp::common::utils::Utils::ConvertIntToStr((CHANGE_FORM_MS_TO_S / cpuProfilingInterval));
    ssPerfCmd << " -N -B -T -g -e'";
    ssPerfCmd << ssCombined.str();
    ssPerfCmd << "' ";
    ssPerfCmd << "-a";
    // split file
    ssPerfCmd << " --switch-output=10s";

    profCtrlcpuCmd = ssPerfCmd.str();

    return PROFILING_SUCCESS;
}

int ProfCtrlcpuJob::PrepareDataDir(std::string &cpuDataFile)
{
    std::string perfDataDir =
        Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(collectionJobCfg_->comParams->devId);
    int ret = analysis::dvvp::common::utils::Utils::CreateDir(perfDataDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Creating dir: %s err!", Utils::BaseName(perfDataDir).c_str());
        MSPROF_INNER_ERROR("EK9999", "Creating dir: %s err!", Utils::BaseName(perfDataDir).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return PROFILING_FAILED;
    }
    std::vector<std::string> ctrlCpuDataPathVec;
    ctrlCpuDataPathVec.push_back(perfDataDir);
    ctrlCpuDataPathVec.push_back("ai_ctrl_cpu.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(ctrlCpuDataPathVec);
    cpuDataFile = dataPath + "." +
        std::to_string(collectionJobCfg_->comParams->devIdOnHost);
    std::ofstream cpuFile(cpuDataFile);
    if (cpuFile.is_open()) {
        cpuFile.close();
    } else {
        MSPROF_LOGE("Failed to open %s, dev_id=%d", cpuDataFile.c_str(), collectionJobCfg_->comParams->devIdOnHost);
        MSPROF_INNER_ERROR("EK9999", "Failed to open %s, dev_id=%d",
            cpuDataFile.c_str(), collectionJobCfg_->comParams->devIdOnHost);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfCtrlcpuJob::Uninit()
{
    static const std::string ENV_PATH = "PATH=/usr/bin:/usr/sbin";
    std::vector<std::string> envV;
    envV.push_back(ENV_PATH);
    std::vector<std::string> argsV;
    argsV.push_back("pkill");
    argsV.push_back("-2");
    argsV.push_back("perf");

    mmProcess appProcess = MSVP_MMPROCESS;
    int exitCode = analysis::dvvp::common::utils::VALID_EXIT_CODE;
    static const std::string CMD = "sudo";
    analysis::dvvp::common::utils::ExecCmdParams execCmdParams(CMD, false, "");
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envV, exitCode, appProcess);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to kill process perf, ret=%d", ret);
        MSPROF_INNER_ERROR("EK9999", "Failed to kill process perf, ret=%d", ret);
    } else {
        MSPROF_LOGI("Succeeded to kill process perf, ret=%d, exitCode=%d", ret, exitCode);
    }

    if (ctrlcpuProcess_ > 0) {
        bool isExited = false;
        ret = analysis::dvvp::common::utils::Utils::WaitProcess(ctrlcpuProcess_, isExited,
                                                                exitCode, true);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to wait process %u, ret=%d",
                        (unsigned int)ctrlcpuProcess_, ret);
            MSPROF_INNER_ERROR("EK9999", "Failed to wait process %u, ret=%d", (unsigned int)ctrlcpuProcess_, ret);
        } else {
            MSPROF_LOGI("Process %u exited, exit code=%d",
                        (unsigned int)ctrlcpuProcess_, exitCode);
        }
        ctrlcpuProcess_ = 0;
        // perf script stop
        if (perfExtraTask_) {
            (void)perfExtraTask_->Stop();
            (void)perfExtraTask_->UnInit();
        }
    }

    if (collectionJobCfg_ != nullptr && collectionJobCfg_->jobParams.events != nullptr) {
        collectionJobCfg_->jobParams.events.reset();
    }
    return ret;
}

static unsigned long long ProfTimerJobCommonInit(const SHARED_PTR_ALIA<CollectionJobCfg> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> &upLoader,
    TimerHandlerTag timerTag)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    static const unsigned int profStatMemIntervalHundredMs = 100;  // 100 MS
    static const unsigned int profMsToNs = 1000000;  // 1000000 NS
    int sampleIntervalMs = profStatMemIntervalHundredMs;
    int profilingInterval = cfg->comParams->params->cpu_sampling_interval;

    if (timerTag == PROF_SYS_MEM) {
        profilingInterval = cfg->comParams->params->sys_sampling_interval;
    } else if (timerTag == PROF_ALL_PID) {
        profilingInterval = cfg->comParams->params->pid_sampling_interval;
    }

    if (profilingInterval > sampleIntervalMs) {
        sampleIntervalMs = profilingInterval;
    }

    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(cfg->comParams->params->job_id, upLoader);
    if (upLoader == nullptr) {
        MSPROF_LOGE("Failed to get devId(%d) upLoader, devIdOnHost : %d",
                    cfg->comParams->devId, cfg->comParams->devIdOnHost);
        MSPROF_INNER_ERROR("EK9999", "Failed to get devId(%d) upLoader, devIdOnHost : %d",
            cfg->comParams->devId, cfg->comParams->devIdOnHost);
        return 0;
    }
    MSPROF_LOGI("[ProfTimerJobCommonInit]devId:%d, devIdOnHost:%d, timerTag:%d, sampleIntervalMs:%d",
        cfg->comParams->devId, cfg->comParams->devIdOnHost, timerTag, sampleIntervalMs);

    return (static_cast<unsigned long long>(sampleIntervalMs) * profMsToNs);
}

ProfSysStatJob::ProfSysStatJob() : ProfSysInfoBase()
{
}

ProfSysStatJob::~ProfSysStatJob()
{
}

int ProfSysStatJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (!Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("Not in device Side, SysStat Profiling not enabled");
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->sys_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("sys_profiling not enabled");
        return PROFILING_FAILED;
    }
    sampleIntervalNs_ = ProfTimerJobCommonInit(collectionJobCfg_, upLoader_, PROF_SYS_STAT);
    if (sampleIntervalNs_ == 0) {
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int ProfSysStatJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    static const unsigned int PROC_SYS_STAT_BUF_SIZE = (1 << 13); // 1 << 13  means 8k

    std::string retFileName(PROF_SYS_CPU_USAGE_FILE);
    SHARED_PTR_ALIA<ProcStatFileHandler> statHandler;

    MSVP_MAKE_SHARED9_RET(statHandler, ProcStatFileHandler,
        PROF_SYS_STAT, collectionJobCfg_->comParams->devId, PROC_SYS_STAT_BUF_SIZE, sampleIntervalNs_,
        PROF_PROC_STAT, retFileName, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);

    if (statHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("statHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "statHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("statHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);
    TimerManager::instance()->StartProfTimer();
    TimerManager::instance()->RegisterProfTimerHandler(PROF_SYS_STAT, statHandler);
    return PROFILING_SUCCESS;
}

int ProfSysStatJob::Uninit()
{
    TimerManager::instance()->RemoveProfTimerHandler(PROF_SYS_STAT);
    TimerManager::instance()->StopProfTimer();
    upLoader_.reset();
    return PROFILING_SUCCESS;
}

ProfAllPidsJob::ProfAllPidsJob() : ProfSysInfoBase()
{
}

ProfAllPidsJob::~ProfAllPidsJob()
{
}

int ProfAllPidsJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (cfg == nullptr ||
        cfg->comParams == nullptr ||
        cfg->comParams->jobCtx == nullptr ||
        cfg->comParams->params == nullptr) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (!Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("Not in device Side, AllPids Profiling not enabled");
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->pid_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("pid_profiling not enabled");
        return PROFILING_FAILED;
    }
    sampleIntervalNs_ = ProfTimerJobCommonInit(collectionJobCfg_, upLoader_, PROF_ALL_PID);
    if (sampleIntervalNs_ == 0) {
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int ProfAllPidsJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    SHARED_PTR_ALIA<ProcAllPidsFileHandler> pidsHandler;
    MSVP_MAKE_SHARED6_RET(pidsHandler, ProcAllPidsFileHandler,
        PROF_ALL_PID, collectionJobCfg_->comParams->devId,
        sampleIntervalNs_, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);
    if (pidsHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("pidsHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "pidsHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("pidsHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);
    TimerManager::instance()->StartProfTimer();
    TimerManager::instance()->RegisterProfTimerHandler(PROF_ALL_PID, pidsHandler);
    return PROFILING_SUCCESS;
}

int ProfAllPidsJob::Uninit()
{
    TimerManager::instance()->RemoveProfTimerHandler(PROF_ALL_PID);
    TimerManager::instance()->StopProfTimer();
    upLoader_.reset();
    return PROFILING_SUCCESS;
}

ProfSysMemJob::ProfSysMemJob() : ProfSysInfoBase()
{
}

ProfSysMemJob::~ProfSysMemJob()
{
}

int ProfSysMemJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (!Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("Not in device Side, SysMem Profiling not enabled");
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->sys_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0 ||
        collectionJobCfg_->comParams->params->msprof.compare(
            analysis::dvvp::common::config::MSVP_PROF_ON) == 0) { // msprof does not collect mem
        MSPROF_LOGI("sys mem profiling not enabled");
        return PROFILING_FAILED;
    }
    sampleIntervalNs_ = ProfTimerJobCommonInit(collectionJobCfg_, upLoader_, PROF_SYS_MEM);
    if (sampleIntervalNs_ == 0) {
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int ProfSysMemJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    static const unsigned int PROC_SYS_MEM_BUF_SIZE = (1 << 15);  // 1 << 15  means 32k
    std::string retFileName(PROF_SYS_MEM_FILE);

    SHARED_PTR_ALIA<ProcMemFileHandler> memHandler;
    MSVP_MAKE_SHARED9_RET(memHandler, ProcMemFileHandler,
        PROF_SYS_MEM, collectionJobCfg_->comParams->devId, PROC_SYS_MEM_BUF_SIZE, sampleIntervalNs_,
        PROF_PROC_MEM, retFileName, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);
    if (memHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("memHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "memHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("memHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);
    TimerManager::instance()->StartProfTimer();
    TimerManager::instance()->RegisterProfTimerHandler(PROF_SYS_MEM, memHandler);
    return PROFILING_SUCCESS;
}

int ProfSysMemJob::Uninit()
{
    TimerManager::instance()->RemoveProfTimerHandler(PROF_SYS_MEM);
    TimerManager::instance()->StopProfTimer();
    upLoader_.reset();
    return PROFILING_SUCCESS;
}

ProfFmkJob::ProfFmkJob()
{
}
ProfFmkJob::~ProfFmkJob()
{
}

int ProfFmkJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfFmkJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (collectionJobCfg_->comParams->params->ts_fw_training.compare("on") != 0) {
        MSPROF_LOGI("ts_fw_training not enabled");
        return PROFILING_SUCCESS;
    }
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_FMK)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_FMK);
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("Begin to start profiling fmk log");

    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_FMK, filePath);
    int ret = DrvFmkDataStart(collectionJobCfg_->comParams->devId, PROF_CHANNEL_FMK);

    MSPROF_LOGI("start profiling fmk log, ret=%d", ret);
    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfFmkJob::Uninit()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);

    if (collectionJobCfg_->comParams->params->ts_fw_training.compare("on") != 0) {
        return PROFILING_SUCCESS;
    }
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_FMK)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_FMK);
        return PROFILING_SUCCESS;
    }

    int ret = DrvStop(collectionJobCfg_->comParams->devId, PROF_CHANNEL_FMK);

    MSPROF_LOGI("stop profiling fmk data, ret=%d", ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, PROF_CHANNEL_FMK);

    return PROFILING_SUCCESS;
}

ProfHwtsLogJob::ProfHwtsLogJob() : channelId_(PROF_CHANNEL_HWTS_LOG)
{
}

ProfHwtsLogJob::~ProfHwtsLogJob()
{
}

int ProfHwtsLogJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    if (cfg->comParams->params->hwts_log.compare("on") != 0) {
        MSPROF_LOGI("hwts_log not enabled");
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

int ProfHwtsLogJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);

    MSPROF_LOGI("[ProfHwtsLogJob]Process, hwts_log:%s, aiv_hwts_log:%s",
        collectionJobCfg_->comParams->params->hwts_log.c_str(),
        collectionJobCfg_->comParams->params->hwts_log1.c_str());

    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("Begin to start profiling hwts log");
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_, filePath);

    int ret = DrvHwtsLogStart(collectionJobCfg_->comParams->devId, channelId_);

    MSPROF_LOGI("start profiling hwts log, ret=%d", ret);
    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfHwtsLogJob::Uninit()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);

    MSPROF_LOGI("[ProfHwtsLogJob]Uninit, hwts_log:%s, aiv_hwts_log:%s",
        collectionJobCfg_->comParams->params->hwts_log.c_str(),
        collectionJobCfg_->comParams->params->hwts_log1.c_str());

    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            channelId_);
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("begin to stop profiling hwts_log data");

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);

    MSPROF_LOGI("stop profiling hwts_log data, ret=%d", ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);

    return PROFILING_SUCCESS;
}

ProfAivHwtsLogJob::ProfAivHwtsLogJob() {}

ProfAivHwtsLogJob::~ProfAivHwtsLogJob() {}

int ProfAivHwtsLogJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }

    if (cfg->comParams->params->hwts_log1.compare("on") != 0) {
        MSPROF_LOGI("aiv_hwts_log not enabled");
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    channelId_ = PROF_CHANNEL_AIV_HWTS_LOG;
    return PROFILING_SUCCESS;
}

ProfL2CacheTaskJob::ProfL2CacheTaskJob() {}

ProfL2CacheTaskJob::~ProfL2CacheTaskJob() {}

int ProfL2CacheTaskJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_COMMON_PARAM_RET(cfg, PROFILING_FAILED);
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;

    if (collectionJobCfg_->comParams->params->l2CacheTaskProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("ProfL2CacheTaskJob Not Enabled");
        return PROFILING_FAILED;
    }
    static const int L2_CACHE_TASK_EVENT_MAX_SIZE = 8;
    SHARED_PTR_ALIA<std::vector<std::string>> l2CacheTaskProfilingEvents;
    MSVP_MAKE_SHARED0_RET(l2CacheTaskProfilingEvents, std::vector<std::string>, PROFILING_FAILED);
    *l2CacheTaskProfilingEvents = analysis::dvvp::common::utils::Utils::Split(
        collectionJobCfg_->comParams->params->l2CacheTaskProfilingEvents, false, "", ",");
    bool ret = ParamValidation::instance()->CheckL2CacheEventsValid(*l2CacheTaskProfilingEvents);
    if (!ret || l2CacheTaskProfilingEvents->size() > L2_CACHE_TASK_EVENT_MAX_SIZE) {
        MSPROF_LOGE("ProfL2CacheTaskJob Exits Error Events Size %u", l2CacheTaskProfilingEvents->size());
        MSPROF_INNER_ERROR("EK9999", "ProfL2CacheTaskJob Exits Error Events Size %lu",
            l2CacheTaskProfilingEvents->size());
        return PROFILING_FAILED;
    }

    collectionJobCfg_->jobParams.events = l2CacheTaskProfilingEvents;
    return PROFILING_SUCCESS;
}

int ProfL2CacheTaskJob::Process()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_L2_CACHE)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_L2_CACHE);
        return PROFILING_SUCCESS;
    }

    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);
    MSPROF_LOGI("Begin to start profiling L2 Cache, events:%s", eventsStr.c_str());
    std::string filePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_L2_CACHE, filePath);
    int ret = DrvL2CacheTaskStart(
        collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_L2_CACHE,
        *collectionJobCfg_->jobParams.events);
    MSPROF_LOGI("start profiling L2 Cache, events:%s, ret=%d", eventsStr.c_str(), ret);

    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

int ProfL2CacheTaskJob::Uninit()
{
    CHECK_JOB_EVENT_PARAM_RET(collectionJobCfg_, PROFILING_SUCCESS);
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, PROF_CHANNEL_L2_CACHE)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            PROF_CHANNEL_L2_CACHE);
        return PROFILING_SUCCESS;
    }
    std::string eventsStr = GetEventsStr(*collectionJobCfg_->jobParams.events);

    int ret = DrvStop(collectionJobCfg_->comParams->devId, PROF_CHANNEL_L2_CACHE);

    MSPROF_LOGI("stop Profiling L2 Cache Task, events:%s, ret=%d", eventsStr.c_str(), ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId,
        PROF_CHANNEL_L2_CACHE);
    collectionJobCfg_->jobParams.events.reset();
    return PROFILING_SUCCESS;
}

PerfExtraTask::PerfExtraTask(unsigned int bufSize, const std::string& retDir,
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param)
    : isInited_(false),
      dataSize_(0),
      retDir_(retDir),
      buf_(bufSize),
      jobCtx_(jobCtx),
      param_(param)
{
}

PerfExtraTask::~PerfExtraTask()
{
}

int PerfExtraTask::Init()
{
    if (param_->host_profiling) {
        return PROFILING_FAILED;
    }

    if (isInited_) {
        MSPROF_LOGE("The PerfExtraTask is inited");
        MSPROF_INNER_ERROR("EK9999", "The PerfExtraTask is inited");
        return PROFILING_FAILED;
    }

    if (!buf_.Init()) {
        MSPROF_LOGE("Buf init failed");
        MSPROF_INNER_ERROR("EK9999", "Buf init failed");
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("PerfExtraTask init succ");
    isInited_ = true;

    return PROFILING_SUCCESS;
}

int PerfExtraTask::UnInit()
{
    if (!isInited_) {
        MSPROF_LOGI("PerfExtraTask is uninited");
        return PROFILING_SUCCESS;
    }

    buf_.Uninit();
    isInited_ = false;
    return PROFILING_SUCCESS;
}

void PerfExtraTask::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    while (!IsQuit()) {
        MSPROF_LOGI("PerfExtraTask running");
        analysis::dvvp::common::utils::Utils::UsleepInterupt(1000000); // 1000000 : sleep 1s
        PerfScriptTask();
    }

    PerfScriptTask();
    MSPROF_LOGI("PerfExtraTask the total data size: %lld", dataSize_);
}

void PerfExtraTask::PerfScriptTask()
{
    std::vector<std::string> files;
    std::vector<std::string> perfDataFiles;

    analysis::dvvp::common::utils::Utils::GetFiles(retDir_, false, files);
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i].find(MSVP_PROF_PERF_DATA_FILE) != std::string::npos &&
            std::count(files[i].begin(), files[i].end(), '.') == 3) { // 3 meaning dot number int ai_ctrl_cpu.data.0.xxx
            perfDataFiles.push_back(files[i]);
        }
    }

    std::sort(perfDataFiles.begin(), perfDataFiles.end());

    size_t i = 0;
    size_t size = perfDataFiles.size();
    std::string retFileName;
    static const size_t LAST_ONE = 1;

    for (; (size > LAST_ONE) && (i < (size - LAST_ONE)); i++) {
        MSPROF_LOGI("PerfExtraTask file: %s", perfDataFiles[i].c_str());
        ResolvePerfRecordData(perfDataFiles[i]);
        retFileName = perfDataFiles[i] + MSVP_PROF_PERF_RET_FILE_SUFFIX;
        StoreData(retFileName);

        (void)::remove(perfDataFiles[i].c_str());
        (void)::remove(retFileName.c_str());
    }

    if (IsQuit()) {
        for (; i < size; i++) {
            MSPROF_LOGI("PerfExtraTask file: %s", perfDataFiles[i].c_str());
            ResolvePerfRecordData(perfDataFiles[i]);
            retFileName = perfDataFiles[i] + MSVP_PROF_PERF_RET_FILE_SUFFIX;
            StoreData(retFileName);

            (void)::remove(perfDataFiles[i].c_str());
            (void)::remove(retFileName.c_str());
        }
    }
}

void PerfExtraTask::ResolvePerfRecordData(const std::string &fileName)
{
    static const std::string cmd = "sudo";
    static const std::string envPath = "PATH=/usr/bin:/usr/sbin:/var";

    std::vector<std::string> argsVec;
    std::vector<std::string> envVec;

    envVec.push_back(envPath);

    std::string newFilePath = fileName + MSVP_PROF_PERF_RET_FILE_SUFFIX;

    argsVec.push_back("perf");
    argsVec.push_back("script");
    argsVec.push_back("-F");
    argsVec.push_back("comm,pid,tid,cpu,time,period,event,ip,sym,dso,symoff");

    argsVec.push_back("-i");
    argsVec.push_back(fileName);
    argsVec.push_back("--show-kernel-path");
    argsVec.push_back("-f");
    int exitCode = analysis::dvvp::common::utils::VALID_EXIT_CODE;
    mmProcess appProcess = MSVP_MMPROCESS;
    analysis::dvvp::common::utils::ExecCmdParams execCmdParams(cmd, false, newFilePath);
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams,
                                                            argsVec,   // const std::vector<std::string> & argv,
                                                            envVec,    // const std::vector<std::string> & envp,
                                                            exitCode,
                                                            appProcess);
    MSPROF_LOGI("resolve ctrlcpu data:%s, ret=%d, exit_code=%d", fileName.c_str(), ret, exitCode);
}

void PerfExtraTask::StoreData(const std::string &fileName)
{
    if (!(analysis::dvvp::common::utils::Utils::IsFileExist(fileName))) {
        MSPROF_LOGW("file:%s is not exist", fileName.c_str());
        return;
    }
    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(fileName);
    if (len <= 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGE("data file size is invalid");
        MSPROF_INNER_ERROR("EK9999", "data file size is invalid");
        return;
    }

    static const char * const PERF_RET_NAME = "data/ai_ctrl_cpu.data";

    UNSIGNED_CHAR_PTR buf = const_cast<UNSIGNED_CHAR_PTR>(buf_.GetBuffer());
    size_t bufSize = buf_.GetBufferSize();

    std::ifstream ifs(fileName, std::ifstream::in);

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk;
    MSVP_MAKE_SHARED0_VOID(fileChunk, analysis::dvvp::proto::FileChunkReq);

    if (!ifs.is_open() || buf == nullptr) {
        return;
    }

    while (ifs.good()) {
        (void)memset_s(buf, bufSize, 0, bufSize);
        ifs.read(reinterpret_cast<CHAR_PTR>(buf), bufSize > 0 ? (bufSize - 1) : 0);

        fileChunk->set_filename(PERF_RET_NAME);
        fileChunk->set_offset(-1);
        fileChunk->set_chunk(buf, ifs.gcount());
        fileChunk->set_chunksizeinbytes(ifs.gcount());
        fileChunk->set_islastchunk(false);
        fileChunk->set_needack(false);
        fileChunk->mutable_hdr()->set_job_ctx(jobCtx_->ToString());
        fileChunk->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE);

        std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(param_->job_id,
            static_cast<void *>(const_cast<CHAR_PTR>(encoded.c_str())), (uint32_t)encoded.size());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Upload cpu data failed , jobId: %s", param_->job_id.c_str());
            MSPROF_INNER_ERROR("EK9999", "Upload cpu data failed , jobId: %s", param_->job_id.c_str());
        }
        dataSize_ += (long long)ifs.gcount();
    }

    ifs.close();
    MSPROF_LOGI("PerfExtraTask data size: %lld", dataSize_);
}

void PerfExtraTask::SetJobCtx(SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx)
{
    if (jobCtx != nullptr) {
        jobCtx_ = jobCtx;
    }
}
}}}

