/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof bin params adapter
 * Author: ly
 * Create: 2020-12-10
 */

#ifndef ANALYSIS_DVVP_MSPROFBIN_PARAMS_ADAPTER_H
#define ANALYSIS_DVVP_MSPROFBIN_PARAMS_ADAPTER_H
#include "singleton/singleton.h"
#include "message/prof_params.h"
#include "proto/msprofiler.pb.h"
#include "utils/utils.h"
namespace Analysis {
namespace Dvvp {
namespace Msprof {
class ProfParamsAdapter : public analysis::dvvp::common::singleton::Singleton<ProfParamsAdapter> {
public:
    ProfParamsAdapter();
    ~ProfParamsAdapter();
    int Init();
    void GenerateLlcEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int UpdateParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    std::string GenerateCapacityEvents();
    std::string GenerateBandwidthEvents();
    void GenerateLlcDefEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    std::map<std::string, std::string> aicoreEvents_;
};
}
}
}
#endif