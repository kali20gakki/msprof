/**
* @file platform_adapter_lhisi.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_lhisi.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapterLhisi {
using namespace analysis::dvvp::common::error;

PlatformAdapterLhisi::PlatformAdapterLhisi() : platformType_(PlatformType::END_TYPE)
{
}

PlatformAdapterLhisi::~PlatformAdapterLhisi()
{
}

int PlatformAdapterLhisi::Init()
{
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_STEP_TRACE,
        PLATFORM_TASK_TS_TRAINING_TRACE, PLATFORM_TASK_TS_MEMCPY, PLATFORM_TASK_AIC_HWTS,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE};
    aicRunningFreq_ = "300";
    sysCountFreq_ = "24";
    return PROFILING_SUCCESS;
}

int PlatformAdapterLhisi::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}