/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: handle prof channel poll
 * Author: dml
 * Create: 2019-12-07
 */

#include "prof_channel_manager.h"
#include "errno/error_code.h"
#include "msprof_error_manager.h"
#include "transport/prof_channel.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;

ProfChannelManager::ProfChannelManager()
    : index_(0)
{
}

ProfChannelManager::~ProfChannelManager()
{
}

int ProfChannelManager::Init()
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    index_++;
    MSPROF_LOGI("ProfChannelManager Init index:%llu", index_);
    if (drvChannelPoll_ != nullptr) {
        MSPROF_LOGI("ProfChannelManager already inited");
        return PROFILING_SUCCESS;
    }
    MSVP_MAKE_SHARED0_RET(drvChannelPoll_, ChannelPoll, PROFILING_FAILED);
    int ret = drvChannelPoll_->Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGI("drvChannelPoll start thread pool failed");
        return ret;
    }
    MSPROF_LOGI("Init Poll Succ");
    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<ChannelPoll> ProfChannelManager::GetChannelPoller()
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    return drvChannelPoll_;
}

void ProfChannelManager::UnInit(bool isReset)
{
    std::lock_guard<std::mutex> lock(channelPollMutex_);
    MSPROF_LOGI("ProfChannelManager UnInit index:%llu", index_);
    if (!isReset) {
        if (index_ == 0) {
            return;
        }
        index_--;
        if (index_ != 0) {
            return;
        }
    } else {
        index_ = 0;
    }
    if (drvChannelPoll_ != nullptr) {
        int ret = drvChannelPoll_->Stop();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("drvChannelPoll_ stop failed");
        }
        drvChannelPoll_.reset();
        drvChannelPoll_ = nullptr;
    }

    MSPROF_LOGI("UnInit Poll Succ");
}

void ProfChannelManager::FlushChannel()
{
    if (drvChannelPoll_ != nullptr) {
        drvChannelPoll_->FlushDrvBuff();
    }
}
}}}