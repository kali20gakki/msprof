/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: handle prof channel poll
 * Author: dml
 * Create: 2019-12-07
 */
#ifndef ANALYSIS_DVVP_PROF_CHANNEL_MANAGER_H
#define ANALYSIS_DVVP_PROF_CHANNEL_MANAGER_H

#include <mutex>
#include "singleton/singleton.h"
#include "transport/prof_channel.h"


namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
class ProfChannelManager : public analysis::dvvp::common::singleton::Singleton<ProfChannelManager> {
    friend analysis::dvvp::common::singleton::Singleton<ProfChannelManager>;
public:
    ProfChannelManager();
    virtual ~ProfChannelManager();
    int Init();
    void UnInit(bool isReset = false);
    SHARED_PTR_ALIA<analysis::dvvp::transport::ChannelPoll> GetChannelPoller();
    void FlushChannel();

private:
    SHARED_PTR_ALIA<analysis::dvvp::transport::ChannelPoll> drvChannelPoll_;
    std::mutex channelPollMutex_;
    uint64_t index_;
};
}}}

#endif