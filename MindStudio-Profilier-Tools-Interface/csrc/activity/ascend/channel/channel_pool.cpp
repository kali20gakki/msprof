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
#include "csrc/activity/ascend/channel/channel_pool.h"

#include <cstring>
#include <functional>
#include <unistd.h>

#include "csrc/common/config.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/common/inject/driver_inject.h"
#include "securec.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

size_t ChannelPool::GetChannelIndex(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    return std::hash<std::string>()(std::to_string(devId) + "_" + std::to_string(channelId));
}

msptiResult ChannelPool::AddReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    size_t channel_index = GetChannelIndex(devId, channelId);
    std::lock_guard<std::mutex> lk(mtx_);
    if (readers_map_.find(channel_index) == readers_map_.end()) {
        std::shared_ptr<ChannelReader> reader = nullptr;
        Mspti::Common::MsptiMakeSharedPtr(reader, devId, channelId);
        if (!reader) {
            MSPTI_LOGE("Create channel reader failed.");
            return MSPTI_ERROR_INNER;
        }
        reader->Init();
        readers_map_[channel_index] = reader;
    }
    return MSPTI_SUCCESS;
}

msptiResult ChannelPool::RemoveReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    size_t channel_index = GetChannelIndex(devId, channelId);
    std::lock_guard<std::mutex> lk(mtx_);
    auto channel_iter = readers_map_.find(channel_index);
    if (channel_iter != readers_map_.end()) {
        channel_iter->second->SetChannelStopped();
        channel_iter->second->Uinit();
        readers_map_.erase(channel_index);
    }
    return MSPTI_SUCCESS;
}

msptiResult ChannelPool::Start()
{
    Mspti::Common::MsptiMakeUniquePtr(threadPool_, Mspti::Common::LOAD_BALANCE_METHOD::ID_MOD, pool_size_);
    if (!threadPool_) {
        MSPTI_LOGE("Failed to init ThreadPool.");
        return MSPTI_ERROR_INNER;
    }
    threadPool_->SetThreadPoolQueueSize(CHANNELPOLL_THREAD_QUEUE_SIZE);
    threadPool_->Start();
    if (!thread_.joinable()) {
        isStarted_ = true;
        thread_ = std::thread(std::bind(&ChannelPool::Run, this));
    }
    return MSPTI_SUCCESS;
}

void ChannelPool::Stop()
{
    if (isStarted_) {
        isStarted_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    if (threadPool_) {
        threadPool_->Stop();
        threadPool_.reset(nullptr);
    }
}

void ChannelPool::Run()
{
    pthread_setname_np(pthread_self(), "MsptiChannelPool");
    static const int CHANNEL_POOL_NUM = 6; // at most get 6 channels to read once loop
    static const int DEFAULT_TIMEOUT_SEC = 1; // at most wait for 1 seconds
    static constexpr size_t channel_info_size = CHANNEL_POOL_NUM * sizeof(struct ProfPollInfo);
    struct ProfPollInfo channels[CHANNEL_POOL_NUM];
    if (memset_s(channels, channel_info_size, 0, channel_info_size) != EOK) {
        MSPTI_LOGE("Init channels failed, channelInfoSize is %zu", channel_info_size);
        return;
    }

    while (isStarted_) {
        int ret = ProfChannelPoll(channels, CHANNEL_POOL_NUM, DEFAULT_TIMEOUT_SEC);
        if (ret == CHANNEL_PROF_ERROR) {
            break;
        }
        if (ret == CHANNEL_PROF_STOPPED_ALREADY) {
            if (isStarted_ == false) {
                break;
            } else {
                constexpr uint32_t sleepTimeUs = 1000;
                usleep(sleepTimeUs);
                continue;
            }
        }
        if (ret < 0 || ret > CHANNEL_POOL_NUM) {
            MSPTI_LOGE("Failed to ProfChannelPoll, CHANNEL_POOL_NUM=%d, ret=%d", CHANNEL_POOL_NUM, ret);
            break;
        }
        for (int ii = 0; ii < ret; ++ii) {
            DispatchChannel(channels[ii].deviceId, static_cast<AI_DRV_CHANNEL>(channels[ii].channelId));
        }
    }
}

void ChannelPool::DispatchChannel(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    if (threadPool_ == nullptr) {
        MSPTI_LOGE("ThreadPool is not initialized ( nullptr ).");
        return;
    }
    size_t channel_index = GetChannelIndex(devId, channelId);
    std::lock_guard<std::mutex> lk(mtx_);
    auto channel_iter = readers_map_.find(channel_index);
    if (channel_iter != readers_map_.end()) {
        bool isScheduling = channel_iter->second->GetSchedulingStatus();
        if (!isScheduling) {
            channel_iter->second->SetSchedulingStatus(true);
            (void)threadPool_->Dispatch(channel_iter->second);
        }
    }
}
}  // Channel
}  // Ascend
}  // Mspti
