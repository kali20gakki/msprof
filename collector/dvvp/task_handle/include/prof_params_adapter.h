/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DVVP_HOST_PARAMS_ADAPTER_H
#define ANALYSIS_DVVP_HOST_PARAMS_ADAPTER_H

#include <map>
#include <memory>
#include "singleton/singleton.h"
#include "message/prof_params.h"
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
    void ProfStartCfgToParamsCfg(const uint64_t dataTypeConfig,
                           SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int HandleSystemTraceConf(const std::string &conf,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    void SetSystemTraceParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    void GenerateLlcEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    void UpdateHardwareMemParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    void UpdateCpuProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams);
    std::string GenerateCapacityEvents();
    std::string GenerateBandwidthEvents();
    void GenerateLlcDefEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
private:
    std::map<std::string, std::string> aicoreEvents_;
};
}
}
}
}

#endif
