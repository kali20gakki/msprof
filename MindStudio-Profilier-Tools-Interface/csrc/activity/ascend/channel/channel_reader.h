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

#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_READER_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_READER_H

#include "csrc/common/task.h"
#include "csrc/include/mspti_result.h"
#include "csrc/common/inject/inject_base.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

constexpr uint64_t MAX_BUFFER_SIZE = 1024 * 1024 * 2;

class ChannelReader : public Mspti::Common::Task {
public:
    ChannelReader(uint32_t deviceId, AI_DRV_CHANNEL channelId);
    virtual ~ChannelReader() = default;
    virtual msptiResult Execute();
    virtual size_t HashId();
    msptiResult Init();
    msptiResult Uinit();

    void SetChannelStopped();
    bool GetSchedulingStatus() const;
    void SetSchedulingStatus(bool isScheduling);

private:
    static size_t TransDataToActivityBuffer(char buffer[], size_t valid_size, uint32_t deviceId,
        AI_DRV_CHANNEL channelId);
    static size_t TransTsFwData(char buffer[], size_t valid_size, uint32_t deviceId);
    static size_t TransStarsLog(char buffer[], size_t valid_size, uint32_t deviceId);

private:
    // basic info
    uint32_t deviceId_;
    AI_DRV_CHANNEL channelId_;
    size_t hashId_{0};

    // status info
    volatile bool isInited_{false};
    volatile bool isScheduling_{false};
    volatile bool isChannelStopped_{false};
};

}  // Channel
}  // Ascend
}  // Mspti

#endif
