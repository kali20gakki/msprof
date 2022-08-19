/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#include "params_adapter.h"

#include <map>

#include "errno/error_code.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::error;

int ParamsAdapter::CheckListInit()
{
    // get platform info
    platformType_ = ConfigManager::instance()->GetPlatformType();
    if (platformType_ < PlatformType::MINI_TYPE || platformType_ >= PlatformType::END_TYPE) {
        return PROFILING_FAILED;
    }
    switch(platformType_) {
        case PlatformType::MINI_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
                INPUT_CFG_COM_L2, INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ,
                INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER,
                INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::CLOUD_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
                INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::MDC_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION,
                INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_AICPU, INPUT_CFG_PYTHON_PATH,
                INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY, INPUT_CFG_EXPORT,
                INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
                INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::LHISI_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
                INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
                INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
                INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_DVPP, INPUT_CFG_COM_DVPP_FREQ,
                INPUT_CFG_COM_SYS_CPU, INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_L2,
                INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE,
                INPUT_CFG_QUERY, INPUT_CFG_EXPORT, INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID,
                INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::DC_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
                INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::CHIP_V4_1_0:
            std::vector<InputCfg>({}).swap(blackSwitch_);
            break;
        default:
            return PROFILING_FAILED;
    }
    // init common config list
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT, INPUT_CFG_COM_STORAGE_LIMIT, INPUT_CFG_COM_MSPROFTX,
        INPUT_CFG_COM_TASK_TIME, INPUT_CFG_COM_TASK_TRACE, INPUT_CFG_COM_TRAINING_TRACE,
        INPUT_CFG_COM_AI_CORE, INPUT_CFG_COM_AIC_MODE, INPUT_CFG_COM_AIC_METRICS,
        INPUT_CFG_COM_AIC_FREQ, INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_ASCENDCL,
        INPUT_CFG_COM_MODEL_EXECUTION, INPUT_CFG_COM_RUNTIME_API, INPUT_CFG_COM_HCCL,
        INPUT_CFG_COM_L2, INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_DEVICES,
        INPUT_CFG_COM_SYS_PERIOD, INPUT_CFG_COM_SYS_USAGE, INPUT_CFG_COM_SYS_USAGE_FREQ,
        INPUT_CFG_COM_SYS_PID_USAGE, INPUT_CFG_COM_SYS_PID_USAGE_FREQ, INPUT_CFG_COM_SYS_CPU,
        INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_DVPP, INPUT_CFG_COM_DVPP_FREQ, INPUT_CFG_COM_POWER,
        INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ, INPUT_CFG_HOST_SYS, INPUT_CFG_HOST_SYS_PID,
        INPUT_HOST_SYS_USAGE
    }).swap(commonConfig_);
    return PROFILING_SUCCESS;
}

bool ParamsAdapter::BlackSwitchCheck(InputCfg inputCfg) const
{
    if (inputCfg < INPUT_CFG_MSPROF_APPLICATION || inputCfg >= INPUT_CFG_MAX) {
        return false;
    }
    if (std::find(blackSwitch_.begin(), blackSwitch_.end(), inputCfg) != blackSwitch_.end()) {
        return false;
    }
    return true;
}

PlatformType ParamsAdapter::GetPlatform() const
{
    return platformType_;
}
} // ParamsAdapter
} // Dvvp
} // Collector