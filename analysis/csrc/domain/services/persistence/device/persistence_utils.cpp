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