/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#include "ai_drv_prof_api.h"
#include <cerrno>
#include <map>
#include "ai_drv_dev_api.h"
#include "driver_plugin.h"
#include "errno/error_code.h"
#include "msprof_error_manager.h"
#include "securec.h"
namespace analysis {
namespace dvvp {
namespace driver {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
// 32 * 1024 * 0.8  is the full threshold  of ai_core_sample
constexpr uint32_t AI_CORE_SAMPLE_FULL_THRESHOLD = 32 * 1024 * 0.8;

std::map<std::string, AI_DRV_CHANNEL> g_channelMaps;

#define CHANNEL_STR(s) #s

const int STRING_TO_LONG_WEIGHT = 16;

int DrvGetChannels(struct DrvProfChannelsInfo &channels)
{
    MSPROF_EVENT("Begin to get channels, deviceId=%d", channels.deviceId);
    if (channels.deviceId < 0) {
        MSPROF_LOGE("DrvGetChannels, devId is invalid, deviceId=%d", channels.deviceId);
        return PROFILING_FAILED;
    }

    channel_list_t channelList;
    (void)memset_s(&channelList, sizeof(channel_list_t), 0, sizeof(channel_list_t));
    int ret = DriverPlugin::instance()->MsprofDrvGetChannels(channels.deviceId, &channelList);
    if (ret != PROF_OK) {
        MSPROF_LOGE("DrvGetChannels get channels failed, deviceId=%d", channels.deviceId);
        return PROFILING_FAILED;
    }
    if (channelList.channel_num > PROF_CHANNEL_NUM_MAX || channelList.channel_num == 0) {
        MSPROF_LOGE("DrvGetChannels channel num is invalid, channelNum=%u", channelList.channel_num);
        return PROFILING_FAILED;
    }
    channels.chipType = channelList.chip_type;
    for (unsigned int i = 0; i < channelList.channel_num; i++) {
        struct DrvProfChannelInfo channel;
        if (channelList.channel[i].channel_id == 0 ||
            channelList.channel[i].channel_id > AI_DRV_CHANNEL::PROF_CHANNEL_MAX) {
            MSPROF_LOGE("Channel id is invalid, channelId=%u, i=%d", channelList.channel[i].channel_id, i);
            continue;
        }
        channel.channelId = static_cast<analysis::dvvp::driver::AI_DRV_CHANNEL>(channelList.channel[i].channel_id);
        channel.channelType = channelList.channel[i].channel_type;
        channel.channelName = std::string(channelList.channel[i].channel_name, PROF_CHANNEL_NAME_LEN);
        MSPROF_LOGI("i:%d,chipType:%u,channelNum:%u,channelId=%u,channelName:%s,channelType:%u",
            i, channelList.chip_type, channelList.channel_num, channelList.channel[i].channel_id,
            channel.channelName.c_str(), channelList.channel[i].channel_type);
        channels.channels.push_back(channel);
    }
    MSPROF_EVENT("End to get channels, deviceId=%d", channels.deviceId);
    return PROFILING_SUCCESS;
}

AiDrvProfApi::AiDrvProfApi()
{
}

AiDrvProfApi::~AiDrvProfApi()
{
}

DrvChannelsMgr::DrvChannelsMgr()
{
}

DrvChannelsMgr::~DrvChannelsMgr()
{
}

int DrvChannelsMgr::GetAllChannels(int indexId)
{
    std::lock_guard<std::mutex> lk(mtxChannel_);
    MSPROF_LOGI("Begin to GetAllChannels, devIndexId %d", indexId);
    struct DrvProfChannelsInfo channels;
    channels.deviceId = indexId;
    if (DrvGetChannels(channels) != PROFILING_SUCCESS) {
        MSPROF_LOGE("DrvGetChannels failed, devId:%d", channels.deviceId);
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("GetAllChannels, devId:%d", channels.deviceId);
    devIdChannelsMap_[channels.deviceId] = channels;
    return PROFILING_SUCCESS;
}

bool DrvChannelsMgr::ChannelIsValid(int devId, AI_DRV_CHANNEL channelId)
{
    std::lock_guard<std::mutex> lk(mtxChannel_);
    auto iter = devIdChannelsMap_.find(devId);
    if (iter == devIdChannelsMap_.end()) {
        MSPROF_LOGI("ChannelIsValid not find channel map, devId:%d", devId);
        return false;
    }
    for (auto channel : iter->second.channels) {
        if (channel.channelId == channelId) {
            MSPROF_LOGI("ChannelIsValid find channel map, devId:%d, channelId:%d", devId, channelId);
            return true;
        }
    }
    MSPROF_LOGI("ChannelIsValid not find channel map, devid:%d", devId);
    return false;
}

int DrvPeripheralStart(const DrvPeripheralProfileCfg &peripheralCfg)
{
    MSPROF_EVENT("Begin to start profiling DrvPeripheralStart, profDeviceId=%d,"
        " profChannel=%d, profSamplePeriod=%d",
        peripheralCfg.profDeviceId, static_cast<int>(peripheralCfg.profChannel), peripheralCfg.profSamplePeriod);
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_PERIPHERAL_TYPE;
    profStartPara.sample_period = (unsigned int)peripheralCfg.profSamplePeriod;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = peripheralCfg.configP;
    profStartPara.user_data_size = peripheralCfg.configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart((unsigned int)peripheralCfg.profDeviceId,
        (unsigned int)peripheralCfg.profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvPeripheralStart, profDeviceId=%d,"
            " profChannel=%d, profSamplePeriod=%d, ret=%d",
            peripheralCfg.profDeviceId, static_cast<int>(peripheralCfg.profChannel),
            peripheralCfg.profSamplePeriod, ret);
        return PROFILING_FAILED;
    }

    MSPROF_EVENT("Succeeded to start profiling DrvPeripheralStart, profDeviceId=%d,"
        " profChannel=%d, profSamplePeriod=%d",
        peripheralCfg.profDeviceId, static_cast<int>(peripheralCfg.profChannel), peripheralCfg.profSamplePeriod);

    return PROFILING_SUCCESS;
}

template<typename T>
int DoProfTsCpuStart(const DrvPeripheralProfileCfg &peripheralCfg,
                     const std::vector<std::string> &profEvents,
                     TEMPLATE_T_PTR<T> configP,
                     uint32_t configSize)
{
    if (configP == nullptr) {
        return PROF_ERROR;
    }
    uint32_t profDeviceId = (uint32_t)peripheralCfg.profDeviceId;
    AI_DRV_CHANNEL profChannel = peripheralCfg.profChannel;
    uint32_t profSamplePeriod = (uint32_t)peripheralCfg.profSamplePeriod;

    (void)memset_s(configP, configSize, 0, configSize);
    configP->period = (uint32_t)profSamplePeriod;
    configP->event_num = (uint32_t)profEvents.size();
    std::string eventStr;
    for (uint32_t i = 0; i < (uint32_t)profEvents.size(); i++) {
        configP->event[i] = (uint32_t)strtol(profEvents[i].c_str(), nullptr, STRING_TO_LONG_WEIGHT);
        eventStr.append(profEvents[i] + ",");
    }
    MSPROF_EVENT("Begin to start profiling DoProfTsCpuStart, profDeviceId=%d,"
        " profChannel=%d, profSamplePeriod=%d",
        profDeviceId, static_cast<int>(profChannel), profSamplePeriod);
    MSPROF_EVENT("DoProfTsCpuStart, period=%d, event_num=%d, events=%s", configP->period,
        configP->event_num, eventStr.c_str());
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = (unsigned int)peripheralCfg.profSamplePeriod;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart(profDeviceId, profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DoProfTsCpuStart, profDeviceId=%d,"
            " profChannel=%d, profSamplePeriod=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), profSamplePeriod, ret);
        return ret;
    }

    MSPROF_EVENT("Succeeded to start profiling DoProfTsCpuStart, profDeviceId=%d,"
        " profChannel=%d, profSamplePeriod=%d",
        profDeviceId, static_cast<int>(profChannel), profSamplePeriod);
    return PROFILING_SUCCESS;
}

int DrvTscpuStart(const DrvPeripheralProfileCfg &peripheralCfg,
                  const std::vector<std::string> &profEvents)
{
    uint32_t configSize =
        sizeof(TsTsCpuProfileConfigT) + profEvents.size() * sizeof(uint32_t);
    auto configP = static_cast<TsTsCpuProfileConfigT *>(malloc(configSize));
    if (configP == nullptr) {
        return PROFILING_FAILED;
    }
    int ret = DoProfTsCpuStart(peripheralCfg, profEvents, configP, configSize);
    free(configP);
    configP = nullptr;
    if (ret != PROF_OK) {
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int DrvAicoreStart(const DrvPeripheralProfileCfg &peripheralCfg, const std::vector<int> &profCores,
                   const std::vector<std::string> &profEvents)
{
    uint32_t profDeviceId = (uint32_t)peripheralCfg.profDeviceId;
    AI_DRV_CHANNEL profChannel = peripheralCfg.profChannel;
    uint32_t profSamplePeriod = (uint32_t)peripheralCfg.profSamplePeriod;
    uint32_t configSize = sizeof(TsAiCoreProfileConfigT) + profEvents.size() * sizeof(uint32_t);

    auto configP = static_cast<TsAiCoreProfileConfigT *>(malloc(configSize));
    if (configP == nullptr) {
        return PROFILING_FAILED;
    }
    (void)memset_s(configP, configSize, 0, configSize);
    configP->type = (uint32_t)TS_PROF_TYPE_SAMPLE_BASE;
    configP->almost_full_threshold = AI_CORE_SAMPLE_FULL_THRESHOLD;
    configP->period = static_cast<uint32_t>(profSamplePeriod);
    for (uint32_t i = 0; i < (uint32_t)profCores.size(); i++) {
        configP->core_mask |= ((uint32_t)1 << (uint32_t)profCores[i]);
    }
    std::string eventStr;
    configP->event_num = (uint32_t)profEvents.size();
    for (uint32_t i = 0; i < (uint32_t)profEvents.size(); i++) {
        configP->event[i] = (uint32_t)strtol(profEvents[i].c_str(), nullptr, STRING_TO_LONG_WEIGHT);
        eventStr.append(profEvents[i] + ",");
    }
    MSPROF_EVENT("Begin to start profiling DrvAicoreStart, profDeviceId=%d, profChannel=%d, profSamplePeriod=%d",
        profDeviceId, static_cast<int>(profChannel), profSamplePeriod);
    MSPROF_EVENT("DrvAicoreStart, period=%d, event_num=%d, events=%s", configP->period,
        configP->event_num, eventStr.c_str());
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = (unsigned int)peripheralCfg.profSamplePeriod;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart(profDeviceId, profChannel, &profStartPara);
    free(configP);
    configP = nullptr;
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvAicoreStart, profDeviceId=%d, profChannel=%d,"
            " profSamplePeriod=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), profSamplePeriod, ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvAicoreStart, profDeviceId=%d, profChannel=%d, profSamplePeriod=%d",
        profDeviceId, static_cast<int>(profChannel), profSamplePeriod);
    return PROFILING_SUCCESS;
}

int DrvAicoreTaskBasedStart(int profDeviceId, AI_DRV_CHANNEL profChannel, const std::vector<std::string> &profEvents)
{
    uint32_t configSize =
        sizeof(TsAiCoreProfileConfigT) + profEvents.size() * sizeof(uint32_t);

    auto configP = static_cast<TsAiCoreProfileConfigT *>(malloc(configSize));
    if (configP == nullptr) {
        return PROFILING_FAILED;
    }

    (void)memset_s(configP, configSize, 0, configSize);
    configP->type = (uint32_t)TS_PROF_TYPE_TASK_BASE;
    configP->event_num = (uint32_t)profEvents.size();
    std::string eventStr;
    for (uint32_t i = 0; i < (uint32_t)profEvents.size(); i++) {
        configP->event[i] = (uint32_t)strtol(profEvents[i].c_str(), nullptr, STRING_TO_LONG_WEIGHT);
        eventStr.append(profEvents[i] + ",");
    }
    MSPROF_EVENT("Begin to start profiling DrvAicoreTaskBasedStart, profDeviceId=%d, profChannel=%d, configSize:%d",
        profDeviceId, static_cast<int>(profChannel), configSize);
    MSPROF_EVENT("DrvAicoreTaskBasedStart, event_num=%d, events=%s", configP->event_num, eventStr.c_str());
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart((uint32_t)profDeviceId, profChannel, &profStartPara);

    free(configP);
    configP = nullptr;

    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvAicoreTaskBasedStart, profDeviceId=%d,"
            " profChannel=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }

    MSPROF_EVENT("Succeeded to start profiling DrvAicoreTaskBasedStart,"
        " profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));

    return PROFILING_SUCCESS;
}

int DrvL2CacheTaskStart(int profDeviceId, AI_DRV_CHANNEL profChannel, const std::vector<std::string> &profEvents)
{
    uint32_t configSize = sizeof(TagTsL2CacheProfileConfig) + profEvents.size() * sizeof(uint32_t);
    auto configP = static_cast<TagTsL2CacheProfileConfig *>(malloc(configSize));
    if (configP == nullptr) {
        return PROFILING_FAILED;
    }

    (void)memset_s(configP, configSize, 0, configSize);
    configP->eventNum = (uint32_t)profEvents.size();
    std::string eventStr;
    for (uint32_t i = 0; i < (uint32_t)profEvents.size(); i++) {
        configP->event[i] = (uint32_t)strtol(profEvents[i].c_str(), nullptr, STRING_TO_LONG_WEIGHT);
        eventStr.append(profEvents[i] + ",");
        MSPROF_LOGI("Receice DrvL2CacheTaskEvent EventId=%d, EventCode=0x%x", i, configP->event[i]);
    }
    MSPROF_EVENT("Begin to start profiling DrvL2CacheTaskStart, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));
    MSPROF_EVENT("DrvL2CacheTaskStart, eventNum=%d, events=%s", configP->eventNum, eventStr.c_str());
    struct prof_start_para profStartPara;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = configP;
    profStartPara.user_data_size = configSize;
    profStartPara.channel_type = PROF_TS_TYPE;
    int ret = DriverPlugin::instance()->MsprofDrvStart((uint32_t)profDeviceId, profChannel, &profStartPara);
    free(configP);
    configP = nullptr;
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvL2CacheTaskStart, profDeviceId=%d,"
            " profChannel=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }

    MSPROF_EVENT("Succeeded to start profiling DrvL2CacheTaskStart, profDeviceId=%d, profChannel=%d",
                 profDeviceId, static_cast<int>(profChannel));

    return PROFILING_SUCCESS;
}

int DrvSetTsCommandType(TsTsFwProfileConfigT &configP,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams)
{
    if (profileParams == nullptr) {
        return PROFILING_FAILED;
    }
    if (profileParams->ts_timeline.compare("on") == 0) {
        configP.ts_timeline = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
    }
    if (profileParams->ts_keypoint.compare("on") == 0) {
        configP.ts_keypoint = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
    }
    if (profileParams->ts_memcpy.compare("on") == 0) {
        configP.ts_memcpy = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
    }
    return PROFILING_SUCCESS;
}

int DrvTsFwStart(const DrvPeripheralProfileCfg &peripheralCfg,
                 SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams)
{
    if (profileParams == nullptr) {
        return PROFILING_FAILED;
    }
    uint32_t profDeviceId = (uint32_t)peripheralCfg.profDeviceId;
    AI_DRV_CHANNEL profChannel = peripheralCfg.profChannel;
    uint32_t profSamplePeriod = (uint32_t)peripheralCfg.profSamplePeriod;
    TsTsFwProfileConfigT configP;
    (void)memset_s(&configP, sizeof(TsTsFwProfileConfigT), 0, sizeof(TsTsFwProfileConfigT));
    configP.period = (uint32_t)profSamplePeriod;
    int ret = DrvSetTsCommandType(configP, profileParams);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Begin to start profiling DrvTsFwStart, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));
    MSPROF_LOGI("DrvTsFwStart profDeviceId=%d, profChannel=%d, timeLine=%u,"
        " keyPoint=%u, memCpy=%u", profDeviceId, static_cast<int>(profChannel),
        configP.ts_timeline,
        configP.ts_keypoint, configP.ts_memcpy);
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = (unsigned int)peripheralCfg.profSamplePeriod;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = &configP;
    profStartPara.user_data_size = sizeof(TsTsFwProfileConfigT);
    ret = DriverPlugin::instance()->MsprofDrvStart(profDeviceId, profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvTsFwStart, profDeviceId=%d, profChannel=%d, ret=%d",
                    profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvTsFwStart, profDeviceId=%d, profChannel=%d",
                 profDeviceId, static_cast<int>(profChannel));
    return PROFILING_SUCCESS;
}

int DrvStarsSocLogStart(const DrvPeripheralProfileCfg &peripheralCfg,
                        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams)
{
    if (profileParams == nullptr) {
        return PROFILING_FAILED;
    }
    int32_t profDeviceId = peripheralCfg.profDeviceId;
    AI_DRV_CHANNEL profChannel = peripheralCfg.profChannel;
    StarsSocLogConfigT configP;
    (void)memset_s(&configP, sizeof(StarsSocLogConfigT), 0, sizeof(StarsSocLogConfigT));

    if (profileParams->stars_acsq_task.compare(analysis::dvvp::common::config::MSVP_PROF_ON) == 0) {
        configP.acsq_task = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP.ffts_thread_task = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP.acc_pmu = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
    }
    MSPROF_LOGI("DrvStarsSocLogStart profDeviceId=%d, profChannel=%d, acsq_task=%u, acc_pmu=%u, cdqm_reg=%u,"
        "dvpp_vpc_block=%u, dvpp_jpegd_block=%u, dvpp_jpede_block=%u" "ffts_thread_task=%u, sdma_dmu=%u",
        profDeviceId, static_cast<int>(profChannel), configP.acsq_task, configP.acc_pmu, configP.cdqm_reg,
        configP.dvpp_vpc_block, configP.dvpp_jpegd_block, configP.dvpp_jpede_block,
        configP.ffts_thread_task, configP.sdma_dmu);

    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = static_cast<unsigned int>(peripheralCfg.profSamplePeriod);
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = &configP;
    profStartPara.user_data_size = sizeof(StarsSocLogConfigT);
    int ret = DriverPlugin::instance()->MsprofDrvStart(profDeviceId, profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvStarsSocLogStart, profDeviceId=%d, profChannel=%d, ret=%d",
                    profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvStarsSocLogStart, profDeviceId=%d, profChannel=%d",
                 profDeviceId, static_cast<int>(profChannel));
    return PROFILING_SUCCESS;
}

void DrvPackPmuParam(int mode, FftsProfileConfigT &configP, const DrvPeripheralProfileCfg &peripheralCfg,
    const std::vector<int> &aiCores, const std::vector<std::string> &aiEvents)
{
    configP.aiEventCfg[mode].type = peripheralCfg.aicMode;
    configP.aiEventCfg[mode].period = (mode == FFTS_PROF_MODE_AIC) ?
        peripheralCfg.profSamplePeriod : peripheralCfg.profSamplePeriodHi;

    for (uint32_t i = 0; i < aiCores.size(); i++) {
        configP.aiEventCfg[mode].core_mask |= ((uint32_t)1 << (uint32_t)aiCores[i]);
    }

    std::string eventStr;
    configP.aiEventCfg[mode].event_num = (uint32_t)aiEvents.size();
    for (uint32_t i = 0; i < aiEvents.size(); i++) {
        configP.aiEventCfg[mode].event[i] = (uint32_t)strtol(aiEvents[i].c_str(), nullptr, STRING_TO_LONG_WEIGHT);
        eventStr.append(aiEvents[i] + ",");
    }
    MSPROF_EVENT("DrvPackPmuParam, mode:%d, period=%d, event_num=%d, events=%s", mode, configP.aiEventCfg[mode].period,
        configP.aiEventCfg[mode].event_num, eventStr.c_str());
}

int DrvFftsProfileStart(const DrvPeripheralProfileCfg &peripheralCfg, const std::vector<int> &aicCores,
                        const std::vector<std::string> &aicEvents, const std::vector<int> &aivCores,
                        const std::vector<std::string> &aivEvents)
{
    uint32_t profDeviceId = static_cast<uint32_t>(peripheralCfg.profDeviceId);
    AI_DRV_CHANNEL profChannel = peripheralCfg.profChannel;
    uint32_t configSize = sizeof(FftsProfileConfigT);

    auto configP = static_cast<FftsProfileConfigT *>(malloc(configSize));
    if (configP == nullptr) {
        return PROFILING_FAILED;
    }
    (void)memset_s(configP, configSize, 0, configSize);
    configP->cfgMode = peripheralCfg.cfgMode; // 0-none,1-aic,2-aiv,3-aic&aiv

    DrvPackPmuParam(FFTS_PROF_MODE_AIC, *configP, peripheralCfg, aicCores, aicEvents);
    DrvPackPmuParam(FFTS_PROF_MODE_AIV, *configP, peripheralCfg, aivCores, aivEvents);

    MSPROF_EVENT("DrvFftsProfileStart : cfgMode(%u)", configP->cfgMode);
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart(profDeviceId, profChannel, &profStartPara);
    free(configP);
    configP = nullptr;
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvFftsProfileStart, profDeviceId=%d, profChannel=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvFftsProfileStart, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));
    return PROFILING_SUCCESS;
}

int DrvBiuProfileStart(const uint32_t devId, const AI_DRV_CHANNEL channelId, const uint32_t sampleCycle)
{
    BiuProfileConfigT config;
    config.period = sampleCycle;
    MSPROF_EVENT("Begin to start profiling DrvBiuProfileStart: profDeviceId=%u, profChannel=%u, "
        "period=%u", devId, static_cast<uint32_t>(channelId), config.period);

    prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = &config;
    profStartPara.user_data_size = sizeof(config);
    int32_t ret = DriverPlugin::instance()->MsprofDrvStart(devId, channelId, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvBiuProfileStart, ret=%d, profDeviceId=%u, profChannel=%u, "
            "period=%u", ret, devId, static_cast<uint32_t>(channelId), config.period);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvBiuProfileStart, profDeviceId=%u, profChannel=%u, "
        "period=%u", devId, static_cast<uint32_t>(channelId), config.period);
    return PROFILING_SUCCESS;
}

int DrvHwtsLogStart(int profDeviceId, AI_DRV_CHANNEL profChannel)
{
    unsigned int configSize = sizeof(TsHwtsProfileConfigT);
    TsHwtsProfileConfigT configP;
    (void)memset_s(&configP, configSize, 0, configSize);
    MSPROF_EVENT("Begin to start profiling DrvHwtsLogStart, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = &configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart((uint32_t)profDeviceId, profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvHwtsLogStart, profDeviceId=%d,"
            " profChannel=%d, ret=%d",
            profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to start profiling DrvHwtsLogStart, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));

    return PROFILING_SUCCESS;
}

int DrvFmkDataStart(int devId, AI_DRV_CHANNEL profChannel)
{
    unsigned int configSize = sizeof(TsHwtsProfileConfigT);

    TsHwtsProfileConfigT configP;
    (void)memset_s(&configP, configSize, 0, configSize);
    MSPROF_EVENT("Begin to start profiling DrvFmkDataStart, devId=%d, profChannel=%d",
        devId, static_cast<int>(profChannel));
    struct prof_start_para profStartPara;
    profStartPara.channel_type = PROF_TS_TYPE;
    profStartPara.sample_period = 0;
    profStartPara.real_time = PROFILE_REAL_TIME;
    profStartPara.user_data = &configP;
    profStartPara.user_data_size = configSize;
    int ret = DriverPlugin::instance()->MsprofDrvStart((uint32_t)devId, profChannel, &profStartPara);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to start profiling DrvFmkDataStart, devId=%d, profChannel=%d, ret=%d",
                    devId, static_cast<int>(profChannel), static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_EVENT("Succeeded to start profiling DrvFmkDataStart, devId=%d, profChannel=%d",
                 devId, static_cast<int>(profChannel));

    return PROFILING_SUCCESS;
}

int DrvStop(int profDeviceId,
            AI_DRV_CHANNEL profChannel)
{
    MSPROF_EVENT("Begin to stop profiling prof_stop DrvStop, profDeviceId=%d, profChannel=%d",
        profDeviceId, static_cast<int>(profChannel));
    int ret = DriverPlugin::instance()->MsprofDrvStop((uint32_t)profDeviceId, profChannel);
    if (ret != PROF_OK) {
        MSPROF_LOGE("Failed to stop profiling prof_stop DrvStop, profDeviceId=%d, profChannel=%d, ret=%d",
                    profDeviceId, static_cast<int>(profChannel), ret);
        return PROFILING_FAILED;
    }
    MSPROF_EVENT("Succeeded to stop profiling prof_stop DrvStop, profDeviceId=%d, profChannel=%d",
                 profDeviceId, static_cast<int>(profChannel));

    return PROFILING_SUCCESS;
}

int DrvChannelRead(int profDeviceId,
                   AI_DRV_CHANNEL profChannel,
                   UNSIGNED_CHAR_PTR outBuf,
                   uint32_t bufSize)
{
    if (outBuf == nullptr) {
        MSPROF_LOGE("outBuf is nullptr");
        return PROFILING_FAILED;
    }
    int ret = DriverPlugin::instance()->MsprofChannelRead((uint32_t)profDeviceId, profChannel,
        reinterpret_cast<CHAR_PTR>(outBuf), bufSize);
    if (ret < 0) {
        if (ret == PROF_STOPPED_ALREADY) {
            MSPROF_LOGW("profChannel has stopped already, profDeviceId=%d, profChannel=%d, bufSize=%d",
                        profDeviceId, static_cast<int>(profChannel), bufSize);
            return 0; // data len is 0
        }
        MSPROF_LOGE("Failed to prof_channel_read, profDeviceId=%d, profChannel=%d, bufSize=%d, ret=%d",
                    profDeviceId, static_cast<int>(profChannel), bufSize, ret);
        return PROFILING_FAILED;
    }

    return ret;
}

int DrvChannelPoll(PROF_POLL_INFO_PTR outBuf,
                   int num,
                   int timeout)
{
    if (outBuf == nullptr) {
        MSPROF_LOGE("outBuf is nullptr");
        return PROFILING_FAILED;
    }
    int ret = DriverPlugin::instance()->MsprofChannelPoll(outBuf, num, timeout);
    if (ret == PROF_ERROR || ret > num) {
        MSPROF_LOGE("Failed to prof_channel_poll, num=%d, timeout=%d, ret=%d", num, timeout, ret);
        return PROFILING_FAILED;
    }

    return ret;
}

int DrvProfFlush(unsigned int deviceId, unsigned int channelId, unsigned int &bufSize)
{
#if (defined(linux) || defined(__linux__))
    if (!DriverPlugin::instance()->IsFuncExist("halProfDataFlush")) {
        MSPROF_LOGW("Function halProfDataFlush not supported, deviceId=%u, channelId=%u", deviceId, channelId);
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Begin to flush drv buff. deviceId=%u, channelId=%u", deviceId, channelId);
    int ret = DriverPlugin::instance()->MsprofHalProfDataFlush(deviceId, channelId, &bufSize);
    if (ret == PROF_OK) {
        MSPROF_LOGI("End to flush drv buff. deviceId=%u, channelId=%u, bufSize=%u", deviceId, channelId, bufSize);
        return PROFILING_SUCCESS;
    }
    if (ret == PROF_STOPPED_ALREADY) {
        MSPROF_LOGE("Failed to halProfDataFlush, channel already closed.deviceId=%u, channelId=%u",
            deviceId, channelId);
        MSPROF_CALL_ERROR("EK9999", "Failed to halProfDataFlush, channel already closed.deviceId=%u, channelId=%u",
            deviceId, channelId);
        return PROFILING_FAILED;
    } else {
        MSPROF_LOGE("Failed to halProfDataFlush.deviceId=%u, channelId=%u, ret=%d", deviceId, channelId, ret);
        MSPROF_CALL_ERROR("EK9999", "Failed to halProfDataFlush.deviceId=%u, channelId=%u, ret=%d",
            deviceId, channelId, ret);
        return PROFILING_FAILED;
    }
#else
    MSPROF_LOGW("Function halProfDataFlush not supported, deviceId=%u, channelId=%u", deviceId, channelId);
    return PROFILING_FAILED;
#endif
}
}  // namespace driver
}  // namespace dvvp
}  // namespace analysis
