/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#ifndef COLLECTOR_DVVP_PARAM_ADAPTER_IMPL_H
#define COLLECTOR_DVVP_PARAM_ADAPTER_IMPL_H

#include "param_adapter.h"
#include "proto/profiler_ext.pb.h"
#include "message/prof_params.h"
#include "input_parser.h"
#include "prof_api_common.h"
#include "acl/acl_prof.h"

namespace Collector {
namespace Dvvp {
namespace ParamAdapter {
using analysis::dvvp::message::ProfileParams;
using Analysis::Dvvp::Msprof::MsprofArgsType;
using Analysis::Dvvp::Msprof::MsprofCmdInfo;
using analysis::dvvp::proto::MsProfStartReq;
using analysis::dvvp::proto::ProfAclConfig;
using analysis::dvvp::proto::ProfGeOptionsConfig;

class MsprofParamAdapter : public ParamAdapter {
public:
    MsprofParamAdapter(SHARED_PTR_ALIA<ProfileParams> params)
        :params_(params)
    {

    };
    SHARED_PTR_ALIA<ProfileParams> GetParamFromInputCfg(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg);
private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::vector<InputCfg>
};

class AclJsonParamAdapter : public ParamAdapter {
public:
    AclJsonParamAdapter(SHARED_PTR_ALIA<ProfileParams> params)
        :params_(params)
    {};
    SHARED_PTR_ALIA<ProfileParams> GetParamFromInputCfg(ProfAclConfig aclCfg);
private:
    SHARED_PTR_ALIA<ProfileParams> params_;
};

class GeOptParamAdapter : public ParamAdapter {
public:
    GeOptParamAdapter(SHARED_PTR_ALIA<ProfileParams> params)
        :params_(params),
         dataTypeConfig_(0)
    {};
    SHARED_PTR_ALIA<ProfileParams> GetParamFromInputCfg(ProfGeOptionsConfig geCfg);
private:
    uint64_t dataTypeConfig_;
    SHARED_PTR_ALIA<ProfileParams> params_;
};

class AclApiParamAdapter : public ParamAdapter {
public:
    AclApiParamAdapter(SHARED_PTR_ALIA<ProfileParams> params)
        :params_(params),
         dataTypeConfig_(0)
    {};
    SHARED_PTR_ALIA<ProfileParams> GetParamFromInputCfg(const ProfConfig * apiCfg);
private:
    uint64_t dataTypeConfig_;
    SHARED_PTR_ALIA<ProfileParams> params_;
};

} // ParamAdapter
} // Dvvp
} // Collector

#endif