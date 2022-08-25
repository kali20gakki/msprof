/**
* @file platform_adapter_dc.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_dc.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapterDc {
using namespace analysis::dvvp::common::error;

PlatformAdapterDc::PlatformAdapterDc() : platformType_(PlatformType::END_TYPE)
{
}

PlatformAdapterDc::~PlatformAdapterDc()
{
}

int PlatformAdapterDc::Init()
{
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_L2_CACHE, PLATFORM_TASK_TS_STEP_TRACE,
        PLATFORM_TASK_TS_TRAINING_TRACE, PLATFORM_TASK_TS_MEMCPY, PLATFORM_TASK_AIC_HWTS,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU,
        PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC, PLATFORM_SYS_DEVICE_DDR,
        PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_HOST_ONE_PID_CPU,
        PLATFORM_SYS_HOST_ONE_PID_MEM, PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT,
        PLATFORM_SYS_HOST_DISK, PLATFORM_SYS_HOST_SYS_CPU_MEM_USAGE, PLATFORM_SYS_HOST_ALL_PID_CPU_MEM_USAGE};
    aicRunningFreq_ = "1150";
    sysCountFreq_ = "38.4";
    l2CacheEvents_ = "0x78,0x79,0x77,0x71,0x6a,0x6c,0x74,0x62";
    return PROFILING_SUCCESS;
}

int PlatformAdapterDc::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}