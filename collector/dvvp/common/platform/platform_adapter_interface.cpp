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
#include "validation/param_validation.h"
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
using namespace analysis::dvvp::common::validation;

PlatformAdapterInterface::PlatformAdapterInterface()
    : params_(nullptr), platformType_(PlatformType::MINI_TYPE)
{
    getLlcEvents_ = {{"read", "read"}, {"write", "write"}};
    aicMetricsEventsList_ = AIC_AIV_METRICS_LIST;
    aivMetricsEventsList_ = AIC_AIV_METRICS_LIST;
}

PlatformAdapterInterface::~PlatformAdapterInterface()
{
}

int PlatformAdapterInterface::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    params_ = params;
    platformType_ = platformType;
    return PROFILING_SUCCESS;
}

void PlatformAdapterInterface::SetParamsForGlobal(struct CommonParams &comParams)
{
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    params_->result_dir = comParams.output.empty() ? params_->result_dir : comParams.output;
    params_->msproftx = comParams.msproftx.empty() ? params_->msproftx : comParams.msproftx;
    params_->host_sys_pid = comParams.hostSysPid;
    params_->devices = comParams.device;
    params_->profiling_period = comParams.profilingPeriod;
}

/*!
 * @param comParams [IN] 外部输入指令
 * @brief 可选输入storage-limit未提供时, 默认开启落盘文件老化,默认值为保存路径中可用磁盘空间.
 */
void PlatformAdapterInterface::SetParamsForStorageLimit(struct CommonParams &comParams)
{
    constexpr uint32_t moveBit = 20;
    unsigned long long dirAvailSize = 0;
    if (comParams.storageLimit.empty()) {
        if (Utils::GetVolumeSize(params_->result_dir, dirAvailSize, VolumeSize::AVAIL_SIZE) == PROFILING_FAILED) {
            MSPROF_LOGW("GetVolumeSize failed");
        }
        dirAvailSize = static_cast<unsigned long long>(static_cast<double>(dirAvailSize) * 0.9); // 0.9的可用空间
        dirAvailSize >>= moveBit;
        if (dirAvailSize < 20 || dirAvailSize > UINT32_MAX) { // 判断范围为20~UINT32_MAX MB
            // 外部输入的storageLimit有效区间为200~4294967296(UINT32_MAX).
            // 若可用空间小于200M，仍然支持老化，不小于20M是为保证storageVolumeUpThd_不小于0
            dirAvailSize = 0;
            params_->storageLimit = "";
        } else {
            params_->storageLimit = std::to_string(dirAvailSize) + STORAGE_LIMIT_UNIT;
        }
    } else {
        params_->storageLimit = (comParams.storageLimit == "0MB") ? "" : comParams.storageLimit;
    }
    MSPROF_LOGI("comParams.storageLimit:%s, dirFreeSize:%lld, params_->storageLimit:%s", comParams.storageLimit.c_str(),
        dirAvailSize, params_->storageLimit.c_str());
}

void PlatformAdapterInterface::SetParamsForTaskTimeL0()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_KEYPOINT) != supportSwitch_.end()) {
        params_->ts_keypoint = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_KEYPOINT_TRACE | PROF_KEYPOINT_TRACE_HELPER;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_TIMELINE) != supportSwitch_.end()) {
        params_->ts_timeline = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_SCHEDULE_TIMELINE | PROF_TASK_TIME;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIC_HWTS) != supportSwitch_.end()) {
        params_->hwts_log = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIV_HWTS) != supportSwitch_.end()) {
        params_->hwts_log1 = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_STARS_ACSQ) != supportSwitch_.end()) {
        params_->stars_acsq_task = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_RUNTIME) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_RUNTIME_TRACE;
        ret = true;
    }
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_MEMCPY) != supportSwitch_.end()) {
        params_->ts_memcpy = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_TASK_TIME;
        ret = true;
    }
    SetParamsForGEL0();
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:task_time_l0 for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForTaskTimeL1()
{
    SetParamsForTaskTimeL0();
    params_->dataTypeConfig |= PROF_TASK_TIME_L1;
    SetParamsForGEL0();
    SetParamsForGEL1();
}

void PlatformAdapterInterface::SetParamsForTaskTrace()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
        ret = true;
    }
    SetParamsForTaskTimeL1();
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:task_trace for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForTrainingTrace()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_TS_KEYPOINT_TRAINING) !=
        supportSwitch_.end()) {
        params_->ts_keypoint = MSPROF_SWITCH_ON;
        params_->ts_fw_training = MSPROF_SWITCH_ON;
        params_->dataTypeConfig |= PROF_KEYPOINT_TRACE | PROF_KEYPOINT_TRACE_HELPER;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:training_trace for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForAscendCL()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_ASCENDCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_ACL_API;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:ascendcl for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}


void PlatformAdapterInterface::SetParamsForTaskMemory()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_MEMORY) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_TASK_MEMORY;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:task_memory for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}
void PlatformAdapterInterface::SetParamsForGEL0()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_GRAPH_ENGINE) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_MODEL_LOAD;
        params_->dataTypeConfig |= PROF_OP_DETAIL;
    }
}

void PlatformAdapterInterface::SetParamsForGEL1()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_GRAPH_ENGINE) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_FRAMEWORK_SCHEDULE_L1; // ge profiling data
        params_->dataTypeConfig |= PROF_FRAMEWORK_SCHEDULE_L0; // ge infershape data
    }
}

void PlatformAdapterInterface::SetParamsForRuntime()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_RUNTIME) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_RUNTIME_API;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:runtime_api for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForAICPU()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AICPU) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_AICPU_TRACE;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:aicpu for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForHCCL()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_HCCL) != supportSwitch_.end()) {
        params_->dataTypeConfig |= PROF_HCCL_TRACE;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:hccl for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForL2Cache()
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_L2_CACHE) != supportSwitch_.end()) {
        params_->l2CacheTaskProfiling = MSPROF_SWITCH_ON;
        params_->l2CacheTaskProfilingEvents = l2CacheEvents_;
        params_->dataTypeConfig |= PROF_L2CACHE;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:l2 for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

int PlatformAdapterInterface::GetMetricsEvents(const std::string &metricsType,
                                               std::string &events, const CoreType type) const
{
    if (metricsType.empty()) {
        MSPROF_LOGE("metricsType is  empty");
        return PROFILING_FAILED;
    }
    std::map<std::string, std::string> metricsEventsList;
    if (type == CoreType::AI_CORE) {
        metricsEventsList = aicMetricsEventsList_;
    } else {
        metricsEventsList = aivMetricsEventsList_;
    }
    if (metricsType.compare(0, CUSTOM_METRICS_VALID_HEADER.length(), CUSTOM_METRICS_VALID_HEADER) == 0) {
        events = metricsType.substr(CUSTOM_METRICS_VALID_HEADER.length());
        transform(events.begin(), events.end(), events.begin(), ::tolower);
        int ret = ParamValidation::instance()->CustomHexCharConfig(events, ",");
        if (ret == PROFILING_FAILED) {
            return PROFILING_FAILED;
        }
        MSPROF_EVENT("Custom metrics values is %s", events.c_str());
        return PROFILING_SUCCESS;
    }
    auto iter = metricsEventsList.find(metricsType);
    if (iter != metricsEventsList.end()) {
        events = iter->second;
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGE("Invalid metrics type %s", metricsType.c_str());
    std::string errReason = "metrics type should be in [";
    for (auto type : metricsEventsList) {
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
        int ret = GetMetricsEvents(metrics, params_->ai_core_profiling_events, CoreType::AI_CORE);
        if (ret != PROFILING_SUCCESS) {
            return;
        }
        if (platformType_ == PlatformType::CHIP_V4_1_0 && metrics == PIPE_UTILIZATION) {
            params_->ai_core_metrics = PIPE_UTILIZATION_EXCT; // change metrics for phase data
        } else {
            params_->ai_core_metrics = metrics;
        }
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
    bool val = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_TASK_AIV_METRICS) != supportSwitch_.end()) {
        params_->aiv_profiling = MSPROF_SWITCH_ON;
        int ret = GetMetricsEvents(metrics, params_->aiv_profiling_events, CoreType::AI_VECTOR_CORE);
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
        val = true;
    }
    if (!val) {
        MSPROF_LOGW("Unrecognized option:aiv_metrics for PlatformType:%d", static_cast<uint8_t>(platformType_));
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
        params_->llc_profiling = llcMode;
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

void PlatformAdapterInterface::SetParamsForDeviceInstr(int instrFreq)
{
    bool ret = false;
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_INSTR_PROFILING) !=
        supportSwitch_.end()) {
        params_->instr_profiling = MSPROF_SWITCH_ON;
        params_->instr_profiling_freq = instrFreq;
        params_->dataTypeConfig |= PROF_INSTR_PROFILING;
        ret = true;
    }
    if (!ret) {
        MSPROF_LOGW("Unrecognized option:instr profiling for PlatformType:%d", static_cast<uint8_t>(platformType_));
    }
}

void PlatformAdapterInterface::SetParamsForDevicePower()
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_DEVICE_POWER) != supportSwitch_.end()) {
        params_->low_power = MSPROF_SWITCH_ON;
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

void PlatformAdapterInterface::SetParamsForHostSysCpu(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_SYS_CPU) !=
        supportSwitch_.end()) {
        params_->host_sys_usage = MSPROF_SWITCH_ON;
        params_->host_sys_cpu_profiling = MSPROF_SWITCH_ON;
        params_->host_cpu_profiling_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForHostSysMem(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_SYS_MEM) !=
        supportSwitch_.end()) {
        params_->host_sys_usage = MSPROF_SWITCH_ON;
        params_->host_sys_mem_profiling = MSPROF_SWITCH_ON;
        params_->host_mem_profiling_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForHostOnePidCpu(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_CPU) !=
        supportSwitch_.end()) {
        params_->host_sys = MSPROF_SWITCH_ON;
        params_->host_one_pid_cpu_profiling = MSPROF_SWITCH_ON;
        params_->host_cpu_profiling_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForHostOnePidMem(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ONE_PID_MEM) !=
        supportSwitch_.end()) {
        params_->host_sys = MSPROF_SWITCH_ON;
        params_->host_one_pid_mem_profiling = MSPROF_SWITCH_ON;
        params_->host_mem_profiling_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForHostAllPidCpu(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ALL_PID_CPU) !=
        supportSwitch_.end()) {
        params_->host_sys_usage = MSPROF_SWITCH_ON;
        params_->host_all_pid_cpu_profiling = MSPROF_SWITCH_ON;
        params_->host_cpu_profiling_sampling_interval = samplingInterval;
    }
}

void PlatformAdapterInterface::SetParamsForHostAllPidMem(int samplingInterval)
{
    if (std::find(supportSwitch_.begin(), supportSwitch_.end(), PLATFORM_SYS_HOST_ALL_PID_MEM) !=
        supportSwitch_.end()) {
        params_->host_sys_usage = MSPROF_SWITCH_ON;
        params_->host_all_pid_mem_profiling = MSPROF_SWITCH_ON;
        params_->host_mem_profiling_sampling_interval = samplingInterval;
    }
}

int PlatformAdapterInterface::Uninit()
{
    return PROFILING_SUCCESS;
}

std::vector<std::string> PlatformAdapterInterface::GetMetricsList()
{
    std::vector<std::string> metricsList;
    for (auto iter : aicMetricsEventsList_) {
        metricsList.push_back(iter.first);
    }
    return metricsList;
}

}
}
}
}