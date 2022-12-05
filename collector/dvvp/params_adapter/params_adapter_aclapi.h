/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for acl API
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_ACLAPI_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_ACLAPI_H
#include "params_adapter.h"

#include "acl_prof.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

class ParamsAdapterAclApi : public ParamsAdapter {
public:
    ParamsAdapterAclApi() : params_(nullptr) {};
    int GetParamFromInputCfg(const ProfConfig *apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclApi() const;
    void ProfCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfMetricsCfgToContainer(const ProfAicoreMetrics aicMetrics,
        const uint64_t dataTypeConfig, std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfTaskCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfSystemCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    bool CheckHostSysAclApiValid(const std::string &cfgStr) const;
    std::string DevIdToStr(uint32_t devNum, const uint32_t* devList) const;

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::vector<InputCfg> aclApiConfig_;
    std::set<InputCfg> setConfig_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif