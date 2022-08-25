/**
* @file platform_adapter.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter.h"
#include "toolchain/prof_acl_api.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;

PlatformAdapter::PlatformAdapter() : platformType_(PlatformType::END_TYPE)
{
}

PlatformAdapter::~PlatformAdapter()
{
}

int PlatformAdapter::Init()
{
    getLlcEvents_ = {{"read", GenerateReadEvents},
                     {"write", GenerateWriteEvents}};
    return PROFILING_SUCCESS;
}

std::string PlatformAdapter::GenerateReadEvents()
{
    return "read";
}

std::string PlatformAdapter::GenerateWriteEvents()
{
    return "write";
}

void PlatformAdapter::SetParamsForGlobal()
{
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
}

void PlatformAdapter::SetParamsForTaskTime()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_TIMELINE) != supportSwitch_.end()) {
        params_->ts_timeline = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_SCHEDULE_TIMELINE | PROF_TASK_TIME;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIC_HWTS) != supportSwitch_.end()) {
        params_->hwts_log = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIV_HWTS) != supportSwitch_.end()) {
        params_->hwts_log1 = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_STARS_ACSQ) != supportSwitch_.end()) {
        params_->stars_acsq_task = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_RUNTIME) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_RUNTIME_TRACE;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_MEMCPY) != supportSwitch_.end()) {
        params_->ts_memcpy = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_GRAPH_ENGINE) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_MODEL_EXECUTE;
    }
}

void PlatformAdapter::SetParamsForTaskTrace()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
    }
    SetParamsForTaskTime();
}

void PlatformAdapter::SetParamsForTrainingTrace()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_TRAINING_TRACE) !=
        supportSwitch_.end()) {
        params->ts_fw_training = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TRAINING_TRACE;
    }
}

void PlatformAdapter::SetParamsForAscendCL()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_ASCENDCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_ACL_API;
    }
}

void PlatformAdapter::SetParamsForGE()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_GRAPH_ENGINE) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_MODEL_EXECUTE;
    }
}

void PlatformAdapter::SetParamsForRuntime()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_RUNTIME) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_RUNTIME_API;
    }
}

void PlatformAdapter::SetParamsForAICPU()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AICPU) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_AICPU_TRACE;
    }
}

void PlatformAdapter::SetParamsForHCCL()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
    }
}

void PlatformAdapter::SetParamsForL2Cache()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_L2_CACHE) != supportSwitch_.end()) {
        params_->l2CacheTaskProfiling = MSPROF_SWITCH_ON;
        params_->l2CacheTaskProfilingEvents = l2CacheEvents_;
    }
}

void PlatformAdapter::SetParamsForAicMetrics()
{
}

void PlatformAdapter::SetParamsForAivMetrics()
{
}

void PlatformAdapter::SetParamsForDeviceSysCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE) != supportSwitch_.end()) {
        params_->sys_profiling = MSPROF_SWITCH_ON;
        params->sys_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceAllPidCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE) != supportSwitch_.end()) {
        params_->pid_profiling = MSPROF_SWITCH_ON;
        params->pid_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU) != supportSwitch_.end()) {
        params->tsCpuProfiling = MSPROF_SWITCH_ON;
        params->ts_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU) != supportSwitch_.end()) {
        params->aiCtrlCpuProfiling = MSPROF_SWITCH_ON;
        params->ai_ctrl_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (setFlag) {
        params->cpu_profiling = MSPROF_SWITCH_ON;
        dstParams->cpu_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceHardwareMem(int samplingInterval, std::string llcMode)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_LLC) != supportSwitch_.end()) {
        params->msprof_llc_profiling = MSPROF_SWITCH_ON;
        params->llc_interval = params->samplingInterval;
        params->llc_profiling_events = getLlcEvents_[llcMode]();
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_DDR) != supportSwitch_.end()) {
        params->ddr_profiling = MSPROF_SWITCH_ON;
        params->ddr_interval = samplingInterval;
        params->ddr_profiling_events = "read,write";
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_HBM) != supportSwitch_.end()) {
        params_->hbmProfiling = MSPROF_SWITCH_ON;
        params->hbmInterval = samplingInterval;
        params->hbm_profiling_events = "read,write";
        setFlag = true;
    }
    if (setFlag) {
        params->hardware_mem = MSPROF_SWITCH_ON;
        dstParams->hardware_mem_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceIO(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_NIC) != supportSwitch_.end()) {
        params->nicProfiling = MSPROF_SWITCH_ON;
        params->nicInterval = samplingInterval;
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_ROCE) != supportSwitch_.end()) {
        params->roceProfiling = MSPROF_SWITCH_ON;
        params->roceInterval = samplingInterval;
        setFlag = true;
    }
    if (setFlag) {
        params->io_profiling = MSPROF_SWITCH_ON;
        dstParams->io_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceIntercommection(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_HCCS) != supportSwitch_.end()) {
        params->hccsProfiling = MSPROF_SWITCH_ON;
        params->hccsInterval = samplingInterval;
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_PCIE) != supportSwitch_.end()) {
        params->pcieProfiling = MSPROF_SWITCH_ON;
        params->pcieInterval = samplingInterval;
        setFlag = true;
    }
    if (setFlag) {
        params->interconnection_profiling = MSPROF_SWITCH_ON;
        dstParams->interconnection_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceDVPP(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_DVPP) != supportSwitch_.end()) {
        params->dvpp_profiling = MSPROF_SWITCH_ON;
        params->dvpp_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceBIU(int biuFreq)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_BIU) != supportSwitch_.end()) {
        params_->biu = MSPROF_SWITCH_ON;
        params->biu_freq = biuFreq;
    }
}

void PlatformAdapter::SetParamsForDevicePower()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_POWER) != supportSwitch_.end()) {
        params_->low_power = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostPidCpu()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_CPU) != supportSwitch_.end()) {
        params_->host_cpu_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostPidMem()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_MEM) != supportSwitch_.end()) {
        params_->host_mem_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostPidDisk()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_DISK) != supportSwitch_.end()) {
        params_->host_disk_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostPidOSRT()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_OSRT) != supportSwitch_.end()) {
        params_->host_osrt_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostNetwork()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_NETWORK) != supportSwitch_.end()) {
        params_->host_network_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapter::SetParamsForHostSysAllPidCpuMemUsage()
{
}

int PlatformAdapter::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}