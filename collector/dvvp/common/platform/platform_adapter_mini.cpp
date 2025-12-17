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

#include "platform_adapter_mini.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;

PlatformAdapterMini::PlatformAdapterMini()
{
}

PlatformAdapterMini::~PlatformAdapterMini()
{
}

int PlatformAdapterMini::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    params_ = params;
    supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_TIMELINE, PLATFORM_TASK_TS_KEYPOINT, PLATFORM_TASK_TS_MEMCPY,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_MEMORY,
        PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC,
        PLATFORM_SYS_DEVICE_DDR, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_HOST_ONE_PID_CPU,
        PLATFORM_SYS_HOST_ONE_PID_MEM, PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT,
        PLATFORM_SYS_HOST_NETWORK, PLATFORM_SYS_HOST_ALL_PID_CPU, PLATFORM_SYS_HOST_ALL_PID_MEM,
        PLATFORM_SYS_HOST_SYS_CPU, PLATFORM_SYS_HOST_SYS_MEM, PLATFORM_TASK_OP_ATTR};
    std::string capacityEvents = GenerateCapacityEvents();
    std::string bandwidthEvents = GenerateBandwidthEvents();
    getLlcEvents_ = {{"capacity", capacityEvents},
                     {"bandwidth", bandwidthEvents}};
    aicRunningFreq_ = "680";
    sysCountFreq_ = "19.2";
    platformType_ = platformType;
    return PROFILING_SUCCESS;
}

std::string PlatformAdapterMini::GenerateCapacityEvents()
{
    std::vector<std::string> llcProfilingEvents;
    for (int i = 0; i < MAX_LLC_EVENTS; i++) {
        std::string tempEvents;
        tempEvents.append("hisi_l3c0_1/dsid");
        tempEvents.append(std::to_string(i));
        tempEvents.append("/");
        llcProfilingEvents.push_back(tempEvents);
    }
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}

std::string PlatformAdapterMini::GenerateBandwidthEvents()
{
    std::vector<std::string> llcProfilingEvents;
    llcProfilingEvents.push_back("hisi_l3c0_1/read_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_noallocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_noallocate/");
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}

int PlatformAdapterMini::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}