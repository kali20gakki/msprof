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
        INPUT_CFG_COM_TASK_TRACE, INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_BIU_FREQ
    }).swap(geOptConfig_);
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
        INPUT_CFG_COM_BIU_FREQ
    }).swap(geOptionsWholeConfig_);
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
    }).swap(geOptionsPrintMap_);
    return PROFILING_SUCCESS;
}

int ParamsAdapterGeOpt::ParamsCheckGeOpt(std::vector<std::pair<InputCfg, std::string>> &cfgList) const
{
    bool ret = true;
    bool flag = true;
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
            default:
                ret = false;
        }
        if (ret != true) {
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
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
    if (!paramContainer_[INPUT_CFG_COM_BIU_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_BIU] = MSVP_PROF_ON;
    }
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

    std::vector<std::pair<InputCfg, std::string>> errCfgList;
    ret = ParamsCheckGeOpt(errCfgList);
    if (ret == PROFILING_FAILED && !errCfgList.empty()) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                geOptionsPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
        return PROFILING_FAILED;
    }

    errCfgList.clear();
    ret = ComCfgCheck(paramContainer_, setConfig_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                geOptionsPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
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