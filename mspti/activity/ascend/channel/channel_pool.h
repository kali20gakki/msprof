/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_pool.h
 * Description        : Get available channels by driver.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_H

#include <mutex>
#include <thread>
#include <unordered_map>

#include "activity/ascend/channel/channel_reader.h"
#include "common/thread_pool.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

const size_t CHANNELPOLL_THREAD_QUEUE_SIZE = 8192;
class ChannelPool {
public:
    ChannelPool(uint32_t pool_size) : pool_size_(pool_size) {}
    ~ChannelPool() = default;
    msptiResult AddReader(uint32_t devId, AI_DRV_CHANNEL channelId);
    msptiResult RemoveReader(uint32_t devId, AI_DRV_CHANNEL channelId);
    msptiResult Start();
    void Stop();

private:
    void Run();
    void DispatchChannel(uint32_t devId, AI_DRV_CHANNEL channelId);
    size_t GetChannelIndex(uint32_t devId, AI_DRV_CHANNEL channelId);

private:
    uint32_t pool_size_;
    volatile bool isStarted_{false};
    std::thread thread_;
    std::unique_ptr<Mspti::Common::ThreadPool> threadPool_;
    std::unordered_map<size_t, std::shared_ptr<ChannelReader>> readers_map_;
    std::mutex mtx_;
};
}  // Channel
}  // Ascend
}  // Mspti

#endif
