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
    INPUT_CFG_API_AICORE_EVENT = 5, // del
    // other
    INPUT_CFG_COM_OUTPUT = 6,
    INPUT_CFG_COM_STORAGE_LIMIT = 7,
    INPUT_CFG_COM_MSPROFTX = 8,
    // Task
    INPUT_CFG_COM_TASK_TIME = 9,
    INPUT_CFG_COM_TASK_TRACE = 10,
    INPUT_CFG_COM_TRAINING_TRACE = 11,
    INPUT_CFG_COM_AI_CORE = 12,
    INPUT_CFG_COM_AIC_MODE = 13,
    INPUT_CFG_COM_AIC_METRICS = 14,
    INPUT_CFG_COM_AIC_FREQ = 15,
    INPUT_CFG_COM_AI_VECTOR = 16,
    INPUT_CFG_COM_AIV_MODE = 17,
    INPUT_CFG_COM_AIV_METRICS = 18,
    INPUT_CFG_COM_AIV_FREQ = 19,
    INPUT_CFG_COM_ASCENDCL = 20,
    INPUT_CFG_COM_MODEL_EXECUTION = 21,
    INPUT_CFG_COM_RUNTIME_API = 22,
    INPUT_CFG_COM_HCCL = 23,
    INPUT_CFG_COM_L2 = 24,
    INPUT_CFG_COM_AICPU = 25,
    // System-device
    INPUT_CFG_COM_SYS_DEVICES = 26,
    INPUT_CFG_COM_SYS_PERIOD = 27,
    INPUT_CFG_COM_SYS_USAGE = 28,
    INPUT_CFG_COM_SYS_USAGE_FREQ = 29,
    INPUT_CFG_COM_SYS_PID_USAGE = 30,
    INPUT_CFG_COM_SYS_PID_USAGE_FREQ = 31,
    INPUT_CFG_COM_SYS_CPU = 32,
    INPUT_CFG_COM_SYS_CPU_FREQ = 33,
    INPUT_CFG_COM_SYS_HARDWARE_MEM = 34,
    INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ = 35,
    INPUT_CFG_COM_LLC_MODE = 36,
    INPUT_CFG_COM_SYS_IO = 37,
    INPUT_CFG_COM_SYS_IO_FREQ = 38,
    INPUT_CFG_COM_SYS_INTERCONNECTION = 39,
    INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ = 40,
    INPUT_CFG_COM_DVPP = 41,
    INPUT_CFG_COM_DVPP_FREQ = 42,
    INPUT_CFG_COM_POWER = 43,
    INPUT_CFG_COM_BIU = 44,
    INPUT_CFG_COM_BIU_FREQ = 45,
    // System-host
    INPUT_CFG_HOST_SYS = 46,
    INPUT_CFG_HOST_SYS_PID = 47,
    INPUT_HOST_SYS_USAGE = 48,
    // analysis
    INPUT_CFG_PYTHON_PATH = 49,
    INPUT_CFG_SUMMARY_FORMAT = 50,
    INPUT_CFG_PARSE = 51,
    INPUT_CFG_QUERY = 52,
    INPUT_CFG_EXPORT = 53,
    INPUT_CFG_ITERATION_ID = 54,
    INPUT_CFG_MODEL_ID = 55,
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
private:
    
    int ComValidCheck(EnableType enableType);
    void ComDefValueSet(); // 放到msprofbin中
    //int TransToParam();

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