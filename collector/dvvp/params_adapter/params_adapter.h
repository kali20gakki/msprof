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

#include "message/prof_params.h"
#include "config/config_manager.h"
#include "mmpa_api.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::message::ProfileParams;
using Analysis::Dvvp::Common::Config::PlatformType;
using analysis::dvvp::message::StatusInfo;

const int AIC_FREQ_MIN = 1;
const int AIC_FREQ_MAX = 100;
const int AIV_FREQ_MIN = 1;
const int AIV_FREQ_MAX = 100;
const int SYS_USAGE_FREQ_MIN = 1;
const int SYS_USAGE_FREQ_MAX = 10;
const int SYS_CPU_FREQ_MIN = 1;
const int SYS_CPU_FREQ_MAX = 50;
const int SYS_PID_USAGE_FREQ_MIN = 1;
const int SYS_PID_USAGE_FREQ_MAX = 10;
const int SYS_HARDWARE_MEM_FREQ_MIN = 1;
const int SYS_HARDWARE_MEM_FREQ_MAX = 1000;
const int SYS_IO_FREQ_MIN = 1;
const int SYS_IO_FREQ_MAX = 100;
const int SYS_INTERCONNECTION_FREQ_MIN = 1;
const int SYS_INTERCONNECTION_FREQ_MAX = 50;
const int DVPP_FREQ_MIN = 1;
const int DVPP_FREQ_MAX = 100;
const int BIU_FREQ_MIN = 300;
const int BIU_FREQ_MAX = 30000;
const int ACC_PMU_MODE_THRED = 5000; // Check
const int FILE_FIND_REPLAY          = 100;
const std::string TOOL_NAME_PERF     = "perf";
const std::string TOOL_NAME_LTRACE   = "ltrace";
const std::string TOOL_NAME_IOTOP    = "iotop";
enum InputCfg {
    INPUT_CFG_MSPROF_APPLICATION = 0, // del
    INPUT_CFG_MSPROF_ENVIRONMENT = 1, // del
    // other
    INPUT_CFG_COM_OUTPUT = 2,
    INPUT_CFG_COM_STORAGE_LIMIT = 3,
    INPUT_CFG_COM_MSPROFTX = 4,
    // Task
    INPUT_CFG_COM_TASK_TIME = 5,
    INPUT_CFG_COM_TASK_TRACE = 6,
    INPUT_CFG_COM_TRAINING_TRACE = 7,
    INPUT_CFG_COM_AI_CORE = 8,
    INPUT_CFG_COM_AIC_MODE = 9,
    INPUT_CFG_COM_AIC_METRICS = 10,
    INPUT_CFG_COM_AIC_FREQ = 11,
    INPUT_CFG_COM_AI_VECTOR = 12,
    INPUT_CFG_COM_AIV_MODE = 13,
    INPUT_CFG_COM_AIV_METRICS = 14,
    INPUT_CFG_COM_AIV_FREQ = 15,
    INPUT_CFG_COM_ASCENDCL = 16,
    INPUT_CFG_COM_MODEL_EXECUTION = 17,
    INPUT_CFG_COM_RUNTIME_API = 18,
    INPUT_CFG_COM_HCCL = 19,
    INPUT_CFG_COM_L2 = 20,
    INPUT_CFG_COM_AICPU = 21,
    // System-device
    INPUT_CFG_COM_SYS_DEVICES = 22,
    INPUT_CFG_COM_SYS_PERIOD = 23,
    INPUT_CFG_COM_SYS_USAGE = 24,
    INPUT_CFG_COM_SYS_USAGE_FREQ = 25,
    INPUT_CFG_COM_SYS_PID_USAGE = 26,
    INPUT_CFG_COM_SYS_PID_USAGE_FREQ = 27,
    INPUT_CFG_COM_SYS_CPU = 28,
    INPUT_CFG_COM_SYS_CPU_FREQ = 29,
    INPUT_CFG_COM_SYS_HARDWARE_MEM = 30,
    INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ = 31,
    INPUT_CFG_COM_LLC_MODE = 32,
    INPUT_CFG_COM_SYS_IO = 33,
    INPUT_CFG_COM_SYS_IO_FREQ = 34,
    INPUT_CFG_COM_SYS_INTERCONNECTION = 35,
    INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ = 36,
    INPUT_CFG_COM_DVPP = 37,
    INPUT_CFG_COM_DVPP_FREQ = 38,
    INPUT_CFG_COM_POWER = 39,
    INPUT_CFG_COM_BIU = 40,
    INPUT_CFG_COM_BIU_FREQ = 41,
    // System-host
    INPUT_CFG_HOST_SYS = 42,
    INPUT_CFG_HOST_SYS_PID = 43,
    INPUT_HOST_SYS_USAGE = 44,
    // analysis
    INPUT_CFG_PYTHON_PATH = 45,
    INPUT_CFG_SUMMARY_FORMAT = 46,
    INPUT_CFG_PARSE = 47,
    INPUT_CFG_QUERY = 48,
    INPUT_CFG_EXPORT = 49,
    INPUT_CFG_ITERATION_ID = 50,
    INPUT_CFG_MODEL_ID = 51,
    INPUT_CFG_MAX
};

enum EnableType {
    ENABLE_MSPROF,
    ENABLE_ACL_JSON,
    ENABLE_GE_OPTION,
    ENABLE_API
};

class ParamsAdapter {
public:
    ParamsAdapter()
    {
    }
    int CheckListInit();
    bool BlackSwitchCheck(InputCfg inputCfg) const;
    PlatformType GetPlatform() const;
    int ComCfgCheck(EnableType enableType, std::array<std::string, INPUT_CFG_MAX> paramContainer,
        std::set<InputCfg> &setArgs,
        std::vector<std::pair<InputCfg, std::string>> &cfgList) const;
    int TransToParam(std::array<std::string, INPUT_CFG_MAX> paramContainer, SHARED_PTR_ALIA<ProfileParams> params);
    // To Del
    void Print(std::array<std::string, INPUT_CFG_MAX> paramContainer);

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
    int CheckAppScriptValid(const std::vector<std::string> &appParams) const;
    int MsprofCheckAppValid(std::string &appParam) const;
    int MsprofCheckEnvValid(const std::string &envParam) const;
    int MsprofCheckAiModeValid(const std::string &aiModeParam, const InputCfg aiModeTypeOpt) const;
    int MsprofCheckSysDeviceValid(const std::string &devListParam) const;
    int MsprofCheckSysPeriodValid(const std::string &sysPeriodParam) const;
    int MsprofCheckHostSysValid(const std::string &hostSysParam) const;
    int MsprofCheckHostSysPidValid(const std::string &hostSysPidParam) const;
    int MsprofCheckPythonPathValid(const std::string &pythonPathParam) const;
    int MsprofCheckSummaryFormatValid(const std::string &formatParam) const;
    int MsprofCheckExportIdValid(const std::string &idParam, const std::string &exportIdType) const;
    int CheckHostSysToolsExit(const std::string &hostSysParam, const std::string &resultDir,
        const std::string &appDir) const;
private:   
    int CheckHostSysToolsIsExist(const std::string toolName, const std::string &resultDir,
        const std::string &appDir) const;
    int CheckHostSysCmdOutIsExist(const std::string tmpDir, const std::string toolName,
                                           const MmProcess tmpProcess) const;
    int CheckHostOutString(const std::string tmpStr, const std::string toolName) const;
    int UninitCheckHostSysCmd(const MmProcess checkProcess) const;
private:
    void SpliteAppPath(const std::string &appParams, std::string &cmdPath, std::string &appParameters, std::string &appDir, std::string &app);
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