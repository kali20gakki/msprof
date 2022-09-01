/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::message::ProfileParams;
using Analysis::Dvvp::Common::Config::PlatformType;
using analysis::dvvp::message::StatusInfo;
using namespace Collector::Dvvp::Common::PlatformAdapter;

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
enum InputCfg {
    INPUT_CFG_MSPROF_APPLICATION = 0,
    INPUT_CFG_MSPROF_ENVIRONMENT = 1,
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
    virtual ~ParamsAdapter();
    int CheckListInit();
    bool BlackSwitchCheck(InputCfg inputCfg) const;
    PlatformType GetPlatform() const;
    int ComCfgCheck(EnableType enableType, std::array<std::string, INPUT_CFG_MAX> paramContainer,
        std::set<InputCfg> &setArgs,
        std::vector<std::pair<InputCfg, std::string>> &cfgList) const;
    int TransToParam(std::array<std::string, INPUT_CFG_MAX> paramContainer, SHARED_PTR_ALIA<ProfileParams> params);
public:
    bool CheckFreqValid(const std::string &freq, const InputCfg freqOpt) const;
private:
    void SetMiniBlackSwitch();
    void SetCloudBlackSwitch();
    void SetMdcBlackSwitch();
    void SetLhisiBlackSwitch();
    void SetDcBlackSwitch();
    void SetCloudV2BlackSwitch();
    void SetCommonConfig();
    void SetCommonParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetTaskParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetAiMetricsParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetDeviceSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetHostSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetHostSysUsageParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;

private:
    std::vector<InputCfg> commonConfig_;
    std::vector<InputCfg> blackSwitch_;
    StatusInfo statusInfo_;
    PlatformType platformType_;
    SHARED_PTR_ALIA<PlatformAdapterInterface> platformAdapter_;
};
} // ParamsAdapter
} // Dvpp
} // Collector

#endif