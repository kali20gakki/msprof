/**
* @file prof_params_adapter.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_HOST_PARAMS_ADAPTER_H
#define ANALYSIS_DVVP_HOST_PARAMS_ADAPTER_H

#include <map>
#include <memory>
#include "singleton/singleton.h"
#include "message/prof_params.h"
#include "proto/profiler.pb.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Host {
namespace Adapter {
class ProfParamsAdapter : public analysis::dvvp::common::singleton::Singleton<ProfParamsAdapter> {
public:
    ProfParamsAdapter();
    ~ProfParamsAdapter();

    int Init();
    int UpdateSampleConfig(SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature,
                           SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    void ProfStartCfgToParamsCfg(const uint64_t dataTypeConfig,
                           SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int HandleSystemTraceConf(const std::string &conf,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int HandleTaskTraceConf(const std::string &conf,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    void SetSystemTraceParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    void GenerateLlcEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    void UpdateHardwareMemParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    void UpdateSysConf(SHARED_PTR_ALIA<analysis::dvvp::proto::ProfilerConf> sysConf,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    void UpdateCpuProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    std::string GenerateCapacityEvents();
    std::string GenerateBandwidthEvents();
    void GenerateLlcDefEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

    void UpdateOpFeature(SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature,
                         SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
private:
    std::map<std::string, std::string> aicoreEvents_;
};
}
}
}
}

#endif
