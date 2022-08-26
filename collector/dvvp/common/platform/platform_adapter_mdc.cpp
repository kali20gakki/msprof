/**
* @file platform_adapter_mdc.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_mdc.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;

PlatformAdapterMdc::PlatformAdapterMdc()
{
}

PlatformAdapterMdc::~PlatformAdapterMdc()
{
}

int PlatformAdapterMdc::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    Analysis::Dvvp::Common::Config::PlatformType platformType)
{
    platformType_ = platformType;
    params_ = params;
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_L2_CACHE, PLATFORM_TASK_TS_STEP_TRACE,
        PLATFORM_TASK_TS_TRAINING_TRACE, PLATFORM_TASK_TS_MEMCPY, PLATFORM_TASK_AIC_HWTS, PLATFORM_TASK_AIV_HWTS,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_AIV_METRICS, PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU,
        PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC, PLATFORM_SYS_DEVICE_DDR,
        PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_DVPP};
    aicRunningFreq_ = "960";
    sysCountFreq_ = "38.4";
    l2CacheEvents_ = "0x78,0x79,0x77,0x71,0x6a,0x6c,0x74,0x62";
    return PROFILING_SUCCESS;
}

int PlatformAdapterMdc::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}