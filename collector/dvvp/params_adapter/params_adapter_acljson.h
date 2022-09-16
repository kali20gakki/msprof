/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for acl json
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_ACLJSON_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_ACLJSON_H
#include "params_adapter.h"
#include "proto/profiler_ext.pb.h"
namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::proto::ProfAclConfig;

class ParamsAdapterAclJson : public ParamsAdapter {
public:
    ParamsAdapterAclJson() : params_(nullptr) {};
    int GetParamFromInputCfg(SHARED_PTR_ALIA<ProfAclConfig> aclCfg,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclJson() const;
    void GenAclJsonContainer(SHARED_PTR_ALIA<ProfAclConfig> aclCfg);
    void SetAclJsonContainerDefaultValue();
    std::string SetOutputDir(const std::string &outputDir) const;

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::map<InputCfg, std::string> aclJsonPrintMap_;
    std::vector<InputCfg> aclJsonWholeConfig_;
    std::vector<InputCfg> aclJsonConfig_;
    std::set<InputCfg>setConfig_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif