/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_pool_manager.cpp
 * Description        : Manager channel pool.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "activity/ascend/channel/channel_pool_manager.h"
#include "common/inject/driver_inject.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

ChannelPoolManager *ChannelPoolManager::GetInstance()
{
    static ChannelPoolManager instance;
    return &instance;
}

ChannelPoolManager::~ChannelPoolManager()
{
    if (drvChannelPoll_ != nullptr) {
        drvChannelPoll_->Stop();
        drvChannelPoll_.reset(nullptr);
    }
}

msptiResult ChannelPoolManager::Init()
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    if (drvChannelPoll_ != nullptr) {
        return MSPTI_SUCCESS;
    }
    uint32_t dev_num = 0;
    auto ret = DrvGetDevNum(&dev_num);
    if (ret != DRV_ERROR_NONE) {
        return MSPTI_ERROR_DEVICE_OFFLINE;
    }
    Mspti::Common::MsptiMakeUniquePtr(drvChannelPoll_, dev_num);
    if (!drvChannelPoll_) {
        MSPTI_LOGE("Failed to init ChannelPool.");
        return MSPTI_ERROR_INNER;
    }
    return drvChannelPoll_->Start();
}

void ChannelPoolManager::UnInit()
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    if (drvChannelPoll_ != nullptr) {
        drvChannelPoll_->Stop();
        drvChannelPoll_.reset(nullptr);
    }
}

msptiResult ChannelPoolManager::AddReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    if (drvChannelPoll_) {
        return drvChannelPoll_->AddReader(devId, channelId);
    }
    return MSPTI_SUCCESS;
}

msptiResult ChannelPoolManager::RemoveReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    if (drvChannelPoll_) {
        return drvChannelPoll_->RemoveReader(devId, channelId);
    }
    return MSPTI_SUCCESS;
}
}  // Channel
}  // Ascend
}  // Mspti
