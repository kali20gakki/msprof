/**
* @file platform_adapter_cloudv2.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_cloudv2.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;

PlatformAdapterCloudV2::PlatformAdapterCloudV2()
{
}

PlatformAdapterCloudV2::~PlatformAdapterCloudV2()
{
}

int PlatformAdapterCloudV2::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    params_ = params;
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_KEYPOINT, PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_AIV_METRICS, PLATFORM_TASK_STARS_ACSQ,
        PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE, PLATFORM_TASK_L2_CACHE,
        PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC,
        PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_ROCE,
        PLATFORM_SYS_DEVICE_HCCS, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_DEVICE_BIU,
        PLATFORM_SYS_DEVICE_POWER, PLATFORM_SYS_HOST_ONE_PID_CPU, PLATFORM_SYS_HOST_ONE_PID_MEM,
        PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT, PLATFORM_SYS_HOST_NETWORK,
        PLATFORM_SYS_HOST_ALL_PID_CPU, PLATFORM_SYS_HOST_ALL_PID_MEM,
        PLATFORM_SYS_HOST_SYS_CPU, PLATFORM_SYS_HOST_SYS_MEM};
    aicRunningFreq_ = "800";
    sysCountFreq_ = "50";
    l2CacheEvents_ = "0xF6,0xFB,0xFC,0xBF,0x90,0x91,0x9C,0x9D";
    platformType_ = platformType;
    return PROFILING_SUCCESS;
}

int PlatformAdapterCloudV2::Uninit()
{
    return PROFILING_SUCCESS;
}
}
}
}
}