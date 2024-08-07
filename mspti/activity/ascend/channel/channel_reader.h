/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_reader.h
 * Description        : read data from channel and decode data.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_READER_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_READER_H

#include "common/task.h"
#include "external/mspti_result.h"
#include "common/inject/inject_base.h"

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
