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
#include "activity/ascend/parser/parser_manager.h"
#include "activity/ascend/parser/device_task_calculator.h"
#include "common/context_manager.h"
#include "common/inject/driver_inject.h"
#include "common/inject/inject_base.h"
#include "common/plog_manager.h"
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
                MSPTI_LOGE("Read data from driver failed.");
            }
            break;
        }
        auto uint_currLen = static_cast<size_t>(currLen);
        if (uint_currLen >= (MAX_BUFFER_SIZE - cur_pos)) {
            MSPTI_LOGE("Read invalid data len [%zu] from driver", uint_currLen);
            break;
        }
        size_t last_pos = TransDataToActivityBuffer(buf, cur_pos + uint_currLen, deviceId_, channelId_);
        if (last_pos < cur_pos + uint_currLen) {
            if (memcpy_s(buf, MAX_BUFFER_SIZE, buf + last_pos, cur_pos + uint_currLen - last_pos) != EOK) {
                break;
            }
        }
        cur_pos = cur_pos + uint_currLen - last_pos;
    }
    return MSPTI_SUCCESS;
}

size_t ChannelReader::TransDataToActivityBuffer(char buffer[], size_t valid_size,
                                                uint32_t deviceId, AI_DRV_CHANNEL channelId)
{
    switch (channelId) {
        case PROF_CHANNEL_TS_FW:
            return TransTsFwData(buffer, valid_size, deviceId);
        case PROF_CHANNEL_STARS_SOC_LOG:
            return TransStarsLog(buffer, valid_size, deviceId);
        default:
            return 0;
    }
}

size_t ChannelReader::TransTsFwData(char buffer[], size_t valid_size, uint32_t deviceId)
{
    size_t pos = 0;
    constexpr uint32_t TS_TRACK_SIZE = 40;
    while (valid_size - pos >= TS_TRACK_SIZE) {
        TsTrackHead* tsHead = reinterpret_cast<TsTrackHead*>(buffer + pos);
        switch (tsHead->rptType) {
            case RPT_TYPE_STEP_TRACE:
                Mspti::Parser::ParserManager::GetInstance()->ReportStepTrace(deviceId,
                    reinterpret_cast<StepTrace*>(buffer + pos));
                break;
            default:
                break;
        }
        pos += TS_TRACK_SIZE;
    }
    return pos;
}

size_t ChannelReader::TransStarsLog(char buffer[], size_t valid_size, uint32_t deviceId)
{
    size_t pos = 0;
    while (valid_size - pos >= sizeof(StarsSocLog)) {
        Mspti::Parser::ParserManager::GetInstance()->ReportStarsSocLog(deviceId,
            reinterpret_cast<StarsSocLog *>(buffer + pos));
        Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId,
            reinterpret_cast<StarsSocHeader *>(buffer + pos));
        pos += sizeof(StarsSocLog);
    }
    return pos;
}
} // Channel
} // Ascend
} // Mspti
