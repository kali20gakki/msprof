/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */

#include "prof_peripheral_job.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "securec.h"
#include "utils/utils.h"
#include "platform/platform.h"
#include "proto/profiler.pb.h"
#include "uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Common::Config;

/*
 * @berif  : Collect Peripheral profiling data
 */
ProfPeripheralJob::ProfPeripheralJob()
    : samplePeriod_(analysis::dvvp::common::config::DEFAULT_INTERVAL),
      channelId_(PROF_CHANNEL_UNKNOWN)
{
}
ProfPeripheralJob::~ProfPeripheralJob() {}

/*
 * @berif  : Collect Peripheral profiling data Default Init
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfPeripheralJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect Peripheral profiling Set Default peripheral Config
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfPeripheralJob::SetPeripheralConfig()
{
    peripheralCfg_.configP    = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect Peripheral start profiling with default
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 *
 */
int ProfPeripheralJob::Process()
{
    if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            static_cast<int>(channelId_));
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("Begin to start profiling Channel %d, events:%s",
        static_cast<int>(channelId_), eventsStr_.c_str());
    peripheralCfg_.profDataFilePath = BindFileWithChannel(collectionJobCfg_->jobParams.dataPath);
    int ret = SetPeripheralConfig();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ProfPeripheralJob SetPeripheralConfig failed");
        return ret;
    }

    AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_,
        peripheralCfg_.profDataFilePath);

    MSPROF_LOGI("begin to start profiling Channel %d, devId :%d",
        static_cast<int>(channelId_), collectionJobCfg_->comParams->devIdOnHost);

    peripheralCfg_.profDeviceId     = collectionJobCfg_->comParams->devId;
    peripheralCfg_.profChannel      = channelId_;
    peripheralCfg_.profSamplePeriod = static_cast<int32_t>(samplePeriod_);
    peripheralCfg_.profDataFile = "";
    ret = DrvPeripheralStart(peripheralCfg_);
    MSPROF_LOGI("start profiling Channel %d, events:%s, ret=%d",
        static_cast<int>(channelId_), eventsStr_.c_str(), ret);

    Utils::ProfFree(peripheralCfg_.configP);
    peripheralCfg_.configP = nullptr;

    FUNRET_CHECK_RET_VALUE(ret, PROFILING_SUCCESS, PROFILING_SUCCESS, ret);
}

/*
 * @berif  : Collect Peripheral stop profiling with default
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 *
 */
int ProfPeripheralJob::Uninit()
{
    if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
        return PROFILING_SUCCESS;
    }
    if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId_)) {
        MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
            static_cast<int>(channelId_));
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("begin to stop profiling Channel %d data", static_cast<int>(channelId_));

    int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId_);
    MSPROF_LOGI("stop profiling Channel %d data, ret=%d", static_cast<int>(channelId_), ret);

    RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId_);
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect DDR profiling data
 */
ProfDdrJob::ProfDdrJob()
{
    channelId_ = PROF_CHANNEL_DDR;
}

ProfDdrJob::~ProfDdrJob() {}

/*
 * @berif  : DDR Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfDdrJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobEventParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if (0 != collectionJobCfg_->comParams->params->ddr_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON)) {
        MSPROF_LOGI("DDR Profiling not enabled");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

/*
 * @berif  : DDR Peripheral Set Config to Driver
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfDdrJob::SetPeripheralConfig()
{
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_MIN;
    if (collectionJobCfg_->comParams->params->ddr_interval >= PERIPHERAL_INTERVAL_MS_MIN &&
        collectionJobCfg_->comParams->params->ddr_interval <= PERIPHERAL_INTERVAL_MS_MAX) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->ddr_interval);
    }

    eventsStr_ = GetEventsStr(*(collectionJobCfg_->jobParams.events));
    uint32_t configSize = sizeof(TagDdrProfileConfig) +
                          (sizeof(uint32_t) * GetEventSize(*(collectionJobCfg_->jobParams.events)));
    if (configSize == 0 || configSize > 0x7FFFFFFF) {
        MSPROF_LOGE("Profiling Config Size Out Range");
        return PROFILING_FAILED;
    }

    TagDdrProfileConfig *configP = reinterpret_cast<TagDdrProfileConfig *>(Utils::ProfMalloc((size_t)configSize));
    if (configP == nullptr) {
        MSPROF_LOGE("ProfDdrJob ProfMalloc TagDdrProfileConfig failed");
        return PROFILING_FAILED;
    }

    configP->period   = samplePeriod_;
    configP->masterId = DEAFULT_MASTER_ID;

    for (uint32_t i = 0; i < (uint32_t)collectionJobCfg_->jobParams.events->size(); i++) {
        if ((*collectionJobCfg_->jobParams.events)[i].compare("read") == 0) {
            configP->event[configP->eventNum++] = PERIPHERAL_EVENT_READ;
        } else if ((*collectionJobCfg_->jobParams.events)[i].compare("write") == 0) {
            configP->event[configP->eventNum++] = PERIPHERAL_EVENT_WRITE;
        } else if ((*collectionJobCfg_->jobParams.events)[i].compare("master_id") == 0) {  // master id
            configP->masterId = (uint32_t)collectionJobCfg_->comParams->params->ddr_master_id;
        } else {
            MSPROF_LOGW("DDR event:%s not support", (*collectionJobCfg_->jobParams.events)[i].c_str());
        }
    }

    peripheralCfg_.configP = configP;
    peripheralCfg_.configSize = configSize;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect HBM profiling data
 */
ProfHbmJob::ProfHbmJob()
{
    channelId_ = PROF_CHANNEL_HBM;
}
ProfHbmJob::~ProfHbmJob()
{
}

/*
 * @berif  : HBM Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfHbmJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobEventParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;

    if (0 != collectionJobCfg_->comParams->params->hbmProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON)) {
        MSPROF_LOGI("HBM Profiling not enabled");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

/*
 * @berif  : HBM Peripheral Set Config to Driver
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfHbmJob::SetPeripheralConfig()
{
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_MIN;
    if (collectionJobCfg_->comParams->params->hbmInterval >= PERIPHERAL_INTERVAL_MS_MIN &&
        collectionJobCfg_->comParams->params->hbmInterval <= PERIPHERAL_INTERVAL_MS_MAX) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->hbmInterval);
    }

    eventsStr_ = GetEventsStr(*(collectionJobCfg_->jobParams.events));
    uint32_t configSize = sizeof(TagTsHbmProfileConfig) +
                          (sizeof(uint32_t) * GetEventSize(*(collectionJobCfg_->jobParams.events)));
    if (configSize == 0 || configSize > 0x7FFFFFFF) {
        MSPROF_LOGE("Profiling Config Size Out Range");
        return PROFILING_FAILED;
    }
    TagTsHbmProfileConfig *configP = reinterpret_cast<TagTsHbmProfileConfig *>(Utils::ProfMalloc(configSize));
    if (configP == nullptr) {
        MSPROF_LOGE("ProfHbmJob ProfMalloc TagTsHbmProfileConfig failed");
        return PROFILING_FAILED;
    }

    configP->period = samplePeriod_;
    configP->masterId = DEAFULT_MASTER_ID;

    for (uint32_t i = 0; i < (uint32_t)collectionJobCfg_->jobParams.events->size(); i++) {
        if ((*collectionJobCfg_->jobParams.events)[i].compare("read") == 0) {
            configP->event[configP->eventNum++] = PERIPHERAL_EVENT_READ;
        } else if ((*collectionJobCfg_->jobParams.events)[i].compare("write") == 0) {
            configP->event[configP->eventNum++] = PERIPHERAL_EVENT_WRITE;
        } else {
            MSPROF_LOGW("HBM event:%s not support", (*collectionJobCfg_->jobParams.events)[i].c_str());
        }
    }

    peripheralCfg_.configP = configP;
    peripheralCfg_.configSize = configSize;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect HCCS profiling data
 */
ProfHccsJob::ProfHccsJob()
{
    channelId_ = PROF_CHANNEL_HCCS;
}

ProfHccsJob::~ProfHccsJob() {}

/*
 * @berif  : HCCS Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfHccsJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;

    if (0 != collectionJobCfg_->comParams->params->hccsProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON)) {
        MSPROF_LOGI("HCCS Profiling not enabled");
        return PROFILING_FAILED;
    }

    std::vector<std::string> profDataFilePathV;
    profDataFilePathV.push_back(collectionJobCfg_->comParams->tmpResultDir);
    profDataFilePathV.push_back("data");
    profDataFilePathV.push_back("hccs.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_MIN;
    if (collectionJobCfg_->comParams->params->hccsInterval >= PERIPHERAL_INTERVAL_MS_MIN &&
        collectionJobCfg_->comParams->params->hccsInterval <= PERIPHERAL_INTERVAL_MS_MAX) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->hccsInterval);
    }

    peripheralCfg_.configP = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect PCIE profiling data
 */
ProfPcieJob::ProfPcieJob()
{
    channelId_ = PROF_CHANNEL_PCIE;
}

ProfPcieJob::~ProfPcieJob() {}

/*
 * @berif  : PCIE Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfPcieJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if (0 != collectionJobCfg_->comParams->params->pcieProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON)) {
        MSPROF_LOGI("PCIE Profiling not enabled");
        return PROFILING_FAILED;
    }

    std::vector<std::string> profDataFilePathV;
    profDataFilePathV.push_back(collectionJobCfg_->comParams->tmpResultDir);
    profDataFilePathV.push_back("data");
    profDataFilePathV.push_back("pcie.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);

    samplePeriod_ = PERIPHERAL_INTERVAL_MS_MIN;
    if (collectionJobCfg_->comParams->params->pcieInterval >= PERIPHERAL_INTERVAL_MS_MIN &&
        collectionJobCfg_->comParams->params->pcieInterval <= PERIPHERAL_INTERVAL_MS_MAX) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->pcieInterval);
    }

    peripheralCfg_.configP = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect NIC profiling data
 */
ProfNicJob::ProfNicJob()
{
    channelId_ = PROF_CHANNEL_NIC;
}

ProfNicJob::~ProfNicJob() {}

/*
 * @berif  : NIC Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfNicJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->nicProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("NIC Profiling not enabled");
        return PROFILING_FAILED;
    }

    std::vector<std::string> profDataFilePathV;
    profDataFilePathV.push_back(collectionJobCfg_->comParams->tmpResultDir);
    profDataFilePathV.push_back("data");
    profDataFilePathV.push_back("nic.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_SMIN;
    if (collectionJobCfg_->comParams->params->nicInterval > 0) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->nicInterval);
    }
    MSPROF_LOGI("NIC Profiling samplePeriod_:%d", samplePeriod_);

    peripheralCfg_.configP = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : Collect DVPP profiling data
 */
ProfDvppJob::ProfDvppJob()
{
    channelId_ = PROF_CHANNEL_DVPP;
    channelList_ = {PROF_CHANNEL_DVPP_VENC,
               PROF_CHANNEL_DVPP_JPEGE,
               PROF_CHANNEL_DVPP_VDEC,
               PROF_CHANNEL_DVPP_JPEGD,
               PROF_CHANNEL_DVPP_VPC,
               PROF_CHANNEL_DVPP_PNG,
               PROF_CHANNEL_DVPP_SCD};
    fileNameList_ = {{PROF_CHANNEL_DVPP_JPEGD, "data/dvpp.jpegd"},
                {PROF_CHANNEL_DVPP_JPEGE, "data/dvpp.jpege"},
                {PROF_CHANNEL_DVPP_PNG, "data/dvpp.png"},
                {PROF_CHANNEL_DVPP_SCD, "data/dvpp.scd"},
                {PROF_CHANNEL_DVPP_VENC, "data/dvpp.venc"},
                {PROF_CHANNEL_DVPP_VPC, "data/dvpp.vpc"},
                {PROF_CHANNEL_DVPP_VDEC, "data/dvpp.vdec"}};
}
ProfDvppJob::~ProfDvppJob()
{
    channelList_.clear();
    fileNameList_.clear();
}

/*
 * @berif  : DVPP Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfDvppJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->dvpp_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("DVPP Profiling not enabled");
        return PROFILING_FAILED;
    }

    std::vector<std::string> profDataFilePathV;
    profDataFilePathV.push_back(collectionJobCfg_->comParams->tmpResultDir);
    profDataFilePathV.push_back("data");
    profDataFilePathV.push_back("dvpp.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_SMIN;
    if (collectionJobCfg_->comParams->params->dvpp_sampling_interval > 0) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->dvpp_sampling_interval);
    }

    peripheralCfg_.configP = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

int ProfDvppJob::Process()
{
    PlatformType type = ConfigManager::instance()->GetPlatformType();
    if (type == PlatformType::MDC_TYPE || type == PlatformType::DC_TYPE ||
        type == PlatformType::CHIP_V4_1_0 || type == PlatformType::CHIP_V4_2_0) {
        if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        (void)SetPeripheralConfig();
        for (auto channelId : channelList_) {
            if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId)) {
                MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
                    static_cast<int>(channelId));
                continue;
            }
            std::string filePath =
                collectionJobCfg_->comParams->tmpResultDir + MSVP_SLASH + fileNameList_[channelId];
            AddReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId,
                filePath);
            MSPROF_LOGI("begin to start profiling Channel %d, devId :%d",
                static_cast<int>(channelId), collectionJobCfg_->comParams->devIdOnHost);
            peripheralCfg_.profDeviceId     = collectionJobCfg_->comParams->devId;
            peripheralCfg_.profChannel      = channelId;
            peripheralCfg_.profSamplePeriod = static_cast<int32_t>(samplePeriod_);
            peripheralCfg_.profDataFile = "";
            int ret = DrvPeripheralStart(peripheralCfg_);
            MSPROF_LOGI("start profiling Channel %d, events:%s, ret=%d",
                static_cast<int>(channelId), eventsStr_.c_str(), ret);

            Utils::ProfFree(peripheralCfg_.configP);
            peripheralCfg_.configP = nullptr;
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("ProfDvppJob DrvPeripheralStart failed, channelId:%d", static_cast<int>(channelId));
                continue;
            }
        }
        return PROFILING_SUCCESS;
    } else {
        return ProfPeripheralJob::Process();
    }
}

int ProfDvppJob::Uninit()
{
    using namespace Analysis::Dvvp::Common::Config;
    PlatformType type = ConfigManager::instance()->GetPlatformType();
    if (type == PlatformType::MDC_TYPE || type == PlatformType::DC_TYPE ||
        type == PlatformType::CHIP_V4_1_0 || type == PlatformType::CHIP_V4_2_0) {
        if (CheckJobCommonParam(collectionJobCfg_) != PROFILING_SUCCESS) {
            return PROFILING_SUCCESS;
        }
        for (auto channelId : channelList_) {
            if (!DrvChannelsMgr::instance()->ChannelIsValid(collectionJobCfg_->comParams->devId, channelId)) {
                MSPROF_LOGW("Channel is invalid, devId:%d, channelId:%d", collectionJobCfg_->comParams->devId,
                    static_cast<int>(channelId));
                continue;
            }

            MSPROF_LOGI("begin to stop profiling Channel %d data", static_cast<int>(channelId));

            int ret = DrvStop(collectionJobCfg_->comParams->devId, channelId);
            MSPROF_LOGI("stop profiling Channel %d data, ret=%d", static_cast<int>(channelId), ret);
            RemoveReader(collectionJobCfg_->comParams->params->job_id, collectionJobCfg_->comParams->devId, channelId);
        }
        return PROFILING_SUCCESS;
    } else {
        return ProfPeripheralJob::Uninit();
    }
}

/*
 * @berif  : Collect LLC profiling data
 */
ProfLlcJob::ProfLlcJob()
    : llcProcess_(MSVP_MMPROCESS)
{
    channelId_ = PROF_CHANNEL_LLC;
}

ProfLlcJob::~ProfLlcJob() {}

/*
 * @berif  : LLC Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfLlcJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobEventParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    if (!Platform::instance()->RunSocSide() &&
        ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        MSPROF_LOGI("Not in device Side, LLC Profiling not enabled");
        return PROFILING_FAILED;
    }

    collectionJobCfg_ = cfg;

    if (0 != collectionJobCfg_->comParams->params->msprof_llc_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON)) {
        MSPROF_LOGI("LLC Profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

/*
 * @berif  : LLC Peripheral Set Config to Driver
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfLlcJob::SetPeripheralConfig()
{
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_MIN;
    if (collectionJobCfg_->comParams->params->llc_interval >= PERIPHERAL_INTERVAL_MS_MIN &&
        collectionJobCfg_->comParams->params->llc_interval <= PERIPHERAL_INTERVAL_MS_MAX) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->llc_interval);
    }

    eventsStr_ = GetEventsStr(*(collectionJobCfg_->jobParams.events));
    uint32_t configSize = sizeof(TagLlcProfileConfig);
    TagLlcProfileConfig *configP = reinterpret_cast<TagLlcProfileConfig *>(Utils::ProfMalloc(configSize));
    if (configP == nullptr) {
        MSPROF_LOGE("ProfLlcJob ProfMalloc TagLlcProfileConfig failed");
        return PROFILING_FAILED;
    }

    configP->period   = samplePeriod_;

    static const uint32_t LLC_READ  = 1;
    static const uint32_t LLC_WRITE = 2;
    for (uint32_t i = 0; i < (uint32_t)collectionJobCfg_->jobParams.events->size(); i++) {
        if ((*collectionJobCfg_->jobParams.events)[i].compare("read") == 0) {
            configP->sampleType = LLC_READ;
        } else if ((*collectionJobCfg_->jobParams.events)[i].compare("write") == 0) {
            configP->sampleType = LLC_WRITE;
        } else {
            MSPROF_LOGW("LLC event:%s not support", (*collectionJobCfg_->jobParams.events)[i].c_str());
        }
    }

    peripheralCfg_.configP = configP;
    peripheralCfg_.configSize = configSize;
    return PROFILING_SUCCESS;
}

/*
 * @berif  : LLC Collect Peripheral start profiling
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 *
 */
int ProfLlcJob::Process()
{
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::MINI_TYPE) {
        return ProfPeripheralJob::Process();
    } else {
        std::string profLlcCmd;
        if (CheckJobEventParam(collectionJobCfg_) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        GetCollectLlcEventsCmd(collectionJobCfg_->comParams->devId, *(collectionJobCfg_->jobParams.events), profLlcCmd);
        MSPROF_LOGI("llc_event:%s, profLlcCmd:%s",
            GetEventsStr(*collectionJobCfg_->jobParams.events).c_str(), profLlcCmd.c_str());
        int ret = PROFILING_SUCCESS;
        if (profLlcCmd.size() > 0) {
            MSPROF_LOGI("Begin to start profiling llc, cmd=%s", profLlcCmd.c_str());

            std::vector<std::string> params =
                analysis::dvvp::common::utils::Utils::Split(profLlcCmd.c_str());

            if (params.empty()) {
                MSPROF_LOGE("profLlcCmd empty");
                return PROFILING_FAILED;
            }

            std::string cmd = params[0];
            std::vector<std::string> argsV;
            std::vector<std::string> envsV;

            if (params.size() > 1) {
                argsV.assign(params.begin() + 1, params.end());
            }
            envsV.push_back("PATH=/usr/bin/:/usr/sbin:/var");

            llcProcess_ = MSVP_MMPROCESS;

            int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
            ExecCmdParams execCmdParams(cmd, true, "");
            ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, llcProcess_);
            MSPROF_LOGI("start profiling llc, pid = %u, ret=%d",
                        (unsigned int)llcProcess_, ret);
            FUNRET_CHECK_FAIL_PRINT(ret, PROFILING_SUCCESS);
        }

        return ret;
    }
}

/*
 * @berif  : LLC Collect Peripheral stop profiling
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 *
 */
int ProfLlcJob::Uninit()
{
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::MINI_TYPE) {
        return ProfPeripheralJob::Uninit();
    } else {
        static const std::string ENV_PATH = "PATH=/usr/bin:/usr/sbin";
        std::vector<std::string> envV;
        envV.push_back(ENV_PATH);
        std::vector<std::string> argsV;
        argsV.push_back("pkill");
        argsV.push_back("-2");
        argsV.push_back("perf");

        int exitCode = analysis::dvvp::common::utils::VALID_EXIT_CODE;
        static const std::string CMD = "sudo";
        MmProcess appProcess = MSVP_MMPROCESS;
        ExecCmdParams execCmdParams(CMD, false, "");
        int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams,
                                                                argsV,
                                                                envV,
                                                                exitCode,
                                                                appProcess);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to kill process perf, ret=%d", ret);
        } else {
            MSPROF_LOGI("Succeeded to kill process perf, ret=%d, exitCode=%d", ret, exitCode);
        }
        if (llcProcess_ > 0) {
            bool isExited = false;
            ret = analysis::dvvp::common::utils::Utils::WaitProcess(llcProcess_,
                                                                    isExited,
                                                                    exitCode,
                                                                    true);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to wait process %d, ret=%d",
                            reinterpret_cast<int>(llcProcess_), ret);
            } else {
                MSPROF_LOGI("Process %d exited, exit code=%d",
                            reinterpret_cast<int>(llcProcess_), exitCode);
            }
        }
        SendData();
        return ret;
    }
}

bool ProfLlcJob::IsGlobalJobLevel()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
            return true;
    }
    return false;
}

void ProfLlcJob::SendData()
{
    std::string llcDataFile = collectionJobCfg_->jobParams.dataPath + "." +
        std::to_string(collectionJobCfg_->comParams->devIdOnHost);
    std::ifstream ifs(llcDataFile, std::ifstream::in);

    if (!ifs.is_open()) {
        return;
    }
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk;
    MSVP_MAKE_SHARED0_VOID(fileChunk, analysis::dvvp::proto::FileChunkReq);

    static const unsigned int PERF_DATA_BUF_SIZE_M = 262144 + 1; // 262144 + 1, 256K Byte + '\0'
    static const char * const PERF_RET_NAME = "data/llc.data";
    SHARED_PTR_ALIA<char> buf;
    MSVP_MAKE_SHARED_ARRAY_VOID(buf, char, PERF_DATA_BUF_SIZE_M);

    while (ifs.good()) {
        (void)memset_s(buf.get(), PERF_DATA_BUF_SIZE_M, 0, PERF_DATA_BUF_SIZE_M);
        ifs.read(buf.get(), PERF_DATA_BUF_SIZE_M - 1);

        fileChunk->set_filename(PERF_RET_NAME);
        fileChunk->set_offset(-1);
        fileChunk->set_chunk(buf.get(), ifs.gcount());
        fileChunk->set_chunksizeinbytes(ifs.gcount());
        fileChunk->set_islastchunk(false);
        fileChunk->set_needack(false);
        fileChunk->mutable_hdr()->set_job_ctx(collectionJobCfg_->comParams->jobCtx->ToString());
        fileChunk->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE);

        std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
            collectionJobCfg_->comParams->params->job_id,
            static_cast<void *>(const_cast<CHAR_PTR>(encoded.c_str())), (uint32_t)encoded.size());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Upload llc data failed , jobId: %s", collectionJobCfg_->comParams->params->job_id.c_str());
        }
    }

    ifs.close();
}

/*
 * @berif  : Get LLC Collect Peripheral Command of start profiling
 * @param  : None
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 *
 */
int ProfLlcJob::GetCollectLlcEventsCmd(int devId,
    const std::vector<std::string> &events, std::string &profLlcCmd)
{
    std::vector<std::string> profDataFilePathV;
    std::string perfDataDir =
        Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(devId);
    profDataFilePathV.push_back(perfDataDir);
    profDataFilePathV.push_back("llc.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);
    std::string llcDataFile = collectionJobCfg_->jobParams.dataPath + "." +
        std::to_string(collectionJobCfg_->comParams->devIdOnHost);

    int ret = analysis::dvvp::common::utils::Utils::CreateDir(perfDataDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Creating dir: %s err!", Utils::BaseName(perfDataDir).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return PROFILING_FAILED;
    }

    std::ofstream llcFile(llcDataFile);
    if (llcFile.is_open()) {
        llcFile.close();
    } else {
        MSPROF_LOGE("Failed to open %s, devId=%d, devIdOnHost:%d", llcDataFile.c_str(),
            devId, collectionJobCfg_->comParams->devIdOnHost);
        return PROFILING_FAILED;
    }

    std::stringstream ssPerfLlcCmd;
    int llcIntervalMs = 100; // Print count deltas every 100ms defaultly

    std::string eventsStr = GetEventsStr(events);
    if (eventsStr.empty() || !ParamValidation::instance()->CheckLlcEventsIsValid(eventsStr)) {
        return PROFILING_FAILED;
    }
    if (collectionJobCfg_->comParams->params->llc_interval > 0) {
        llcIntervalMs = collectionJobCfg_->comParams->params->llc_interval;
    }
    ssPerfLlcCmd << "sudo perf stat -o ";
    ssPerfLlcCmd << llcDataFile;
    ssPerfLlcCmd << " -a -e'";
    ssPerfLlcCmd << eventsStr;
    ssPerfLlcCmd << "' -I ";
    ssPerfLlcCmd << llcIntervalMs;

    profLlcCmd = ssPerfLlcCmd.str();

    return PROFILING_SUCCESS;
}

ProfRoceJob::ProfRoceJob()
{
    channelId_ = PROF_CHANNEL_ROCE;
}
ProfRoceJob::~ProfRoceJob() {}

/*
 * @berif  : ROCE Peripheral Init profiling
 * @param  : cfg : Collect data config infomation
 * @return : PROFILING_FAILED(-1) :failed
 *       : PROFILING_SUCCESS(0) : success
 */
int ProfRoceJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->roceProfiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("ROCE Profiling not enabled");
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("ROCE Profiling enabled");
    std::vector<std::string> profDataFilePathV;
    profDataFilePathV.push_back(collectionJobCfg_->comParams->tmpResultDir);
    profDataFilePathV.push_back("data");
    profDataFilePathV.push_back("roce.data");
    collectionJobCfg_->jobParams.dataPath = analysis::dvvp::common::utils::Utils::JoinPath(profDataFilePathV);
    samplePeriod_ = PERIPHERAL_INTERVAL_MS_SMIN;
    if (collectionJobCfg_->comParams->params->roceInterval > 0) {
        samplePeriod_ = static_cast<uint32_t>(collectionJobCfg_->comParams->params->roceInterval);
    }

    peripheralCfg_.configP = nullptr;
    peripheralCfg_.configSize = 0;
    return PROFILING_SUCCESS;
}

ProfStarsSocProfileJob::ProfStarsSocProfileJob()
{
    channelId_ = PROF_CHANNEL_STARS_SOC_PROFILE;
}

ProfStarsSocProfileJob::~ProfStarsSocProfileJob() {}

int ProfStarsSocProfileJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    if (CheckJobCommonParam(cfg) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (cfg->comParams->params->host_profiling) {
        return PROFILING_FAILED;
    }
    collectionJobCfg_ = cfg;
    if ((collectionJobCfg_->comParams->params->interconnection_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) &&
        (collectionJobCfg_->comParams->params->hardware_mem.compare(
            analysis::dvvp::common::config::MSVP_PROF_ON) != 0) &&
        (collectionJobCfg_->comParams->params->low_power.compare(
            analysis::dvvp::common::config::MSVP_PROF_ON) != 0)) {
        MSPROF_LOGI("StarsSocProfile Profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
int ProfStarsSocProfileJob::SetPeripheralConfig()
{
    uint32_t configSize = sizeof(StarsSocProfileConfigT);
    int period;
    StarsSocProfileConfigT *configP = static_cast<StarsSocProfileConfigT *>(
        Utils::ProfMalloc(static_cast<size_t>(configSize)));
    if (configP == nullptr) {
        MSPROF_LOGE("ProfStarsSocProfileJob ProfMalloc failed");
        return PROFILING_FAILED;
    }

    if (collectionJobCfg_->comParams->params->interconnection_profiling ==
        analysis::dvvp::common::config::MSVP_PROF_ON) {
        period = collectionJobCfg_->comParams->params->interconnection_sampling_interval;
        configP->inter_chip.innerSwitch = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP->inter_chip.period = static_cast<uint32_t>(period > 0 ? period : DEFAULT_PROFILING_INTERVAL_20MS);
    }

    if (collectionJobCfg_->comParams->params->hardware_mem == analysis::dvvp::common::config::MSVP_PROF_ON) {
        period = collectionJobCfg_->comParams->params->hardware_mem_sampling_interval;
        configP->on_chip.innerSwitch = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP->on_chip.period = static_cast<uint32_t>(period > 0 ? period : DEFAULT_PROFILING_INTERVAL_20MS);
        configP->acc_pmu.innerSwitch = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP->acc_pmu.period = DEFAULT_PROFILING_INTERVAL_20MS;
    }

    if (collectionJobCfg_->comParams->params->low_power == analysis::dvvp::common::config::MSVP_PROF_ON) {
        configP->low_power.innerSwitch = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP->low_power.period = DEFAULT_PROFILING_INTERVAL_20MS;
    }

    MSPROF_LOGI("SocProfileParam: acc_pmu:[%d, %d], on_chip:[%d, %d], inter_chip:[%d, %d], low_power:[%d, %d]",
        configP->acc_pmu.innerSwitch, configP->acc_pmu.period,
        configP->on_chip.innerSwitch, configP->on_chip.period,
        configP->inter_chip.innerSwitch, configP->inter_chip.period,
        configP->low_power.innerSwitch, configP->low_power.period);
    peripheralCfg_.configP = configP;
    peripheralCfg_.configSize = configSize;
    return PROFILING_SUCCESS;
}
}
}
}
