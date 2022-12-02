/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for acl json
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#include "params_adapter_acljson.h"

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
        INPUT_CFG_COM_BIU_FREQ
    }).swap(aclJsonConfig_);
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
        INPUT_CFG_COM_BIU_FREQ,
        INPUT_CFG_HOST_SYS_USAGE,
        INPUT_CFG_HOST_SYS_USAGE_FREQ
    }).swap(aclJsonWholeConfig_);
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
        {INPUT_CFG_COM_BIU_FREQ, "biu_freq"},
        {INPUT_CFG_HOST_SYS_USAGE, "host_sys_usage"},
        {INPUT_CFG_HOST_SYS_USAGE_FREQ, "host_sys_usage_freq"},
    }).swap(aclJsonPrintMap_);
    return PROFILING_SUCCESS;
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
            case INPUT_CFG_COM_BIU_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
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
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE] = aclCfg->host_sys_usage();
    paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ] = aclCfg->host_sys_usage_freq();
    std::string biuFreqParam = std::to_string(aclCfg->biu_freq());
    if (biuFreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_BIU_FREQ] = biuFreqParam;
    }
    std::string l2FreqParam = std::to_string(aclCfg->l2_freq());
    if (l2FreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_L2_SAMPLE_FREQ] = l2FreqParam;
    }
    for (auto configOpt : aclJsonWholeConfig_) {
        if (!paramContainer_[configOpt].empty()) {
            setConfig_.insert(configOpt);
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
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIC_METRICS];

    paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
    paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty() ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
    SetDefaultAivParams(paramContainer_);
    
    if (!paramContainer_[INPUT_CFG_COM_BIU_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_BIU] = MSVP_PROF_ON;
    }
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
    
    GenAclJsonContainer(aclCfg);

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