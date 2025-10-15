/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_prof_task.cpp
 * Description        : Collection Job.
 * Author             : msprof team
 * Creation Date      : 2024/07/27
 * *****************************************************************************
*/


#include "activity/ascend/dev_prof_task.h"
#include "activity/ascend/channel/channel_pool_manager.h"
#include "common/inject/driver_inject.h"
#include "common/plog_manager.h"
#include "common/context_manager.h"

#include "securec.h"

namespace Mspti {
namespace Ascend {

const std::map<Mspti::Common::PlatformType, std::map<msptiActivityKind, std::set<AI_DRV_CHANNEL>>>
DevProfTaskFactory::kindToChannel_map_ = {
    {
        Mspti::Common::PlatformType::CHIP_910B, {
            {MSPTI_ACTIVITY_KIND_MARKER, {PROF_CHANNEL_TS_FW}},
            {MSPTI_ACTIVITY_KIND_KERNEL, {PROF_CHANNEL_TS_FW, PROF_CHANNEL_STARS_SOC_LOG}},
            {MSPTI_ACTIVITY_KIND_HCCL, {PROF_CHANNEL_TS_FW}},
            {MSPTI_ACTIVITY_KIND_COMMUNICATION, {PROF_CHANNEL_TS_FW, PROF_CHANNEL_STARS_SOC_LOG}},
        }
    },
    {
        Mspti::Common::PlatformType::CHIP_310B, {
            {MSPTI_ACTIVITY_KIND_MARKER, {PROF_CHANNEL_TS_FW}},
            {MSPTI_ACTIVITY_KIND_KERNEL, {PROF_CHANNEL_TS_FW, PROF_CHANNEL_STARS_SOC_LOG}},
            {MSPTI_ACTIVITY_KIND_HCCL, {PROF_CHANNEL_TS_FW}},
            {MSPTI_ACTIVITY_KIND_COMMUNICATION, {PROF_CHANNEL_TS_FW, PROF_CHANNEL_STARS_SOC_LOG}},
        }
    }
};

std::unique_ptr<DevProfTask> DevProfTaskFactory::CreateDevChannelTask(uint32_t deviceId, AI_DRV_CHANNEL channelId)
{
    switch (channelId) {
        case PROF_CHANNEL_TS_FW:
            return std::make_unique<DevProfTaskTsFw>(deviceId);
            break;
        case PROF_CHANNEL_STARS_SOC_LOG:
            return std::make_unique<DevProfTaskStars>(deviceId);
            break;
        default:
            return std::make_unique<DevProfTaskDefault>(deviceId);
            break;
    }
}

std::vector<std::unique_ptr<DevProfTask>> DevProfTaskFactory::CreateTasks(uint32_t deviceId, msptiActivityKind kind)
{
    std::vector<std::unique_ptr<DevProfTask>> profTasks;
    auto platform = Mspti::Common::ContextManager::GetInstance()->GetChipType(deviceId);
    auto devIter = kindToChannel_map_.find(platform);
    if (devIter == kindToChannel_map_.end()) {
        MSPTI_LOGE("The platform: %d of device: %u is not support.", static_cast<int>(platform), deviceId);
        return profTasks;
    }
    auto kindIter = devIter->second.find(kind);
    if (kindIter == devIter->second.end()) {
        MSPTI_LOGW("The kind: %d of device: %u is not support.", kind, deviceId);
        return profTasks;
    }
    const auto& channelTypes = kindIter->second;
    for (const auto& channelType : channelTypes) {
        auto task = CreateDevChannelTask(deviceId, channelType);
        profTasks.emplace_back(std::move(task));
    }
    return profTasks;
}

msptiResult DevProfTask::Start()
{
    if (!t_.joinable()) {
        StartTask();
        t_ = std::thread(std::bind(&DevProfTask::Run, this));
    }
    return MSPTI_SUCCESS;
}

msptiResult DevProfTask::Stop()
{
    {
        std::unique_lock<std::mutex> lck(cv_mtx_);
        task_run_ = true;
        cv_.notify_one();
    }
    if (t_.joinable()) {
        t_.join();
    }
    return MSPTI_SUCCESS;
}

void DevProfTask::Run()
{
    pthread_setname_np(pthread_self(), "DevProfTask");
    {
        std::unique_lock<std::mutex> lk(cv_mtx_);
        cv_.wait(lk, [&] () {
            return task_run_;
        });
    }
    StopTask();
}

// DevProfTaskTsFw的引用计数，保证在第一次使能时，Start Device任务
// 最后一次反使能时，Stop Device任务
std::map<uint32_t, uint32_t> DevProfTaskTsFw::ref_cnts_;
std::mutex DevProfTaskTsFw::cnt_mtx_;
msptiResult DevProfTaskTsFw::StartTask()
{
    uint32_t refCnt = 0;
    {
        std::lock_guard<std::mutex> lk(cnt_mtx_);
        auto iter = ref_cnts_.find(deviceId_);
        if (iter == ref_cnts_.end()) {
            auto ret = ref_cnts_.insert({deviceId_, refCnt});
            if (!ret.second) {
                MSPTI_LOGE("Insert tsfw task cnt failed.");
                return MSPTI_ERROR_INNER;
            }
            iter = ret.first;
        } else {
            refCnt = iter->second;
        }
        iter->second++;
    }
    if (refCnt == 0) {
        if (!Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->CheckChannelValid(deviceId_, channelId_)) {
            return MSPTI_SUCCESS;
        }
        static const uint32_t SAMPLE_PERIOD = 20;
        Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->AddReader(deviceId_, channelId_);
        TsTsFwProfileConfigT configP;
        if (memset_s(&configP, sizeof(configP), 0, sizeof(configP)) != EOK) {
            return MSPTI_ERROR_INNER;
        }
        configP.period = SAMPLE_PERIOD;
        configP.tsKeypoint = 1;

        ProfStartParaT profStartPara;
        profStartPara.channelType = PROF_CHANNEL_TYPE_TS;
        profStartPara.samplePeriod = SAMPLE_PERIOD;
        profStartPara.realTime = PROFILE_REAL_TIME;
        profStartPara.userData = &configP;
        profStartPara.userDataSize = static_cast<unsigned int>(sizeof(TsTsFwProfileConfigT));
        int ret = ProfDrvStart(deviceId_, channelId_, &profStartPara);
        if (ret != 0) {
            MSPTI_LOGE("Failed to start TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
            return MSPTI_ERROR_INNER;
        }
        MSPTI_LOGI("Succeed to start TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
    }
    return MSPTI_SUCCESS;
}

msptiResult DevProfTaskTsFw::StopTask()
{
    uint32_t refCnt = 0;
    {
        std::lock_guard<std::mutex> lk(cnt_mtx_);
        auto iter = ref_cnts_.find(deviceId_);
        if (iter == ref_cnts_.end()) {
            MSPTI_LOGW("The TsFw DevProfTask was not start. DeviceId: %u", deviceId_);
            return MSPTI_SUCCESS;
        }
        refCnt = --iter->second;
    }
    if (refCnt == 0) {
        int ret = ProfStop(deviceId_, channelId_);
        if (ret != 0) {
            MSPTI_LOGE("Failed to stop TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
            return MSPTI_ERROR_INNER;
        }
        Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->RemoveReader(deviceId_, channelId_);
        MSPTI_LOGI("Succeed to stop TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
    }
    return MSPTI_SUCCESS;
}

// DevProfTaskStars的引用计数，保证在第一次使能时，Start Device任务
// 最后一次反使能时，Stop Device任务
std::map<uint32_t, uint32_t> DevProfTaskStars::ref_cnts_;
std::mutex DevProfTaskStars::cnt_mtx_;
msptiResult DevProfTaskStars::StartTask()
{
    uint32_t refCnt = 0;
    {
        std::lock_guard<std::mutex> lk(cnt_mtx_);
        auto iter = ref_cnts_.find(deviceId_);
        if (iter == ref_cnts_.end()) {
            auto ret = ref_cnts_.insert({deviceId_, refCnt});
            if (!ret.second) {
                MSPTI_LOGE("Insert stars task cnt failed.");
                return MSPTI_ERROR_INNER;
            }
            iter = ret.first;
        } else {
            refCnt = iter->second;
        }
        iter->second++;
    }
    if (refCnt == 0) {
        if (!Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->CheckChannelValid(deviceId_, channelId_)) {
            return MSPTI_SUCCESS;
        }
        Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->AddReader(deviceId_, channelId_);
        StarsSocLogConfigT configP;
        if (memset_s(&configP, sizeof(StarsSocLogConfigT), 0, sizeof(StarsSocLogConfigT)) != EOK) {
            return MSPTI_ERROR_INNER;
        }
        configP.acsq_task = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        configP.ffts_thread_task = TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE;
        ProfStartParaT profStartPara;
        profStartPara.channelType = PROF_CHANNEL_TYPE_TS;
        static const uint32_t SAMPLE_PERIOD = 20;
        profStartPara.samplePeriod = SAMPLE_PERIOD;
        profStartPara.realTime = PROFILE_REAL_TIME;
        profStartPara.userData = &configP;
        profStartPara.userDataSize = static_cast<unsigned int>(sizeof(StarsSocLogConfigT));
        int ret = ProfDrvStart(deviceId_, channelId_, &profStartPara);
        if (ret != 0) {
            MSPTI_LOGE("Failed to start ProfStarsJob for device: %u, channel id: %u", deviceId_, channelId_);
            return MSPTI_ERROR_INNER;
        }
        MSPTI_LOGI("Succeed to start ProfStarsJob for device: %u, channel id: %u", deviceId_, channelId_);
    }
    return MSPTI_SUCCESS;
}

msptiResult DevProfTaskStars::StopTask()
{
    uint32_t refCnt = 0;
    {
        std::lock_guard<std::mutex> lk(cnt_mtx_);
        auto iter = ref_cnts_.find(deviceId_);
        if (iter == ref_cnts_.end()) {
            MSPTI_LOGW("The Stars DevProfTask was not start. DeviceId: %u", deviceId_);
            return MSPTI_SUCCESS;
        }
        refCnt = --iter->second;
    }
    if (refCnt == 0) {
        int ret = ProfStop(deviceId_, channelId_);
        if (ret != 0) {
            MSPTI_LOGE("Failed to stop ProfStarsJob for device: %u, channel id: %u", deviceId_, channelId_);
            return MSPTI_ERROR_INNER;
        }
        Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->RemoveReader(deviceId_, channelId_);
        MSPTI_LOGI("Succeed to stop ProfStarsJob for device: %u, channel id: %u", deviceId_, channelId_);
    }
    return MSPTI_SUCCESS;
}

}  // Ascend
}  // Mspti
