/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Description: handle params from user's input config for acl Subscribe
 * Author: Huawei Technologies Co., Ltd.
 */
#include "params_adapter_aclsubscribe.h"


namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::message;

int ParamsAdapterAclSubscribe::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Acl subscribe]params adapter init failed.");
        return PROFILING_FAILED;
    }
    return ret;
}

void ParamsAdapterAclSubscribe::GenAclSubscribeContainer(const ProfAicoreMetrics aicMetrics,
                                                         const uint64_t dataTypeConfig)
{
    if (dataTypeConfig & PROF_TASK_TIME_MASK) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME_L1] = MSVP_PROF_ON;
    }

    if (dataTypeConfig & PROF_KEYPOINT_TRACE_MASK) {
        paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = MSVP_PROF_ON;
    }
    
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    // ai_core
    if ((dataTypeConfig & PROF_AICORE_METRICS_MASK) && !metrics.empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIC_METRICS] = metrics;
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
    }
    // aiv
    if ((dataTypeConfig & PROF_AIV_METRICS_MASK) && !metrics.empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIV_METRICS] = metrics;
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
    }
    SetDefaultAivParams(paramContainer_);

    // l2cache
    if (dataTypeConfig & PROF_L2CACHE_MASK) {
        paramContainer_[INPUT_CFG_COM_L2] = MSVP_PROF_ON;
    }
}

int ParamsAdapterAclSubscribe::GetParamFromInputCfg(PROF_SUB_CONF_CONST_PTR profSubscribeConfig,
                                                    const uint64_t dataTypeConfig,
                                                    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("params is nullptr.");
        return PROFILING_FAILED;
    }

    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("profSubscribeConfig is nullptr.");
        return PROFILING_FAILED;
    }

    params_ = params;

    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init Failed");
        return PROFILING_FAILED;
    }

    GenAclSubscribeContainer(profSubscribeConfig->aicoreMetrics, dataTypeConfig);
    
    ret = PlatformAdapterInit(params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("acl subscribe set params fail.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
} // ParamsAdapter
} // Dvvp
} // Collector