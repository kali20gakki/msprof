/**
* @file platform_adapter_cloud.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_cloud.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;

PlatformAdapterCloud::PlatformAdapterCloud()
{
}

PlatformAdapterCloud::~PlatformAdapterCloud()
{
}

int PlatformAdapterCloud::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    params_ = params;
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_L2_CACHE, PLATFORM_TASK_TS_KEYPOINT,
        PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY, PLATFORM_TASK_AIC_HWTS,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_MEMORY, PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU,
        PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC, PLATFORM_SYS_DEVICE_DDR,
        PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_ROCE, PLATFORM_SYS_DEVICE_HCCS,
        PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_HOST_ONE_PID_CPU,
        PLATFORM_SYS_HOST_ONE_PID_MEM, PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT,
        PLATFORM_SYS_HOST_NETWORK, PLATFORM_SYS_HOST_ALL_PID_CPU, PLATFORM_SYS_HOST_ALL_PID_MEM,
        PLATFORM_SYS_HOST_SYS_CPU, PLATFORM_SYS_HOST_SYS_MEM};
    aicRunningFreq_ = "800";
    sysCountFreq_ = "100";
    l2CacheEvents_ = "0x5b,0x59,0x5c,0x7d,0x7e,0x71,0x79,0x7c";
    platformType_ = platformType;
    return PROFILING_SUCCESS;
}

int PlatformAdapterCloud::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}