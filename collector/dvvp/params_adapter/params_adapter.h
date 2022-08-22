/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_H

#include <array>
#include <string>
#include <vector>

#include "config/config_manager.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

using Analysis::Dvvp::Common::Config::PlatformType;
using analysis::dvvp::message::StatusInfo;
enum InputCfg {
    INPUT_CFG_MSPROF_APPLICATION = 0, // del
    INPUT_CFG_MSPROF_ENVIRONMENT = 1, // del
    INPUT_CFG_ACL_JSON_SWITCH = 2,  // del
    INPUT_CFG_GE_OPT_FP_POINT = 3,
    INPUT_CFG_GE_OPT_BP_POINT = 4,
    // other
    INPUT_CFG_COM_OUTPUT = 5,
    INPUT_CFG_COM_STORAGE_LIMIT = 6,
    INPUT_CFG_COM_MSPROFTX = 7,
    // Task
    INPUT_CFG_COM_TASK_TIME = 8,
    INPUT_CFG_COM_TASK_TRACE = 9,
    INPUT_CFG_COM_TRAINING_TRACE = 10,
    INPUT_CFG_COM_AI_CORE = 11,
    INPUT_CFG_COM_AIC_MODE = 12,
    INPUT_CFG_COM_AIC_METRICS = 13,
    INPUT_CFG_COM_AIC_FREQ = 14,
    INPUT_CFG_COM_AI_VECTOR = 15,
    INPUT_CFG_COM_AIV_MODE = 16,
    INPUT_CFG_COM_AIV_METRICS = 17,
    INPUT_CFG_COM_AIV_FREQ = 18,
    INPUT_CFG_COM_ASCENDCL = 19,
    INPUT_CFG_COM_MODEL_EXECUTION = 20,
    INPUT_CFG_COM_RUNTIME_API = 21,
    INPUT_CFG_COM_HCCL = 22,
    INPUT_CFG_COM_L2 = 23,
    INPUT_CFG_COM_AICPU = 24,
    // System-device
    INPUT_CFG_COM_SYS_DEVICES = 25,
    INPUT_CFG_COM_SYS_PERIOD = 26,
    INPUT_CFG_COM_SYS_USAGE = 27,
    INPUT_CFG_COM_SYS_USAGE_FREQ = 28,
    INPUT_CFG_COM_SYS_PID_USAGE = 29,
    INPUT_CFG_COM_SYS_PID_USAGE_FREQ = 30,
    INPUT_CFG_COM_SYS_CPU = 31,
    INPUT_CFG_COM_SYS_CPU_FREQ = 32,
    INPUT_CFG_COM_SYS_HARDWARE_MEM = 33,
    INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ = 34,
    INPUT_CFG_COM_LLC_MODE = 35,
    INPUT_CFG_COM_SYS_IO = 36,
    INPUT_CFG_COM_SYS_IO_FREQ = 37,
    INPUT_CFG_COM_SYS_INTERCONNECTION = 38,
    INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ = 39,
    INPUT_CFG_COM_DVPP = 40,
    INPUT_CFG_COM_DVPP_FREQ = 41,
    INPUT_CFG_COM_POWER = 42,
    INPUT_CFG_COM_BIU = 43,
    INPUT_CFG_COM_BIU_FREQ = 44,
    // System-host
    INPUT_CFG_HOST_SYS = 45,
    INPUT_CFG_HOST_SYS_PID = 46,
    INPUT_HOST_SYS_USAGE = 47,
    // analysis
    INPUT_CFG_PYTHON_PATH = 48,
    INPUT_CFG_SUMMARY_FORMAT = 49,
    INPUT_CFG_PARSE = 50,
    INPUT_CFG_QUERY = 51,
    INPUT_CFG_EXPORT = 52,
    INPUT_CFG_ITERATION_ID = 53,
    INPUT_CFG_MODEL_ID = 54,
    INPUT_CFG_MAX
};

enum EnableType {
    ENABLE_MSPROF,
    ENABLE_ACL_JSON,
    ENABLE_GE_OPTION,
    ENABLE_API
};

class ParamsAdapter{
public:
    ParamsAdapter()
    {
    }
    int CheckListInit();
    bool BlackSwitchCheck(InputCfg inputCfg) const;
    PlatformType GetPlatform() const;
    int ComCfgCheck(EnableType enableType, std::array<std::string, INPUT_CFG_MAX> paramContainer,
        std::vector<InputCfg> &cfgList) const;
    int TransToParam();

public:
    //======common=======
    int CheckOutputValid(const std::string &outputParam) const;
    int CheckStorageLimitValid(const std::string &storageLimitParam) const;
    int CheckAiMetricsValid(const std::string &aiMetrics) const;
    int CheckSwitchValid(const std::string &switchParam) const;
    int CheckFreqValid(const std::string &freq, const InputCfg freqOpt) const;
    int CheckLlcModeValid(const std::string &LlcMode) const;
    int CheckHostSysUsageValid(const std::string &HostSysUsage) const;
    //======msprof======
    int CheckAppParamValid(const std::string &appParam) const;
    int CheckAppScrValid(const std::string &appScript) const;
    int MsprofCheckAppValid(const std::string &appParam) const;
    int MsprofCheckEnvValid(const std::string &envParam) const;
    int MsprofCheckAiModeValid(const std::string &envParam) const;
    int MsprofCheckSysDeviceValid(const std::string &devListParam) const;
    int MsprofCheckSysPeriodValid(const std::string &sysPeriodParam) const;
    int MsprofCheckHostSysValid(const std::string &hostSysParam) const;
    int MsprofCheckHostSysPidValid(const std::string &hostSysPidParam) const;
    int MsprofCheckPythonPathValid(const std::string &pythonPathParam) const;
    int MsprofCheckSummaryFormatValid(const std::string &formatParam) const;
    int MsprofCheckExportIdValid(const std::string &idParam, const std::string &exportIdType) const;

private:
    std::vector<InputCfg> commonConfig_;
    std::vector<InputCfg> blackSwitch_;
    StatusInfo statusInfo_;
    PlatformType platformType_;
};
} // ParamsAdapter
} // Dvpp
} // Collector

#endif