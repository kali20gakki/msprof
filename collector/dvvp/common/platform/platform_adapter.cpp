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
#include "prof_acl_api.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::config;

PlatformAdapter::PlatformAdapter()
    : platformType_(Analysis::Dvvp::Common::Config::PlatformType::END_TYPE), params_(nullptr)
{
}

PlatformAdapter::~PlatformAdapter()
{
}

int PlatformAdapter::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    Analysis::Dvvp::Common::Config::PlatformType platformType)
{
    platformType_ = platformType;
    params_ = params;
    getLlcEvents_ = {{"read", "read"}, {"write", "write"}};
    return PROFILING_SUCCESS;
}

void PlatformAdapter::SetParamsForGlobal(struct CommonParams &comParams)
{
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    params_->app = comParams.appPath;
    params_->app_env = comParams.appEnv;
    params_->msproftx = comParams.msproftx;
    params_->host_sys_pid = comParams.hostSysPid;
    params_->pythonPath = comParams.pythonPath;
    params_->parseSwitch = comParams.parseSwitch;
    params_->querySwitch = comParams.querySwitch;
    params_->exportSwitch = comParams.exportSwitch;
    params_->exportSummaryFormat = comParams.exportSummaryFormat;
    params_->exportIterationId = comParams.exportIterationId;
    params_->exportModelId = comParams.exportModelId;
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
        params_->ts_fw_training = MSPROF_SWITCH_ON;
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
        params_->dataTypeConfig |= PROF_L2CACHE;
    }
}

int PlatformAdapter::GetMetricsEvents(const std::string &metricsType, std::string &events) const
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

void PlatformAdapter::SetParamsForAicMetrics(const std::string &mode, const std::string &metrics, int samplingInterval)
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

void PlatformAdapter::SetParamsForAivMetrics(const std::string &mode, const std::string &metrics, int samplingInterval)
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

void PlatformAdapter::SetParamsForDeviceSysCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE) != supportSwitch_.end()) {
        params_->sys_profiling = MSPROF_SWITCH_ON;
        params_->sys_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceAllPidCpuMemUsage(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE) != supportSwitch_.end()) {
        params_->pid_profiling = MSPROF_SWITCH_ON;
        params_->pid_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(int samplingInterval)
{
    bool setFlag = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU) != supportSwitch_.end()) {
        params_->tsCpuProfiling = MSPROF_SWITCH_ON;
        params_->ts_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU) != supportSwitch_.end()) {
        params_->aiCtrlCpuProfiling = MSPROF_SWITCH_ON;
        params_->ai_ctrl_cpu_profiling_events = "0x8,0x11";
        setFlag = true;
    }
    if (setFlag) {
        params_->cpu_profiling = MSPROF_SWITCH_ON;
        params_->cpu_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceHardwareMem(int samplingInterval, std::string llcMode)
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

void PlatformAdapter::SetParamsForDeviceIO(int samplingInterval)
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

void PlatformAdapter::SetParamsForDeviceIntercommection(int samplingInterval)
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

void PlatformAdapter::SetParamsForDeviceDVPP(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_DVPP) != supportSwitch_.end()) {
        params_->dvpp_profiling = MSPROF_SWITCH_ON;
        params_->dvpp_sampling_interval = samplingInterval;
    }
}

void PlatformAdapter::SetParamsForDeviceBIU(int biuFreq)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_BIU) != supportSwitch_.end()) {
        params_->biu = MSPROF_SWITCH_ON;
        params_->biu_freq = biuFreq;
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

void PlatformAdapter::SetParamsForHostSysAllPidCpuUsage()
{
}

void PlatformAdapter::SetParamsForHostSysAllPidMemUsage()
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