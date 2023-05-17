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
        PLATFORM_SYS_DEVICE_HCCS, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP,
        PLATFORM_SYS_DEVICE_INSTR_PROFILING, PLATFORM_SYS_DEVICE_POWER, PLATFORM_SYS_HOST_ONE_PID_CPU,
        PLATFORM_SYS_HOST_ONE_PID_MEM, PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT,
        PLATFORM_SYS_HOST_NETWORK, PLATFORM_SYS_HOST_ALL_PID_CPU, PLATFORM_SYS_HOST_ALL_PID_MEM,
        PLATFORM_SYS_HOST_SYS_CPU, PLATFORM_SYS_HOST_SYS_MEM};
    aicRunningFreq_ = "800";
    sysCountFreq_ = "50";
    l2CacheEvents_ = "0xF6,0xFB,0xFC,0xBF,0x90,0x91,0x9C,0x9D";
    platformType_ = platformType;
    aicMetricsEventsList_ = {
        {"ArithmeticUtilization", "0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f"},
        {"PipeUtilization", "0x416,0x417,0x9,0x302,0xc,0x303,0x54,0x55"},
        {"Memory", "0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13"},
        {"MemoryL0", "0x1b,0x1c,0x21,0x22,0x27,0x28,0x29,0x2a"},
        {"ResourceConflictRatio", "0x64,0x65,0x66"},
        {"MemoryUB", "0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44"},
        {"L2Cache", "0x500,0x502,0x504,0x506,0x508,0x50a"}
    };
    aivMetricsEventsList_ = {
        {"ArithmeticUtilization", "0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f"},
        {"PipeUtilization", "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55"},
        {"Memory", "0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13"},
        {"MemoryL0", "0x1b,0x1c,0x21,0x22,0x27,0x28,0x29,0x2a"},
        {"ResourceConflictRatio", "0x64,0x65,0x66"},
        {"MemoryUB", "0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44"},
        {"L2Cache", "0x500,0x502,0x504,0x506,0x508,0x50a"}
    };
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