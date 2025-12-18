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
#include "csrc/activity/ascend/channel/channel_reader.h"

#include <cstring>

#include "csrc/activity/activity_manager.h"
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/activity/ascend/parser/parser_manager.h"
#include "csrc/activity/ascend/parser/device_task_calculator.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/inject/driver_inject.h"
#include "csrc/common/inject/inject_base.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/activity/ascend/channel/soclog_convert.h"
#include "csrc/activity/ascend/parser/kernel_parser.h"
#include "csrc/include/mspti_activity.h"
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
        if (uint_currLen > (MAX_BUFFER_SIZE - cur_pos)) {
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
        HalLogData logData;
        Convert::SocLogConvert::GetInstance().TransData(buffer, valid_size, deviceId, pos, logData);
        if (Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId, logData) != MSPTI_SUCCESS) {
            MSPTI_LOGE("DeviceTaskCalculator parse SocLog failed");
    }
        if (Mspti::Parser::KernelParser::GetInstance().ReportStarsSocLog(deviceId, logData) != MSPTI_SUCCESS) {
            MSPTI_LOGE("KernelParser parse SocLog failed");
        }
    }
    return pos;
}
} // Channel
} // Ascend
} // Mspti
