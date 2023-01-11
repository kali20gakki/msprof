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
const int HOST_SYS_USAGE_FREQ_MIN = 1;
const int HOST_SYS_USAGE_FREQ_MAX = 50;
const int L2_SAMPLE_FREQ_MIN = 1;
const int L2_SAMPLE_FREQ_MAX = 100;
const int APP_PID_MIN = 1;
const int APP_PID_MAX = 2147483647;
const int ACC_PMU_MODE_THRED = 5000; // Check
enum InputCfg {
    INPUT_CFG_MSPROF_APPLICATION = 0,
    INPUT_CFG_MSPROF_ENVIRONMENT,
    INPUT_CFG_MSPROF_DYNAMIC,
    INPUT_CFG_MSPROF_DYNAMIC_PID,
    // other
    INPUT_CFG_COM_OUTPUT = 10,
    INPUT_CFG_COM_STORAGE_LIMIT,
    INPUT_CFG_COM_MSPROFTX,
    // Task
    INPUT_CFG_COM_TASK_TIME = 30,
    INPUT_CFG_COM_TASK_TRACE,
    INPUT_CFG_COM_TRAINING_TRACE,
    INPUT_CFG_COM_AI_CORE,
    INPUT_CFG_COM_AIC_MODE,
    INPUT_CFG_COM_AIC_METRICS,
    INPUT_CFG_COM_AIC_FREQ,
    INPUT_CFG_COM_AI_VECTOR,
    INPUT_CFG_COM_AIV_MODE,
    INPUT_CFG_COM_AIV_METRICS,
    INPUT_CFG_COM_AIV_FREQ,
    INPUT_CFG_COM_ASCENDCL,
    INPUT_CFG_COM_MODEL_EXECUTION,
    INPUT_CFG_COM_RUNTIME_API,
    INPUT_CFG_COM_HCCL,
    INPUT_CFG_COM_L2,
    INPUT_CFG_COM_AICPU,
    INPUT_CFG_COM_L2_SAMPLE_FREQ,
    // System-device
    INPUT_CFG_COM_SYS_DEVICES = 50,
    INPUT_CFG_COM_SYS_PERIOD,
    INPUT_CFG_COM_SYS_USAGE,
    INPUT_CFG_COM_SYS_USAGE_FREQ,
    INPUT_CFG_COM_SYS_PID_USAGE,
    INPUT_CFG_COM_SYS_PID_USAGE_FREQ,
    INPUT_CFG_COM_SYS_CPU,
    INPUT_CFG_COM_SYS_CPU_FREQ,
    INPUT_CFG_COM_SYS_HARDWARE_MEM,
    INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
    INPUT_CFG_COM_LLC_MODE,
    INPUT_CFG_COM_SYS_IO,
    INPUT_CFG_COM_SYS_IO_FREQ,
    INPUT_CFG_COM_SYS_INTERCONNECTION,
    INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
    INPUT_CFG_COM_DVPP,
    INPUT_CFG_COM_DVPP_FREQ,
    INPUT_CFG_COM_POWER,
    INPUT_CFG_COM_BIU,
    INPUT_CFG_COM_BIU_FREQ,
    // System-host
    INPUT_CFG_HOST_SYS = 80,
    INPUT_CFG_HOST_SYS_PID,
    INPUT_CFG_HOST_SYS_USAGE,
    INPUT_CFG_HOST_SYS_USAGE_FREQ,
    // analysis
    INPUT_CFG_SUMMARY_FORMAT = 100,
    INPUT_CFG_PARSE,
    INPUT_CFG_QUERY,
    INPUT_CFG_EXPORT,
    INPUT_CFG_ITERATION_ID,
    INPUT_CFG_MODEL_ID,
    INPUT_CFG_PYTHON_PATH,
    INPUT_CFG_MAX
};

enum class EnableType {
    ENABLE_MSPROF,
    ENABLE_ACL_JSON,
    ENABLE_GE_OPTION,
    ENABLE_API
};

class ParamsAdapter {
public:
    ParamsAdapter() : platformType_(PlatformType::MINI_TYPE), platformAdapter_(nullptr) {};
    virtual ~ParamsAdapter();
    int CheckListInit();
    bool BlackSwitchCheck(InputCfg inputCfg) const;
    PlatformType GetPlatform() const;
    int ComCfgCheck(std::array<std::string, INPUT_CFG_MAX> paramContainer, std::set<InputCfg> &setArgs) const;
    int TransToParam(std::array<std::string, INPUT_CFG_MAX> paramContainer, SHARED_PTR_ALIA<ProfileParams> params);
    void SetDefaultAivParams(std::array<std::string, INPUT_CFG_MAX> &paramContainer) const;
public:
    bool CheckFreqValid(const std::string &freq, const InputCfg freqOpt) const;
private:
    void SetMiniBlackSwitch();
    void SetCloudBlackSwitch();
    void SetMdcBlackSwitch();
    void SetLhisiBlackSwitch();
    void SetDcBlackSwitch();
    void SetCloudV2BlackSwitch();
    void SetMiniV2BlackSwitch();
    void SetCommonConfig();
    void SetCommonParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetTaskParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetAiMetricsParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetDeviceSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetHostSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    void SetHostSysUsageParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const;
    bool ComCfgCheck1(const InputCfg inputCfg, const std::string &cfgValue) const;
    bool ComCfgCheck2(const InputCfg inputCfg, const std::string &cfgValue) const;

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