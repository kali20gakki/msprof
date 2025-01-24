/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : persistence_utils.cpp
 * Description        : 保存数据通用操作
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"

namespace Analysis {
namespace Domain {
namespace {
const uint64_t MILLI_SECOND = 1000;
}

SyscntConversionParams GenerateSyscntConversionParams(const DeviceContext& context)
{
    CpuInfo cpuInfo;
    context.Getter(cpuInfo);
    HostStartLog hostStartLog;
    context.Getter(hostStartLog);
    uint64_t hostMonotonic = hostStartLog.clockMonotonicRaw;
    DeviceInfo deviceInfo;
    context.Getter(deviceInfo);
    DeviceStartLog deviceStartLog;
    context.Getter(deviceStartLog);
    if (!IsDoubleEqual(cpuInfo.frequency, 0.0) && hostStartLog.cntVctDiff) {
        uint64_t diffTime = static_cast<uint64_t>(hostStartLog.cntVctDiff * MILLI_SECOND / cpuInfo.frequency);
        if (UINT64_MAX - hostStartLog.clockMonotonicRaw >= diffTime) {
            hostMonotonic = hostStartLog.clockMonotonicRaw + diffTime;
        }
    }
    SyscntConversionParams params{deviceInfo.hwtsFrequency, deviceStartLog.cntVct, hostMonotonic};
    return params;
}
}
}