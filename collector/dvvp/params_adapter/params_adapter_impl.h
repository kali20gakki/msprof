/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_IMPL_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_IMPL_H

#include <unordered_map>
#include "params_adapter.h"
#include "proto/profiler_ext.pb.h"
#include "message/prof_params.h"
#include "msprofbin/include/input_parser.h"
#include "prof_api_common.h"
#include "acl/acl_prof.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::message::ProfileParams;
using Analysis::Dvvp::Msprof::MsprofArgsType;
using Analysis::Dvvp::Msprof::MsprofCmdInfo;
using analysis::dvvp::proto::ProfAclConfig;
using analysis::dvvp::proto::ProfGeOptionsConfig;


class MsprofParamAdapter : public ParamsAdapter {
public:
    MsprofParamAdapter() {}
    ~MsprofParamAdapter() {}
    int GetParamFromInputCfg(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void CreateCfgMap();
    int ParamsCheckMsprof(std::vector<InputCfg> &cfgList) const;
    void DefaultCfgSet();
    int TransToParams();

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::unordered_map<int, InputCfg> cfgMap_;
    std::vector<InputCfg> msprofConfig_;
    std::set<InputCfg>setConfig_;
};

class AclJsonParamAdapter : public ParamsAdapter {
public:
    AclJsonParamAdapter() {};
    int GetParamFromInputCfg(ProfAclConfig aclCfg, SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclJson(std::vector<InputCfg> &cfgList) const;

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::vector<InputCfg> aclJsonConfig_;
};

class GeOptParamAdapter : public ParamsAdapter {
public:
    GeOptParamAdapter() {};
    int GetParamFromInputCfg(ProfGeOptionsConfig geCfg, SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckGeOpt(std::vector<InputCfg> &cfgList) const;

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::vector<InputCfg> geOptConfig_;
};

class AclApiParamAdapter : public ParamsAdapter {
public:
    AclApiParamAdapter() {};
    int GetParamFromInputCfg(const ProfConfig * apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclApi(std::vector<InputCfg> &cfgList) const;
    void ProfCfgToContainer(const ProfConfig * apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfTaskCfgToContainer(const ProfConfig * apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfSystemCfgToContainer(const ProfConfig * apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::vector<InputCfg> aclApiConfig_;
};

} // ParamsAdapter
} // Dvvp
} // Collector

#endif