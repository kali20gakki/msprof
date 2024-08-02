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

#include "securec.h"

namespace Mspti {
namespace Ascend {

std::unique_ptr<DevProfTask> DevProfTaskFactory::CreateTask(uint32_t deviceId, msptiActivityKind kind)
{
    switch (kind) {
        case MSPTI_ACTIVITY_KIND_MARKER:
            return std::make_unique<DevProfTaskTsFw>(deviceId);
        default:
            return std::make_unique<DevProfTaskDefault>(deviceId);
    }
}

msptiResult DevProfTask::Start()
{
    if (!t_.joinable()) {
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
    StartTask();
    {
        std::unique_lock<std::mutex> lk(cv_mtx_);
        cv_.wait(lk, [&] () {
            return task_run_;
        });
    }
    StopTask();
}

msptiResult DevProfTaskTsFw::StartTask()
{
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
    profStartPara.userDataSize = sizeof(TsTsFwProfileConfigT);
    int ret = ProfDrvStart(deviceId_, channelId_, &profStartPara);
    if (ret != 0) {
        MSPTI_LOGE("Failed to start TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult DevProfTaskTsFw::StopTask()
{
    int ret = ProfStop(deviceId_, channelId_);
    if (ret != 0) {
        MSPTI_LOGE("Failed to stop TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
        return MSPTI_ERROR_INNER;
    }
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->RemoveReader(deviceId_, channelId_);
    return MSPTI_SUCCESS;
}

}  // Ascend
}  // Mspti
