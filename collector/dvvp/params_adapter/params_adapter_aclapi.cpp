/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for acl API
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#include "params_adapter_aclapi.h"

#include "config/config.h"
#include "config_manager.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "prof_params.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::message;

int ParamsAdapterAclApi::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Acl api]params adapter init failed.");
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_SYS_DEVICES, INPUT_CFG_HOST_SYS
        }).swap(aclApiConfig_);
    return ret;
}

int ParamsAdapterAclApi::ParamsCheckAclApi() const
{
    bool ret = true;
    for (auto inputCfg : aclApiConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_TRAINING_TRACE:
                ret = ParamValidation::instance()->IsValidSwitch(cfgValue);
                break;
            case INPUT_CFG_COM_SYS_DEVICES:
                ret = ParamValidation::instance()->MsprofCheckSysDeviceValid(cfgValue);
                break;
            case INPUT_CFG_HOST_SYS:
                ret = CheckHostSysAclApiValid(cfgValue);
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

bool ParamsAdapterAclApi::CheckHostSysAclApiValid(const std::string &cfgStr) const
{
    if (cfgStr.empty()) {
        MSPROF_LOGE("Config: ACL_PROF_HOST_SYS is empty. Please input in the range of "
            "'cpu|mem'.");
        return false;
    }
    std::vector<std::string> cfgStrVec = Utils::Split(cfgStr, false, "", ",");
    for (auto cfg : cfgStrVec) {
        if (cfg.compare(HOST_SYS_CPU) != 0 && cfg.compare(HOST_SYS_MEM) != 0) {
            MSPROF_LOGE("The value: %s of ACL_PROF_HOST_SYS is not support. Please input in the range of "
                "'cpu|mem'.", cfg.c_str());
            return false;
        }
    }
    return true;
}

std::string ParamsAdapterAclApi::DevIdToStr(uint32_t devNum, const uint32_t *devList) const
{
    std::string devStr;
    bool flag = false;
    for (uint32_t i = 0; i < devNum; i++) {
        if (devList[i] == DEFAULT_HOST_ID) {
            continue; // except Host id
        }
        if (!flag) {
            flag = true;
            devStr += std::to_string(devList[i]);
            continue;
        }
        devStr += ",";
        devStr += std::to_string(devList[i]);
    }
    return devStr;
}

void ParamsAdapterAclApi::ProfTaskCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    std::string devStr = DevIdToStr(apiCfg->devNums, apiCfg->devIdList);
    if (!devStr.empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_DEVICES] = devStr;
        setConfig_.insert(INPUT_CFG_COM_SYS_DEVICES);
    }
    if (!argsArr[ACL_PROF_STORAGE_LIMIT].empty()) {
        paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = argsArr[ACL_PROF_STORAGE_LIMIT];
        setConfig_.insert(INPUT_CFG_COM_STORAGE_LIMIT);
    }
    ProfMetricsCfgToContainer(apiCfg->aicoreMetrics, apiCfg->dataTypeConfig, argsArr);
    const std::map<uint64_t, InputCfg> profCfgList = {
        {PROF_TASK_TIME_MASK, INPUT_CFG_COM_TASK_TIME_L1},
        {PROF_TASK_TIME_L0_MASK, INPUT_CFG_COM_TASK_TIME_L0},
        {PROF_KEYPOINT_TRACE_MASK, INPUT_CFG_COM_TRAINING_TRACE},
        {PROF_L2CACHE_MASK, INPUT_CFG_COM_L2},
        {PROF_ACL_API_MASK, INPUT_CFG_COM_ASCENDCL},
        {PROF_AICPU_TRACE_MASK, INPUT_CFG_COM_AICPU},
        {PROF_RUNTIME_API_MASK, INPUT_CFG_COM_RUNTIME_API},
        {PROF_HCCL_TRACE_MASK, INPUT_CFG_COM_HCCL},
        {PROF_MSPROFTX_MASK, INPUT_CFG_COM_MSPROFTX},
    };
    for (auto cfg : profCfgList) {
        if (apiCfg->dataTypeConfig & cfg.first) {
            paramContainer_[cfg.second] = MSVP_PROF_ON;
            setConfig_.insert(cfg.second);
        }
    }
}

void ParamsAdapterAclApi::ProfMetricsCfgToContainer(const ProfAicoreMetrics aicMetrics,
    const uint64_t dataTypeConfig, std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    if ((dataTypeConfig & PROF_AICORE_METRICS_MASK) && !metrics.empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AI_CORE);
        paramContainer_[INPUT_CFG_COM_AIC_METRICS] = metrics;
        setConfig_.insert(INPUT_CFG_COM_AIC_METRICS);
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
        setConfig_.insert(INPUT_CFG_COM_AIC_MODE);
    }
    if (!argsArr[ACL_PROF_AIV_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AI_VECTOR);
        paramContainer_[INPUT_CFG_COM_AIV_METRICS] = argsArr[ACL_PROF_AIV_METRICS];
        setConfig_.insert(INPUT_CFG_COM_AIV_METRICS);
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
        setConfig_.insert(INPUT_CFG_COM_AIV_MODE);
    }
    SetDefaultAivParams(paramContainer_);
    return;
}

void ParamsAdapterAclApi::ProfSystemCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    std::map<aclprofConfigType, std::vector<InputCfg>> aclToInput = {
        {ACL_PROF_SYS_HARDWARE_MEM_FREQ, {INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ}},
        {ACL_PROF_SYS_IO_FREQ, {INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ}},
        {ACL_PROF_SYS_INTERCONNECTION_FREQ,
            {INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ}},
        {ACL_PROF_DVPP_FREQ, {INPUT_CFG_COM_DVPP, INPUT_CFG_COM_DVPP_FREQ}},
    };
    for (auto iter : aclToInput) {
        if (!argsArr[iter.first].empty()) {
            if (iter.second[0] != INPUT_CFG_MAX) {
                paramContainer_[iter.second[0]] = MSVP_PROF_ON;
                setConfig_.insert(iter.second[0]);
            }
            paramContainer_[iter.second[1]] = argsArr[iter.first];
            setConfig_.insert(iter.second[1]);
        }
    }
    if (!argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ].empty()) {
        if (!argsArr[ACL_PROF_LLC_MODE].empty()) {
            paramContainer_[INPUT_CFG_COM_LLC_MODE] = argsArr[ACL_PROF_LLC_MODE];
        } else {
            SetDefaultLlcMode(paramContainer_);
        }
        setConfig_.insert(INPUT_CFG_COM_LLC_MODE);
    }
    std::vector<std::pair<aclprofConfigType, InputCfg>> hostSysConfigs = {
        {ACL_PROF_HOST_SYS, INPUT_CFG_HOST_SYS},
        {ACL_PROF_HOST_SYS_USAGE, INPUT_CFG_HOST_SYS_USAGE},
        {ACL_PROF_HOST_SYS_USAGE_FREQ, INPUT_CFG_HOST_SYS_USAGE_FREQ}
    };
    for (auto hostStsConfig : hostSysConfigs) {
        if (!argsArr[hostStsConfig.first].empty()) {
            paramContainer_[hostStsConfig.second] = argsArr[hostStsConfig.first];
            setConfig_.insert(hostStsConfig.second);
        }
    }
}

void ParamsAdapterAclApi::ProfCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    ProfTaskCfgToContainer(apiCfg, argsArr);
    ProfSystemCfgToContainer(apiCfg, argsArr);
}

int ParamsAdapterAclApi::GetParamFromInputCfg(const ProfConfig *apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init Failed");
        return PROFILING_FAILED;
    }
    if (!params_->result_dir.empty()) {
        paramContainer_[INPUT_CFG_COM_OUTPUT] = params_->result_dir;
        setConfig_.insert(INPUT_CFG_COM_OUTPUT);
    }
    ProfCfgToContainer(apiCfg, argsArr);

    ret = PlatformAdapterInit(params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = ParamsCheckAclApi();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("private param check fail.");
        return PROFILING_FAILED;
    }
    ret = ComCfgCheck(paramContainer_, setConfig_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("common param check fail.");
        return PROFILING_FAILED;
    }

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("acl api set params fail.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
} // ParamsAdapter
} // Dvvp
} // Collector