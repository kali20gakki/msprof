/**
* @file platform_adapter_interface.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter_interface.h"
#include "prof_acl_api.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "utils/utils.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;

PlatformAdapterInterface::PlatformAdapterInterface()
    : params_(nullptr)
{
}

PlatformAdapterInterface::~PlatformAdapterInterface()
{
}

int PlatformAdapterInterface::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    params_ = params;
    getLlcEvents_ = {{"read", "read"}, {"write", "write"}};
    return PROFILING_SUCCESS;
}

void PlatformAdapterInterface::SetParamsForGlobal(struct CommonParams &comParams)
{
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    params_->result_dir = comParams.output.empty() ? params_->result_dir : comParams.output;
    params_->storageLimit = comParams.storageLimit.empty() ? params_->storageLimit : comParams.storageLimit;
    params_->msproftx = comParams.msproftx.empty() ? params_->msproftx : comParams.msproftx;
    params_->host_sys_pid = comParams.hostSysPid;
    params_->devices = comParams.device;
    params_->profiling_period = comParams.profilingPeriod;
}

void PlatformAdapterInterface::SetParamsForTaskTime()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_KEYPOINT) != supportSwitch_.end()) {
        params_->ts_keypoint = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_KEYPOINT_TRACE | PROF_KEYPOINT_TRACE_HELPER;
    }
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
        params_->dataTypeConfig |= PROF_MODEL_LOAD;
    }
}

void PlatformAdapterInterface::SetParamsForTaskTrace()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
    }
    SetParamsForTaskTime();
}

void PlatformAdapterInterface::SetParamsForTrainingTrace()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_KEYPOINT_TRAINING) !=
        supportSwitch_.end()) {
        params_->ts_keypoint = MSPROF_SWITCH_ON;
        params_->ts_fw_training = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_KEYPOINT_TRACE | PROF_KEYPOINT_TRACE_HELPER;
    }
}

void PlatformAdapterInterface::SetParamsForAscendCL()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_ASCENDCL) != supportSwitch_.end()) {
        params_->acl = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_ACL_API;
    }
}

void PlatformAdapterInterface::SetParamsForGE()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_GRAPH_ENGINE) != supportSwitch_.end()) {
        params_->modelExecution = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_MODEL_EXECUTE;
    }
}

void PlatformAdapterInterface::SetParamsForRuntime()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_RUNTIME) != supportSwitch_.end()) {
        params_->runtimeApi = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_RUNTIME_API;
    }
}

void PlatformAdapterInterface::SetParamsForAICPU()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AICPU) != supportSwitch_.end()) {
        params_->aicpuTrace = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_AICPU_TRACE;
    }
}

void PlatformAdapterInterface::SetParamsForHCCL()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->hcclTrace = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
    }
}

void PlatformAdapterInterface::SetParamsForL2Cache()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_L2_CACHE) != supportSwitch_.end()) {
        params_->l2CacheTaskProfiling = MSPROF_SWITCH_ON;
        params_->l2CacheTaskProfilingEvents = l2CacheEvents_;
        params_->dataTypeConfig |= PROF_L2CACHE;
    }
}

int PlatformAdapterInterface::GetMetricsEvents(const std::string &metricsType, std::string &events) const
{
    if (metricsType.empty()) {
        MSPROF_LOGE("metricsType is  empty");
        return PROFILING_FAILED;
    }
    auto iter = AIC_AIV_METRICS_LIST.find(metricsType);
    if (iter != AIC_AIV_METRICS_LIST.end()) {
        events = iter->second;
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGE("Invalid metrics type %s", metricsType.c_str());
    std::string errReason = "metrics type should be in [";
    for (auto type : AIC_AIV_METRICS_LIST) {
        errReason += type.first;
        errReason += "|";
    }
    if (errReason[errReason.size() - 1] == '|') {
        errReason.replace(errReason.size() - 1, 1, "]");
    }
    MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
        std::vector<std::string>({"metrics", metricsType, errReason}));
    return PROFILING_FAILED;
}

void PlatformAdapterInterface::SetParamsForAicMetrics(const std::string &mode, const std::string &metrics,
    int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIC_METRICS) != supportSwitch_.end()) {
        params_->ai_core_profiling = MSPROF_SWITCH_ON;
        int ret = GetMetricsEvents(metrics, params_->ai_core_profiling_events);
        if (ret != PROFILING_SUCCESS) {
            return;
        }
        params_->ai_core_metrics = metrics;
        params_->ai_core_profiling_mode = mode;
        if (mode == analysis::dvvp::message::PROFILING_MODE_SAMPLE_BASED) {
            params_->aicore_sampling_interval = samplingInterval;
        } else {
            params_->dataTypeConfig |= PROF_AICORE_METRICS;
        }
    }
}

void PlatformAdapterInterface::SetParamsForAivMetrics(const std::string &mode, const std::string &metrics,
    int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIV_METRICS) != supportSwitch_.end()) {
        params_->aiv_profiling = MSPROF_SWITCH_ON;
        int ret = GetMetricsEvents(metrics, params_->aiv_profiling_events);
        if (ret != PROFILING_SUCCESS) {
            return;
        }
        params_->aiv_metrics = metrics;
        params_->aiv_profiling_mode = mode;
        if (mode == analysis::dvvp::message::PROFILING_MODE_SAMPLE_BASED) {
            params_->aiv_sampling_interval = samplingInterval;
        } else {
            params_->dataTypeConfig |= PROF_AIV_METRICS;
        }
    }
}

void PlatformAdapterInterface::SetParamsForDeviceSysCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE) !=
        supportSwitch_.end()) {
        params_->sys_profiling = MSPROF_SWITCH_ON;
        params_->sys_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceAllPidCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE) !=
        supportSwitch_.end()) {
        params_->pid_profiling = MSPROF_SWITCH_ON;
        params_->pid_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU) !=
        supportSwitch_.end()) {
        params_->tsCpuProfiling = MSPROF_SWITCH_ON;
        params_->ts_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU) !=
        supportSwitch_.end()) {
        params_->aiCtrlCpuProfiling = MSPROF_SWITCH_ON;
        params_->ai_ctrl_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (setFlag) {
        params_->cpu_profiling = MSPROF_SWITCH_ON;
        params_->cpu_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceHardwareMem(int samplingInterval, std::string llcMode)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_LLC) != supportSwitch_.end()) {
        params_->msprof_llc_profiling = MSPROF_SWITCH_ON;
        params_->llc_interval = samplingInterval;
        params_->llc_profiling_events = getLlcEvents_[llcMode];
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_DDR) != supportSwitch_.end()) {
        params_->ddr_profiling = MSPROF_SWITCH_ON;
        params_->ddr_interval = samplingInterval;
        params_->ddr_profiling_events = "read,write";
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_HBM) != supportSwitch_.end()) {
        params_->hbmProfiling = MSPROF_SWITCH_ON;
        params_->hbmInterval = samplingInterval;
        params_->hbm_profiling_events = "read,write";
        setFlag = true;
    }
    if (setFlag) {
        params_->hardware_mem = MSPROF_SWITCH_ON;
        params_->hardware_mem_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceIO(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_NIC) != supportSwitch_.end()) {
        params_->nicProfiling = MSPROF_SWITCH_ON;
        params_->nicInterval = samplingInterval;
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_ROCE) != supportSwitch_.end()) {
        params_->roceProfiling = MSPROF_SWITCH_ON;
        params_->roceInterval = samplingInterval;
        setFlag = true;
    }
    if (setFlag) {
        params_->io_profiling = MSPROF_SWITCH_ON;
        params_->io_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceIntercommection(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_HCCS) != supportSwitch_.end()) {
        params_->hccsProfiling = MSPROF_SWITCH_ON;
        params_->hccsInterval = samplingInterval;
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_PCIE) != supportSwitch_.end()) {
        params_->pcieProfiling = MSPROF_SWITCH_ON;
        params_->pcieInterval = samplingInterval;
        setFlag = true;
    }
    if (setFlag) {
        params_->interconnection_profiling = MSPROF_SWITCH_ON;
        params_->interconnection_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceDVPP(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_DVPP) != supportSwitch_.end()) {
        params_->dvpp_profiling = MSPROF_SWITCH_ON;
        params_->dvpp_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForDeviceBIU(int biuFreq)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_BIU) != supportSwitch_.end()) {
        params_->biu = MSPROF_SWITCH_ON;
        params_->biu_freq = biuFreq;
    }
}

void PlatformAdapterInterface::SetParamsForDevicePower()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_POWER) != supportSwitch_.end()) {
        params_->low_power = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostPidCpu()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_CPU) !=
        supportSwitch_.end()) {
        params_->host_cpu_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostPidMem()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_MEM) !=
        supportSwitch_.end()) {
        params_->host_mem_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostPidDisk()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_DISK) !=
        supportSwitch_.end()) {
        params_->host_disk_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostPidOSRT()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_OSRT) !=
        supportSwitch_.end()) {
        params_->host_osrt_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostNetwork()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_NETWORK) != supportSwitch_.end()) {
        params_->host_network_profiling = MSPROF_SWITCH_ON;
        params_->host_sys = MSPROF_SWITCH_ON;
    }
}

void PlatformAdapterInterface::SetParamsForHostSysAllPidCpuUsage()
{
}

void PlatformAdapterInterface::SetParamsForHostSysAllPidMemUsage()
{
}

int PlatformAdapterInterface::Uninit()
{
    return PROFILING_SUCCESS;
}
}
}
}
}