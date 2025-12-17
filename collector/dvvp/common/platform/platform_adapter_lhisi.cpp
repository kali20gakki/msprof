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

#include "platform_adapter_lhisi.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;

PlatformAdapterLhisi::PlatformAdapterLhisi()
{
}

PlatformAdapterLhisi::~PlatformAdapterLhisi()
{
}

int PlatformAdapterLhisi::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    params_ = params;
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_KEYPOINT,
        PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY, PLATFORM_TASK_AIC_HWTS,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_MEMORY, PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE, PLATFORM_TASK_OP_ATTR};
    aicRunningFreq_ = "300";
    sysCountFreq_ = "24";
    platformType_ = platformType;
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