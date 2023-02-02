/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for ge option/env
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-05
 */

#include "params_adapter_geopt.h"

#include "errno/error_code.h"
#include "param_validation.h"
#include "prof_params.h"
#include "config/config.h"
#include "platform/platform.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
int ParamsAdapterGeOpt::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Ge opt]params adapter init failed.");
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_TASK_TRACE, INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_BIU_FREQ, INPUT_CFG_HOST_SYS
    }).swap(geOptConfig_);
    InitWholeConfigMap();
    InitPrintMap();
    return PROFILING_SUCCESS;
}

void ParamsAdapterGeOpt::InitWholeConfigMap()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT,
        INPUT_CFG_COM_STORAGE_LIMIT,
        INPUT_CFG_COM_MSPROFTX,
        INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_TASK_TRACE,
        INPUT_CFG_COM_TRAINING_TRACE,
        INPUT_CFG_COM_AICPU,
        INPUT_CFG_COM_L2,
        INPUT_CFG_COM_HCCL,
        INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_AIC_METRICS,
        INPUT_CFG_COM_AIV_METRICS,
        INPUT_CFG_COM_BIU_FREQ,
        INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE,
        INPUT_CFG_COM_SYS_IO_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_DVPP_FREQ,
        INPUT_CFG_HOST_SYS,
        INPUT_CFG_HOST_SYS_USAGE,
        INPUT_CFG_HOST_SYS_USAGE_FREQ
    }).swap(geOptionsWholeConfig_);
}

void ParamsAdapterGeOpt::InitPrintMap()
{
    std::map<InputCfg, std::string>({
        {INPUT_CFG_COM_OUTPUT, "output"},
        {INPUT_CFG_COM_STORAGE_LIMIT, "storage_limit"},
        {INPUT_CFG_COM_MSPROFTX, "msproftx"},
        {INPUT_CFG_COM_TASK_TIME, "task_time"},
        {INPUT_CFG_COM_TASK_TRACE, "task_trace"},
        {INPUT_CFG_COM_TRAINING_TRACE, "training_trace"},
        {INPUT_CFG_COM_AICPU, "aicpu"},
        {INPUT_CFG_COM_L2, "l2"},
        {INPUT_CFG_COM_HCCL, "hccl"},
        {INPUT_CFG_COM_RUNTIME_API, "runtime_api"},
        {INPUT_CFG_COM_AIC_METRICS, "aic_metrics"},
        {INPUT_CFG_COM_AIV_METRICS, "aiv_metrics"},
        {INPUT_CFG_COM_BIU_FREQ, "biu_freq"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, "sys_hardware_mem_freq"},
        {INPUT_CFG_COM_LLC_MODE, "llc_profiling"},
        {INPUT_CFG_COM_SYS_IO_FREQ, "sys_io_sampling_freq"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, "sys_interconnection_freq"},
        {INPUT_CFG_COM_DVPP_FREQ, "dvpp_freq"},
        {INPUT_CFG_HOST_SYS, "host_sys"},
        {INPUT_CFG_HOST_SYS_USAGE, "host_sys_usage"},
        {INPUT_CFG_HOST_SYS_USAGE_FREQ, "host_sys_usage_freq"},
    }).swap(geOptionsPrintMap_);
}

bool ParamsAdapterGeOpt::CheckHostSysGeOptValid(const std::string &cfgStr) const
{
    if (cfgStr.empty()) {
        MSPROF_LOGE("Config: host_sys is empty. Please input in the range of "
            "'cpu|mem'.");
        return false;
    }
    std::vector<std::string> cfgStrVec = Utils::Split(cfgStr, false, "", ",");
    for (auto cfg : cfgStrVec) {
        if (cfg.compare(HOST_SYS_CPU) != 0 && cfg.compare(HOST_SYS_MEM) != 0) {
            MSPROF_LOGE("The value: %s of host_sys is not support. Please input in the range of "
                "'cpu|mem'.", cfg.c_str());
            return false;
        }
    }
    return true;
}

int ParamsAdapterGeOpt::ParamsCheckGeOpt() const
{
    bool ret = true;
    for (auto inputCfg : geOptConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_TASK_TRACE:
            case INPUT_CFG_COM_TRAINING_TRACE:
                ret = ParamValidation::instance()->IsValidSwitch(cfgValue);
                break;
            case INPUT_CFG_COM_BIU_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            case INPUT_CFG_HOST_SYS:
                ret = CheckHostSysGeOptValid(cfgValue);
                break;
            default:
                ret = false;
        }
        if (ret != true) {
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

void ParamsAdapterGeOpt::GenGeOptionsContainer(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg)
{
    paramContainer_[INPUT_CFG_COM_OUTPUT] = geCfg->output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = geCfg->storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = geCfg->msproftx();
    paramContainer_[INPUT_CFG_COM_TASK_TIME] = geCfg->task_time();
    paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = geCfg->training_trace();
    paramContainer_[INPUT_CFG_COM_TASK_TRACE] = geCfg->task_trace();
    paramContainer_[INPUT_CFG_COM_AICPU] = geCfg->aicpu();
    paramContainer_[INPUT_CFG_COM_L2] = geCfg->l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = geCfg->hccl();
    paramContainer_[INPUT_CFG_COM_ASCENDCL] = "off";
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = geCfg->runtime_api();
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = geCfg->aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = geCfg->aiv_metrics();
    paramContainer_[INPUT_CFG_COM_POWER] = geCfg->power();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = (geCfg->sys_hardware_mem_freq() <= 0) ? "" :
        std::to_string(geCfg->sys_hardware_mem_freq());
    paramContainer_[INPUT_CFG_COM_LLC_MODE] = geCfg->llc_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ] = (geCfg->sys_io_sampling_freq() <= 0) ? "" :
        std::to_string(geCfg->sys_io_sampling_freq());
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = (geCfg->sys_interconnection_freq() <= 0) ? "" :
        std::to_string(geCfg->sys_interconnection_freq());
    paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = (geCfg->dvpp_freq() <= 0) ? "" :
        std::to_string(geCfg->dvpp_freq());
    paramContainer_[INPUT_CFG_HOST_SYS] = geCfg->host_sys();
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE] = geCfg->host_sys_usage();
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ] = (geCfg->host_sys_usage_freq() <= 0) ? "" :
        std::to_string(geCfg->host_sys_usage_freq());
    std::string biuFreqParam = std::to_string(geCfg->biu_freq());
    if (biuFreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_BIU_FREQ] = biuFreqParam;
    }
    for (auto configOpt : geOptionsWholeConfig_) {
        if (!paramContainer_[configOpt].empty()) {
            setConfig_.insert(configOpt);
        }
    }
}

void ParamsAdapterGeOpt::SetGeOptContainerSysValue()
{
    const std::unordered_map<int, InputCfg> sysConfigMap = {
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM},
        {INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_IO},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION},
        {INPUT_CFG_COM_DVPP_FREQ, INPUT_CFG_COM_DVPP}
    };
    for (auto kv : sysConfigMap) {
        std::string configStr = paramContainer_[kv.first];
        if (!configStr.empty()) {
            paramContainer_[kv.second] = MSVP_PROF_ON;
        }
    }
}

int ParamsAdapterGeOpt::SetGeOptionsContainerDefaultValue()
{
    if (!Platform::instance()->PlatformIsHelperHostSide()) {
        int ret = SetOutputDir(paramContainer_[INPUT_CFG_COM_OUTPUT]);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    } else {
        paramContainer_[INPUT_CFG_COM_OUTPUT].clear();
    }
    if (!paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
    }
    if (!paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
    }
    SetDefaultAivParams(paramContainer_);
    if (!paramContainer_[INPUT_CFG_COM_BIU_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_BIU] = MSVP_PROF_ON;
    }
    SetGeOptContainerSysValue();
    SetDefaultLlcMode(paramContainer_);
    return PROFILING_SUCCESS;
}

int ParamsAdapterGeOpt::SetOutputDir(std::string &outputDir) const
{
    std::string result;
    if (outputDir.empty()) {
        MSPROF_LOGE("Result path is empty");
        return PROFILING_FAILED;
    }
    std::string path = Utils::RelativePathToAbsolutePath(outputDir);
    if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
    }
    result = analysis::dvvp::common::utils::Utils::CanonicalizePath(path);
    if (result.empty() || !analysis::dvvp::common::utils::Utils::IsDirAccessible(result)) {
        MSPROF_LOGE("Result path is not accessible or not exist, result path: %s", Utils::BaseName(outputDir).c_str());
        return PROFILING_FAILED;
    }
    outputDir = result;
    return PROFILING_SUCCESS;
}

int ParamsAdapterGeOpt::GetParamFromInputCfg(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory for params is empty.");
        return PROFILING_FAILED;
    }
    if (!geCfg) {
        MSPROF_LOGE("memory for ge options config is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]ge options Init failed.");
        return PROFILING_FAILED;
    }

    GenGeOptionsContainer(geCfg);

    ret = ParamsCheckGeOpt();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("private param check fail.");
        return PROFILING_FAILED;
    }

    ret = ComCfgCheck(paramContainer_, setConfig_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("common param check fail.");
        return PROFILING_FAILED;
    }

    ret = SetGeOptionsContainerDefaultValue();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ge opt set params fail.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
} // ParamsAdapter
} // Dvvp
} // Collector