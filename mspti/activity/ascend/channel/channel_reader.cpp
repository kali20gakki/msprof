/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_reader.cpp
 * Description        : read data from channel and decode data.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "activity/ascend/channel/channel_reader.h"

#include <cstring>

#include "activity/activity_manager.h"
#include "activity/ascend/channel/channel_data.h"
#include "common/context_manager.h"
#include "common/inject/driver_inject.h"
#include "common/inject/inject_base.h"
#include "common/utils.h"
#include "external/mspti_activity.h"
#include "securec.h"

namespace Mspti {
namespace Ascend {
namespace Channel {

ChannelReader::ChannelReader(uint32_t deviceId, AI_DRV_CHANNEL channelId)
    : deviceId_(deviceId),
      channelId_(channelId) {}

msptiResult ChannelReader::Init()
{
    hashId_ = std::hash<std::string>()(std::to_string(static_cast<int>(channelId_)) +
        std::to_string(deviceId_) +
        std::to_string(Mspti::Common::Utils::GetClockMonotonicRawNs()));
    isInited_ = true;
    return MSPTI_SUCCESS;
}

msptiResult ChannelReader::Uinit()
{
    isInited_ = false;
    return MSPTI_SUCCESS;
}

size_t ChannelReader::HashId()
{
    return hashId_;
}

void ChannelReader::SetChannelStopped()
{
    isChannelStopped_ = true;
}

bool ChannelReader::GetSchedulingStatus() const
{
    return isScheduling_;
}

void ChannelReader::SetSchedulingStatus(bool isScheduling)
{
    isScheduling_ = isScheduling;
}

msptiResult ChannelReader::Execute()
{
    isScheduling_ = false;
    char buf[MAX_BUFFER_SIZE] = {0};
    size_t cur_pos = 0;
    int currLen = 0;
    while (isInited_ && !isChannelStopped_) {
        currLen = ProfChannelRead(deviceId_, channelId_, buf + cur_pos, MAX_BUFFER_SIZE - cur_pos);
        if (currLen <= 0) {
            if (currLen < 0) {
                printf("[ERROR]Get Data from Driver failed.\n");
            }
            break;
        }
        size_t last_pos = TransDataToActivityBuffer(buf, cur_pos + currLen, deviceId_, channelId_);
        if (last_pos < cur_pos + currLen) {
            if (memcpy_s(buf, MAX_BUFFER_SIZE, buf + last_pos, cur_pos + currLen - last_pos) != EOK) {
                break;
            }
        }
        cur_pos = cur_pos + currLen - last_pos;
    }
}

size_t ChannelReader::TransDataToActivityBuffer(char buffer[], size_t valid_size,
    uint32_t deviceId, AI_DRV_CHANNEL channelId)
{
    switch (channelId) {
        case PROF_CHANNEL_TS_FW:
            return TransTsFwData(buffer, valid_size, deviceId);
        default:
            return 0;
    }
}

size_t ChannelReader::TransTsFwData(char buffer[], size_t valid_size, uint32_t deviceId)
{
    size_t pos = 0;
    constexpr uint32_t MIN_TSFW_SIZE = 32;
    constexpr uint32_t RPT_TYPE_POS = 1;
    constexpr uint32_t BUF_SIZE_POS = 2;
    constexpr uint32_t TIMESTAMP_POS = 8;
    constexpr uint32_t MARKID_POS = 16;
    constexpr uint32_t STREAMID_POS = 32;
    while (valid_size - pos >= MIN_TSFW_SIZE) {
        uint8_t *mode = reinterpret_cast<uint8_t*>(buffer + pos);
        uint8_t *rptType = reinterpret_cast<uint8_t*>(buffer + pos + RPT_TYPE_POS);
        uint16_t *bufSize = reinterpret_cast<uint16_t*>(buffer + pos + BUF_SIZE_POS);
        TsTrackRptType type = static_cast<TsTrackRptType>(*rptType);
        switch (type) {
            case RPT_TYPE_STEP_TRACE:
                msptiActivityMark activity;
                activity.kind = MSPTI_ACTIVITY_KIND_MARK;
                activity.mode = 1;
                activity.timestamp = *reinterpret_cast<uint64_t*>(buffer + pos + TIMESTAMP_POS);
                activity.timestamp = Mspti::Common::ContextManager::GetInstance()->GetMonotomicFromSysCnt(deviceId,
                    activity.timestamp);
                activity.markId = *reinterpret_cast<uint64_t*>(buffer + pos + MARKID_POS);
                activity.streamId = static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(buffer + pos + STREAMID_POS));
                activity.deviceId = deviceId;
                activity.name = "";
                Mspti::Activity::ActivityManager::GetInstance()->Record(
                    reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
                break;
            default:
                break;
        }
        pos += *bufSize;
    }
    return pos;
}
}  // Channel
}  // Ascend
}  // Mspti
