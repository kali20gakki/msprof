/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_pool_manager.h
 * Description        : Manager channel pool.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_MANAGER_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_MANAGER_H

#include <memory>
#include <mutex>

#include "activity/ascend/channel/channel_pool.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

class ChannelPoolManager {
public:
    static ChannelPoolManager* GetInstance();
    msptiResult Init();
    void UnInit();
    msptiResult AddReader(uint32_t devId, AI_DRV_CHANNEL channelId);
    msptiResult RemoveReader(uint32_t devId, AI_DRV_CHANNEL channelId);

private:
    ChannelPoolManager() = default;
    ~ChannelPoolManager();
    explicit ChannelPoolManager(const ChannelPoolManager &obj) = delete;
    ChannelPoolManager& operator=(const ChannelPoolManager &obj) = delete;
    explicit ChannelPoolManager(ChannelPoolManager &&obj) = delete;
    ChannelPoolManager& operator=(ChannelPoolManager &&obj) = delete;

private:
    std::unique_ptr<ChannelPool> drvChannelPoll_;
    std::mutex channelPollMutex_;
};
}  // Channel
}  // Ascend
}  // Mspti

#endif
