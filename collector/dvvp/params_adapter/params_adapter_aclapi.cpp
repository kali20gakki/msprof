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
        INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_SYS_DEVICES
    }).swap(aclApiConfig_);
    return ret;
}

int ParamsAdapterAclApi::ParamsCheckAclApi() const
{
    bool ret = true;
    for (auto inputCfg : aclApiConfig_) {
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_TRAINING_TRACE:
                ret = ParamValidation::instance()->IsValidSwitch(cfgValue);
                break;
            case INPUT_CFG_COM_SYS_DEVICES:
                ret = ParamValidation::instance()->MsprofCheckSysDeviceValid(cfgValue);
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
    uint64_t dataTypeConfig = apiCfg->dataTypeConfig;
    ProfAicoreMetrics aicMetrics = apiCfg->aicoreMetrics;
    if (dataTypeConfig & PROF_TASK_TIME_MASK) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TASK_TIME);
    }
    // training trace
    if (dataTypeConfig & PROF_KEYPOINT_TRACE_MASK) {
        paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TRAINING_TRACE);
    }
    ProfMetricsCfgToContainer(aicMetrics, dataTypeConfig, argsArr);
    if (dataTypeConfig & PROF_L2CACHE_MASK) {
        paramContainer_[INPUT_CFG_COM_L2] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_L2);
    }
    if (dataTypeConfig & PROF_ACL_API) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_ASCENDCL);
    }
    if (dataTypeConfig & PROF_AICPU_TRACE) {
        paramContainer_[INPUT_CFG_COM_AICPU] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AICPU);
    }
    if (dataTypeConfig & PROF_RUNTIME_API) {
        paramContainer_[INPUT_CFG_COM_RUNTIME_API] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_RUNTIME_API);
    }
    if (dataTypeConfig & PROF_HCCL_TRACE) {
        paramContainer_[INPUT_CFG_COM_HCCL] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_HCCL);
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
    return;
}

void ParamsAdapterAclApi::ProfSystemCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    uint64_t dataTypeConfig = apiCfg->dataTypeConfig;
    ProfAicoreMetrics aicMetrics = apiCfg->aicoreMetrics;
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    if (!argsArr[ACL_PROF_SYS_CPU_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_CPU] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_CPU);
        paramContainer_[INPUT_CFG_COM_SYS_CPU_FREQ] = argsArr[ACL_PROF_SYS_CPU_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_CPU_FREQ);
    }
    ProfSystemHardwareMemCfgToContainer(argsArr);
    if (!argsArr[ACL_PROF_SYS_IO_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_IO] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_IO);
        paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ] = argsArr[ACL_PROF_SYS_IO_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_IO_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_INTERCONNECTION);
        paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ);
    }
    if (!argsArr[ACL_PROF_DVPP_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_DVPP] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_DVPP);
        paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = argsArr[ACL_PROF_DVPP_FREQ];
        setConfig_.insert(INPUT_CFG_COM_DVPP_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_USAGE_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_USAGE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_USAGE);
        paramContainer_[INPUT_CFG_COM_SYS_USAGE_FREQ] = argsArr[ACL_PROF_SYS_USAGE_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_USAGE_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_PID_USAGE_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_PID_USAGE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_PID_USAGE);
        paramContainer_[INPUT_CFG_COM_SYS_PID_USAGE_FREQ] = argsArr[ACL_PROF_SYS_PID_USAGE_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_PID_USAGE_FREQ);
    }
    if (!argsArr[ACL_PROF_HOST_SYS].empty()) {
        paramContainer_[INPUT_HOST_SYS_USAGE] = argsArr[ACL_PROF_HOST_SYS];
        setConfig_.insert(INPUT_HOST_SYS_USAGE);
    }
}

void ParamsAdapterAclApi::ProfSystemHardwareMemCfgToContainer(std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    if (!argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_HARDWARE_MEM);
        paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ);
    }
    if (!argsArr[ACL_PROF_LLC_MODE].empty() && !argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_LLC_MODE] = argsArr[ACL_PROF_LLC_MODE];
        setConfig_.insert(INPUT_CFG_COM_LLC_MODE);
    }
    return;
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