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
#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_MANAGER_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_POOL_MANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include <set>

#include "csrc/activity/ascend/channel/channel_pool.h"
#include "csrc/include/mspti_result.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

class ChannelPoolManager {
public:
    static ChannelPoolManager* GetInstance();
    msptiResult Init();
    void UnInit();
    msptiResult GetAllChannels(uint32_t devId);
    bool CheckChannelValid(uint32_t devId, uint32_t channelId);
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
    std::unordered_map<uint32_t, std::set<uint32_t>> channels_;
    std::mutex channels_mtx_;
};
}  // Channel
}  // Ascend
}  // Mspti

#endif
