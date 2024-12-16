/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_utils.cpp
 * Description        : Runtime Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/12/4
 * *****************************************************************************
*/

#include "common/runtime_utils.h"
#include "common/inject/runtime_inject.h"
#include "external/mspti_activity.h"
#include "external/mspti_result.h"

namespace Mspti {
namespace Common {

uint32_t GetDeviceId()
{
    int32_t deviceId = 0;
    if (rtGetDevice(&deviceId) != MSPTI_SUCCESS) {
        return MSPTI_INVALID_DEVICE_ID;
    }
    return static_cast<uint32_t>(deviceId);
}

uint32_t GetStreamId(RtStreamT stm)
{
    int32_t streamId = 0;
    if (rtGetStreamId(stm, &streamId) != MSPTI_SUCCESS) {
        return MSPTI_INVALID_STREAM_ID;
    }
    return static_cast<uint32_t>(streamId);
}

} // Common
} // Mspti
