/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for acl json
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#include "params_adapter_acljson.h"
#include <utility>
#include <vector>
#include <algorithm>
#include "errno/error_code.h"
#include "param_validation.h"
#include "config/config.h"
#include "utils.h"
namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::utils;
int ParamsAdapterAclJson::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Acl json]params adapter init failed.");
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_INSTR_PROFILING_FREQ, INPUT_CFG_HOST_SYS
        }).swap(aclJsonConfig_);
    InitWholeConfigMap();
    InitPrintMap();
    return PROFILING_SUCCESS;
}

void ParamsAdapterAclJson::InitWholeConfigMap()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT,
        INPUT_CFG_COM_STORAGE_LIMIT,
        INPUT_CFG_COM_MSPROFTX,
        INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_AICPU,
        INPUT_CFG_COM_L2,
        INPUT_CFG_COM_HCCL,
        INPUT_CFG_COM_ASCENDCL,
        INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_AIC_METRICS,
        INPUT_CFG_COM_AIV_METRICS,
        INPUT_CFG_COM_INSTR_PROFILING_FREQ,
        INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE,
        INPUT_CFG_COM_SYS_IO_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_DVPP_FREQ,
        INPUT_CFG_HOST_SYS,
        INPUT_CFG_HOST_SYS_USAGE,
        INPUT_CFG_HOST_SYS_USAGE_FREQ
        }).swap(aclJsonWholeConfig_);
}

void ParamsAdapterAclJson::InitPrintMap()
{
    std::map<InputCfg, std::string>({
        {INPUT_CFG_COM_OUTPUT, "output"},
        {INPUT_CFG_COM_STORAGE_LIMIT, "storage_limit"},
        {INPUT_CFG_COM_MSPROFTX, "msproftx"},
        {INPUT_CFG_COM_TASK_TIME, "task_time"},
        {INPUT_CFG_COM_AICPU, "aicpu"},
        {INPUT_CFG_COM_L2, "l2"},
        {INPUT_CFG_COM_HCCL, "hccl"},
        {INPUT_CFG_COM_ASCENDCL, "ascendcl"},
        {INPUT_CFG_COM_RUNTIME_API, "runtime_api"},
        {INPUT_CFG_COM_AIC_METRICS, "aic_metrics"},
        {INPUT_CFG_COM_AIV_METRICS, "aiv_metrics"},
        {INPUT_CFG_COM_INSTR_PROFILING_FREQ, "instr_profiling_freq"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, "sys_hardware_mem_freq"},
        {INPUT_CFG_COM_LLC_MODE, "llc_profiling"},
        {INPUT_CFG_COM_SYS_IO_FREQ, "sys_io_sampling_freq"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, "sys_interconnection_freq"},
        {INPUT_CFG_COM_DVPP_FREQ, "dvpp_freq"},
        {INPUT_CFG_HOST_SYS, "host_sys"},
        {INPUT_CFG_HOST_SYS_USAGE, "host_sys_usage"},
        {INPUT_CFG_HOST_SYS_USAGE_FREQ, "host_sys_usage_freq"},
        }).swap(aclJsonPrintMap_);
}

bool ParamsAdapterAclJson::CheckHostSysAclJsonValid(const std::string &cfgStr) const
{
    if (cfgStr.empty()) {
        MSPROF_LOGE("Config: host_sys is empty. Please input in the range of "
            "'cpu|mem'.");
        return false;
    }
    std::vector<std::string> cfgStrVec = Utils::Split(cfgStr, false, "", ",");
    for (auto cfg : cfgStrVec) {
        if (cfg.compare(HOST_SYS_CPU) != 0 && cfg.compare(HOST_SYS_MEM) != 0) {
            MSPROF_LOGE("The value [%s] of host_sys is not support. Please input in the range of "
                "'cpu|mem'.", cfg.c_str());
            return false;
        }
    }
    return true;
}

int ParamsAdapterAclJson::ParamsCheckAclJson() const
{
    bool ret = true;
    for (auto inputCfg : aclJsonConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_INSTR_PROFILING_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            case INPUT_CFG_HOST_SYS:
                ret = CheckHostSysAclJsonValid(cfgValue);
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

void ParamsAdapterAclJson::GenAclJsonContainer(SHARED_PTR_ALIA<ProfAclConfig> aclCfg)
{
    paramContainer_[INPUT_CFG_COM_OUTPUT] = aclCfg->output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = aclCfg->storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = aclCfg->msproftx();
    paramContainer_[INPUT_CFG_COM_TASK_TIME] = aclCfg->task_time();
    paramContainer_[INPUT_CFG_COM_AICPU] = aclCfg->aicpu();
    paramContainer_[INPUT_CFG_COM_L2] = aclCfg->l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = aclCfg->hccl();
    paramContainer_[INPUT_CFG_COM_ASCENDCL] = aclCfg->ascendcl();
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = aclCfg->runtime_api();
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = aclCfg->aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = aclCfg->aiv_metrics();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = (aclCfg->sys_hardware_mem_freq() <= 0) ? "" :
        std::to_string(aclCfg->sys_hardware_mem_freq());
    paramContainer_[INPUT_CFG_COM_LLC_MODE] = aclCfg->llc_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ] = (aclCfg->sys_io_sampling_freq() <= 0) ? "" :
        std::to_string(aclCfg->sys_io_sampling_freq());
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = (aclCfg->sys_interconnection_freq() <= 0) ? "" :
        std::to_string(aclCfg->sys_interconnection_freq());
    paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = (aclCfg->dvpp_freq() <= 0) ? "" :
        std::to_string(aclCfg->dvpp_freq());
    paramContainer_[INPUT_CFG_HOST_SYS] = aclCfg->host_sys();
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE] = aclCfg->host_sys_usage();
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ] = (aclCfg->host_sys_usage_freq() <= 0) ? "" :
        std::to_string(aclCfg->host_sys_usage_freq());
    std::string instrFreqParam = std::to_string(aclCfg->instr_profiling_freq());
    if (instrFreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_INSTR_PROFILING_FREQ] = instrFreqParam;
    }
    for (auto configOpt : aclJsonWholeConfig_) {
        if (!paramContainer_[configOpt].empty()) {
            setConfig_.insert(configOpt);
        }
    }
}

void ParamsAdapterAclJson::SetAclJsonContainerSysValue()
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

void ParamsAdapterAclJson::SetAclJsonContainerDefaultValue()
{
    paramContainer_[INPUT_CFG_COM_OUTPUT] = SetOutputDir(paramContainer_[INPUT_CFG_COM_OUTPUT]);
    if (paramContainer_[INPUT_CFG_COM_ASCENDCL].empty()) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_RUNTIME_API].empty()) {
        paramContainer_[INPUT_CFG_COM_RUNTIME_API] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_MODEL_EXECUTION] = MSVP_PROF_ON;
    if (paramContainer_[INPUT_CFG_COM_TASK_TIME].empty()) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
    paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty() ?
        SetDefaultAicMetricsType() : paramContainer_[INPUT_CFG_COM_AIC_METRICS];

    paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
    paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty() ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
    SetDefaultAivParams(paramContainer_);
    
    if (!paramContainer_[INPUT_CFG_COM_INSTR_PROFILING_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_INSTR_PROFILING] = MSVP_PROF_ON;
    }
    SetAclJsonContainerSysValue();
    SetDefaultLlcMode(paramContainer_);
}

std::string ParamsAdapterAclJson::SetOutputDir(const std::string &outputDir) const
{
    std::string result;
    if (outputDir.empty()) {
        MSPROF_LOGI("No output set, use default path");
    } else {
        std::string path = Utils::RelativePathToAbsolutePath(outputDir);
        if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
        }
        result = Utils::CanonicalizePath(path);
    }
    if (result.empty() || !Utils::IsDirAccessible(result)) {
        MSPROF_LOGI("No output set or is not accessible, use app dir instead");
        result = Utils::GetSelfPath();
        size_t pos = result.rfind(MSVP_SLASH);
        if (pos != std::string::npos) {
            result = result.substr(0, pos + 1);
        }
    }
    MSPROF_LOGI("Profiling result path: %s", Utils::BaseName(result).c_str());

    return result;
}

bool ParamsAdapterAclJson::CheckInstrAndTaskParamBothSet(SHARED_PTR_ALIA<ProfAclConfig> aclCfg)
{
    std::string instrFreqParam = std::to_string(aclCfg->instr_profiling_freq());
    if (instrFreqParam.compare("0") == 0) {
        return false;
    }
    const std::vector<std::pair<bool, std::string>> ARG_VEC {
        { aclCfg->task_time() == "on", " task_time " },
        { aclCfg->aicpu() == "on", " aicpu " },
        { aclCfg->l2() == "on", " l2 " },
        { aclCfg->hccl() == "on", " hccl " },
        { aclCfg->ascendcl() == "on", " acsendcl " },
        { aclCfg->runtime_api() == "on", " runtime_api " },
        { !aclCfg->aic_metrics().empty(), " aic_metrics " },
        { !aclCfg->aiv_metrics().empty(), " aiv_metrics " }
    };
    bool anyComflict = std::any_of(ARG_VEC.begin(), ARG_VEC.end(), [](std::pair<bool, std::string> arg) {
        return arg.first;
    });
    if (anyComflict) {
        std::string comflictStr = "";
        for (auto arg : ARG_VEC) {
            if (arg.first) {
                comflictStr.append(arg.second);
            }
        }
        MSPROF_LOGE("[Acl json] Profiling fails to start because instr_profiling_freq is set,"
            " Params %s not allowed to set in single operator model if instr_profiling_freq is set.",
            comflictStr.c_str());
        return true;
    }
    return false;
}

int ParamsAdapterAclJson::GetParamFromInputCfg(SHARED_PTR_ALIA<ProfAclConfig> aclCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory for params is empty.");
        return PROFILING_FAILED;
    }
    if (!aclCfg) {
        MSPROF_LOGE("memory for acljson config is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]acljson Init failed.");
        return PROFILING_FAILED;
    }
    if (CheckInstrAndTaskParamBothSet(aclCfg)) {
        return PROFILING_FAILED;
    }
    GenAclJsonContainer(aclCfg);
    ret = PlatformAdapterInit(params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    ret = ParamsCheckAclJson();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("private param check fail.");
        return PROFILING_FAILED;
    }

    ret = ComCfgCheck(paramContainer_, setConfig_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("common param check fail.");
        return PROFILING_FAILED;
    }

    SetAclJsonContainerDefaultValue();

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("acl json set params fail.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
} // ParamsAdapter
} // Dvvp
} // Collector
