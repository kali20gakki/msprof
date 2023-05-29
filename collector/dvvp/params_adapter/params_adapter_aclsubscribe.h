/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Description: handle params from user's input config for acl Subscribe
 * Author: Huawei Technologies Co., Ltd.
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_ACLSUBSCIRBE_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_ACLSUBSCIRBE_H

#include "params_adapter.h"


namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

class ParamsAdapterAclSubscribe : public ParamsAdapter {
public:
    ParamsAdapterAclSubscribe() : params_(nullptr) {};
    int GetParamFromInputCfg(PROF_SUB_CONF_CONST_PTR profSubscribeConfig,
                             const uint64_t dataTypeConfig,
                             SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void GenAclSubscribeContainer(const ProfAicoreMetrics aicMetrics, const uint64_t dataTypeConfig);

private:
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    SHARED_PTR_ALIA<ProfileParams> params_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif