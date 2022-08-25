/**
* @file platform_adapter.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H

#include <map>
#include <vector>
#include "singleton/singleton.h"
#include "utils/utils.h"
#include "message/prof_params.h"
#include "config/config_manager.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
enum CollectorTypesForPlatform {
    // Task
    PLATFORM_TASK_ASCENDCL,
    PLATFORM_TASK_GRAPH_ENGINE,
    PLATFORM_TASK_RUNTIME,
    PLATFORM_TASK_AICPU,
    PLATFORM_TASK_HCCL,
    PLATFORM_TASK_L2_CACHE,
    PLATFORM_TASK_TS_TIMELINE,
    PLATFORM_TASK_TS_STEP_TRACE,
    PLATFORM_TASK_TS_TRAINING_TRACE,
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
    PLATFORM_SYS_DEVICE_BIU,
    PLATFORM_SYS_DEVICE_POWER,
    // System-host
    PLATFORM_SYS_HOST_ONE_PID_CPU,
    PLATFORM_SYS_HOST_ONE_PID_MEM,
    PLATFORM_SYS_HOST_ONE_PID_DISK,
    PLATFORM_SYS_HOST_ONE_PID_OSRT,
    PLATFORM_SYS_HOST_NETWORK,
    PLATFORM_SYS_HOST_SYS_CPU_MEM_USAGE,
    PLATFORM_SYS_HOST_ALL_PID_CPU_MEM_USAGE,
    // MAX
    PLATFORM_COLLECTOR_TYPES_MAX
};

struct CommonParams {
    std::string appPath;
    std::string appEnv;
    std::string msproftx;
    std::string pythonPath;
    std::string parseSwitch;
    std::string querySwitch;
    std::string exportSwitch;
    std::string exportSummaryFormat;
    int exportIterationId;
    int exportModelId;
    int hostSysPid;
};

class PlatformAdapter : public analysis::dvvp::common::singleton::Singleton<PlatformAdapter> {
public:
    PlatformAdapter();
    virtual ~PlatformAdapter();

    virtual int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        Analysis::Dvvp::Common::Config::PlatformType platformType);
    virtual int Uninit();
    virtual void SetParamsForGlobal(struct CommonParams &comParams);
    virtual void SetParamsForTaskTime();
    virtual void SetParamsForTaskTrace();
    virtual void SetParamsForTrainingTrace();
    virtual void SetParamsForAscendCL();
    virtual void SetParamsForGE();
    virtual void SetParamsForRuntime();
    virtual void SetParamsForAICPU();
    virtual void SetParamsForHCCL();
    virtual void SetParamsForL2Cache();
    virtual void SetParamsForAicMetrics();
    virtual void SetParamsForAivMetrics();
    virtual void SetParamsForDeviceSysCpuMemUsage(int samplingInterval);
    virtual void SetParamsForDeviceAllPidCpuMemUsage(int samplingInterval);
    virtual void SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(int samplingInterval);
    virtual void SetParamsForDeviceHardwareMem(int samplingInterval, std::string llcMode);
    virtual void SetParamsForDeviceIO(int samplingInterval);
    virtual void SetParamsForDeviceIntercommection(int samplingInterval);
    virtual void SetParamsForDeviceDVPP(int samplingInterval);
    virtual void SetParamsForDeviceBIU(int biuFreq);
    virtual void SetParamsForDevicePower();
    virtual void SetParamsForHostPidCpu();
    virtual void SetParamsForHostPidMem();
    virtual void SetParamsForHostPidDisk();
    virtual void SetParamsForHostPidOSRT();
    virtual void SetParamsForHostNetwork();
    virtual void SetParamsForHostSysAllPidCpuMemUsage();

protected:
    std::vector<CollectorTypesForPlatform> supportSwitch_;
    std::map<std::string, std::string> getLlcEvents_;
    std::string aicRunningFreq_;   // to calculate aic total time
    std::string sysCountFreq_;    // to calculate op start/end time
    std::string l2CacheEvents_;
    Analysis::Dvvp::Common::Config::PlatformType platformType_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
};
}
}
}
}
#endif

