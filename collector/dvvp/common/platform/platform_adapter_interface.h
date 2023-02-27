/**
* @file platform_adapter_interface.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_INTERFACE_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_INTERFACE_H

#include <map>
#include <vector>
#include "utils/utils.h"
#include "message/prof_params.h"
#include "config/config_manager.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using Analysis::Dvvp::Common::Config::PlatformType;
enum CollectorTypesForPlatform {
    // Task
    PLATFORM_TASK_ASCENDCL,
    PLATFORM_TASK_GRAPH_ENGINE,
    PLATFORM_TASK_RUNTIME,
    PLATFORM_TASK_AICPU,
    PLATFORM_TASK_HCCL,
    PLATFORM_TASK_L2_CACHE,
    PLATFORM_TASK_TS_TIMELINE,
    PLATFORM_TASK_TS_KEYPOINT,
    PLATFORM_TASK_TS_KEYPOINT_TRAINING,
    PLATFORM_TASK_TS_MEMCPY,
    PLATFORM_TASK_AIC_HWTS,
    PLATFORM_TASK_AIV_HWTS,
    PLATFORM_TASK_AIC_METRICS,
    PLATFORM_TASK_AIV_METRICS,
    PLATFORM_TASK_STARS_ACSQ,
    // System-device
    PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE,
    PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE,
    PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU,
    PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU,
    PLATFORM_SYS_DEVICE_LLC,
    PLATFORM_SYS_DEVICE_DDR,
    PLATFORM_SYS_DEVICE_HBM,
    PLATFORM_SYS_DEVICE_NIC,
    PLATFORM_SYS_DEVICE_ROCE,
    PLATFORM_SYS_DEVICE_HCCS,
    PLATFORM_SYS_DEVICE_PCIE,
    PLATFORM_SYS_DEVICE_DVPP,
    PLATFORM_SYS_DEVICE_INSTR_PROFILING,
    PLATFORM_SYS_DEVICE_POWER,
    // System-host
    PLATFORM_SYS_HOST_ONE_PID_CPU,
    PLATFORM_SYS_HOST_ALL_PID_CPU,
    PLATFORM_SYS_HOST_ONE_PID_MEM,
    PLATFORM_SYS_HOST_ALL_PID_MEM,
    PLATFORM_SYS_HOST_ONE_PID_DISK,
    PLATFORM_SYS_HOST_ONE_PID_OSRT,
    PLATFORM_SYS_HOST_NETWORK,
    PLATFORM_SYS_HOST_SYS_CPU,
    PLATFORM_SYS_HOST_SYS_MEM,
    // MAX
    PLATFORM_COLLECTOR_TYPES_MAX
};

enum class CoreType {
    AI_CORE = 0,
    AI_VECTOR_CORE
};

struct CommonParams {
    std::string output;
    std::string storageLimit;
    std::string msproftx;
    std::string device;
    int profilingPeriod;
    int hostSysPid;
};

const std::map<std::string, std::string> AIC_AIV_METRICS_LIST = {
    {"ArithmeticUtilization", "0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f"},
    {"PipeUtilization", "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55"},
    {"Memory", "0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13"},
    {"MemoryL0", "0x1b,0x1c,0x21,0x22,0x27,0x28,0x29,0x2a"},
    {"ResourceConflictRatio", "0x64,0x65,0x66"},
    {"MemoryUB", "0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44"}
};

class PlatformAdapterInterface {
public:
    PlatformAdapterInterface();
    virtual ~PlatformAdapterInterface();

    virtual int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, PlatformType platformType);
    virtual int Uninit();
    virtual void SetParamsForGlobal(struct CommonParams &comParams);
    virtual void SetParamsForStorageLimit(struct CommonParams &comParams);
    virtual void SetParamsForTaskTime();
    virtual void SetParamsForTaskTrace();
    virtual void SetParamsForTrainingTrace();
    virtual void SetParamsForAscendCL();
    virtual void SetParamsForGE();
    virtual void SetParamsForRuntime();
    virtual void SetParamsForAICPU();
    virtual void SetParamsForHCCL();
    virtual void SetParamsForL2Cache();
    virtual void SetParamsForAicMetrics(const std::string &mode, const std::string &metrics, int samplingInterval);
    virtual void SetParamsForAivMetrics(const std::string &mode, const std::string &metrics, int samplingInterval);
    virtual void SetParamsForDeviceSysCpuMemUsage(int samplingInterval);
    virtual void SetParamsForDeviceAllPidCpuMemUsage(int samplingInterval);
    virtual void SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(int samplingInterval);
    virtual void SetParamsForDeviceHardwareMem(int samplingInterval, std::string llcMode);
    virtual void SetParamsForDeviceIO(int samplingInterval);
    virtual void SetParamsForDeviceIntercommection(int samplingInterval);
    virtual void SetParamsForDeviceDVPP(int samplingInterval);
    virtual void SetParamsForDeviceInstr(int instrFreq);
    virtual void SetParamsForDevicePower();
    virtual void SetParamsForHostPidDisk();
    virtual void SetParamsForHostPidOSRT();
    virtual void SetParamsForHostNetwork();
    virtual void SetParamsForHostOnePidCpu(int samplingInterval);
    virtual void SetParamsForHostOnePidMem(int samplingInterval);
    virtual void SetParamsForHostAllPidCpu(int samplingInterval);
    virtual void SetParamsForHostAllPidMem(int samplingInterval);
    virtual void SetParamsForHostSysMem(int samplingInterval);
    virtual void SetParamsForHostSysCpu(int samplingInterval);

protected:
    std::vector<CollectorTypesForPlatform> supportSwitch_;
    std::map<std::string, std::string> getLlcEvents_;
    std::map<std::string, std::string> aicMetricsEventsList_;
    std::map<std::string, std::string> aivMetricsEventsList_;
    std::string aicRunningFreq_;   // to calculate aic total time
    std::string sysCountFreq_;    // to calculate op start/end time
    std::string l2CacheEvents_;
    PlatformType platformType_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;

private:
    int GetMetricsEvents(const std::string &metricsType, std::string &events, const CoreType type) const;
};
}
}
}
}
#endif

