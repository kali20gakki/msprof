/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_prof_job.cpp
 * Description        : Collection Job.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "activity/ascend/dev_prof_job.h"

#include <cstring>

#include "activity/ascend/channel/channel_pool_manager.h"
#include "common/inject/driver_inject.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "securec.h"

namespace Mspti {
namespace Ascend {

std::shared_ptr<DevProfJob> CollectionJobFactory::CreateJob(uint32_t deviceId, msptiActivityKind kind)
{
    if (kind == MSPTI_ACTIVITY_KIND_MARK) {
        std::shared_ptr<ProfTsTrackJob> ret = nullptr;
        Mspti::Common::MsptiMakeSharedPtr(ret, deviceId);
        return ret;
    }
    return nullptr;
}

msptiResult DevProfJob::AddReader(uint32_t deviceId, AI_DRV_CHANNEL channelId)
{
    return Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->AddReader(deviceId, channelId);
}

msptiResult DevProfJob::RemoveReader(uint32_t deviceId, AI_DRV_CHANNEL channelId)
{
    return Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->RemoveReader(deviceId, channelId);
}

msptiResult ProfTsTrackJob::Start()
{
    static const uint32_t SAMPLE_PERIOD = 20;
    DevProfJob::AddReader(deviceId_, channelId_);
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

msptiResult ProfTsTrackJob::Stop()
{
    int ret = ProfStop(deviceId_, channelId_);
    if (ret != 0) {
        MSPTI_LOGE("Failed to stop TsTrackJob for device: %u, channel id: %u", deviceId_, channelId_);
        return MSPTI_ERROR_INNER;
    }
    DevProfJob::RemoveReader(deviceId_, channelId_);
    return MSPTI_SUCCESS;
}

}  // Ascend
}  // Mspti