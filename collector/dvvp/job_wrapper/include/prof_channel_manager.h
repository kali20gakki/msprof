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
 * -------------------------------------------------------------------------*/
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