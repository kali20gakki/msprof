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

#include "csrc/activity/ascend/channel/channel_pool_manager.h"
#include "csrc/common/inject/driver_inject.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "securec.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

namespace {
    constexpr uint32_t MAX_DEV_NUM = 64;
}

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
    if (ret != DRV_ERROR_NONE || dev_num == 0) {
        MSPTI_LOGE("Failed to get dev num, ret: %d, num: %u", ret, dev_num);
        return MSPTI_ERROR_DEVICE_OFFLINE;
    }

    if (dev_num > MAX_DEV_NUM) {
        MSPTI_LOGW("dev_num is too big: %u, reset to %u", dev_num, MAX_DEV_NUM);
        dev_num = MAX_DEV_NUM;
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

msptiResult ChannelPoolManager::GetAllChannels(uint32_t devId)
{
    std::lock_guard<std::mutex> lock(channels_mtx_);
    ChannelListT channelList;
    if (memset_s(&channelList, sizeof(ChannelListT), 0, sizeof(ChannelListT)) != EOK) {
        MSPTI_LOGE("DrvGetChannels, memset failed");
        return MSPTI_ERROR_INNER;
    }
    ProfDrvGetChannels(devId, &channelList);
    if (channelList.channelNum > PROF_CHANNEL_NUM_MAX || channelList.channelNum == 0) {
        MSPTI_LOGE("ProfDrvGetChannels failed. Channel Num: %d.", channelList.channelNum);
        return MSPTI_ERROR_INNER;
    }
    for (size_t i = 0; i < channelList.channelNum; ++i) {
        channels_[devId].insert(channelList.channel[i].channelId);
    }
    return MSPTI_SUCCESS;
}

bool ChannelPoolManager::CheckChannelValid(uint32_t devId, uint32_t channelId)
{
    std::lock_guard<std::mutex> lock(channels_mtx_);
    auto devIter = channels_.find(devId);
    if (devIter == channels_.end() || devIter->second.find(channelId) == devIter->second.end()) {
        return false;
    }
    return true;
}

msptiResult ChannelPoolManager::AddReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    if (drvChannelPoll_) {
        return drvChannelPoll_->AddReader(devId, channelId);
    }
    return MSPTI_SUCCESS;
}

msptiResult ChannelPoolManager::RemoveReader(uint32_t devId, AI_DRV_CHANNEL channelId)
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    if (drvChannelPoll_) {
        return drvChannelPoll_->RemoveReader(devId, channelId);
    }
    return MSPTI_SUCCESS;
}
}  // Channel
}  // Ascend
}  // Mspti
