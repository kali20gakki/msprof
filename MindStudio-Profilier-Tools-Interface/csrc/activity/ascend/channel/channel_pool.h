/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/
#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_H

#include <mutex>
#include <thread>
#include <unordered_map>

#include "csrc/activity/ascend/channel/channel_reader.h"
#include "csrc/common/thread_pool.h"
#include "csrc/include/mspti_result.h"

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
