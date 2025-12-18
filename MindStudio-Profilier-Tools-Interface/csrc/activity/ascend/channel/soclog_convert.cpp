/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soclog_convert.cpp
 * Description        : 转换驱动数据为通用SocLog
 * Author             : msprof team
 * Creation Date      : 2025/07/29
 * *****************************************************************************
 */

#include "soclog_convert.h"

#include <functional>

#include "channel_data.h"
#include "stars_common.h"

namespace Mspti {
namespace Convert {
namespace {
bool Trans910BSocLog(const void *const buffer, HalLogData &halLogData)
{
    const auto *originHeader = reinterpret_cast<const StarsSocHeader *>(buffer);
    if (originHeader->funcType == STARS_FUNC_TYPE_BEGIN || originHeader->funcType == STARS_FUNC_TYPE_END) {
        const auto *originData = reinterpret_cast<const StarsSocLog *>(buffer);
        uint16_t streamId = StarsCommon::GetStreamId(static_cast<uint16_t>(originData->streamId),
                                                     static_cast<uint16_t>(originData->taskId));
        uint32_t taskId = StarsCommon::GetTaskId(static_cast<uint16_t>(originData->streamId),
                                                 static_cast<uint16_t>(originData->taskId));
        halLogData.type = ACSQ_LOG;
        halLogData.acsq = {static_cast<uint16_t>(originData->funcType),
                           static_cast<uint16_t>(originData->taskType),
                           streamId, taskId, originData->timestamp};
    } else if (originHeader->funcType == FFTS_PLUS_TYPE_START || originHeader->funcType == FFTS_PLUS_TYPE_END) {
        const auto *originData = reinterpret_cast<const FftsPlusLog *>(buffer);
        uint16_t streamId = StarsCommon::GetStreamId(static_cast<uint16_t>(originData->streamId),
                                                     static_cast<uint16_t>(originData->taskId));
        uint16_t taskId = StarsCommon::GetTaskId(static_cast<uint16_t>(originData->streamId),
                                                 static_cast<uint16_t>(originData->taskId));
        halLogData.type = FFTS_LOG;
        halLogData.ffts = {static_cast<uint16_t>(originData->funcType),
                           static_cast<uint16_t>(originData->taskType),
                           streamId, taskId, originData->timestamp, originData->subTaskId};
    } else {
        MSPTI_LOGW("unkonw funcType, funcType is %u", static_cast<uint32_t>(originHeader->funcType));
        return false;
    }
    return true;
}

bool Trans310BSocLog(const void *const buffer, HalLogData &halLogData)
{
    return Trans910BSocLog(buffer, halLogData);
}

bool TransSocLogV6(const void *const buffer, HalLogData &halLogData)
{
    const auto *originData = reinterpret_cast<const StarsSocLogV6 *>(buffer);
    uint16_t streamId = 0;
    uint32_t taskId = originData->taskId;
    if (originData->funcType == STARS_FUNC_TYPE_BEGIN || originData->funcType == STARS_FUNC_TYPE_END) {
        halLogData.type = ACSQ_LOG;
        halLogData.acsq = {static_cast<uint16_t>(originData->funcType),
                           static_cast<uint16_t>(originData->taskType), streamId,
                           taskId, originData->timestamp};
    } else {
        MSPTI_LOGW("unkonw funcType, funcType is %u", static_cast<uint32_t>(originData->funcType));
        return false;
    }
    return true;
}
}

SocLogConvert::TransFunc SocLogConvert::GetTransFunc(uint32_t deviceId, Common::PlatformType chipType) const
{
    switch (chipType) {
        case Common::PlatformType::CHIP_910B:
            return Trans910BSocLog;
        case Common::PlatformType::CHIP_310B:
            return Trans310BSocLog;
        case Common::PlatformType::CHIP_V6:
            return TransSocLogV6;
        default:
            return nullptr;
    }
}

size_t SocLogConvert::GetStructSize(uint32_t deviceId, Common::PlatformType chipType) const
{
    switch (chipType) {
        case Common::PlatformType::CHIP_910B:
            return sizeof(StarsSocLog);
        case Common::PlatformType::CHIP_310B:
            return sizeof(StarsSocLog);
        case Common::PlatformType::CHIP_V6:
            return sizeof(StarsSocLogV6);
        default:
            return INVALID_STRUCT_SIZE;
    }
}

SocLogConvert &SocLogConvert::GetInstance()
{
    static SocLogConvert instance;
    return instance;
}
}
}

