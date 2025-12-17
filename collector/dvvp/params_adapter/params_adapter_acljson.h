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
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_ACLJSON_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_ACLJSON_H
#include "params_adapter.h"
#include "message/prof_json_config.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::message;
class ParamsAdapterAclJson : public ParamsAdapter {
public:
    ParamsAdapterAclJson() : params_(nullptr) {};
    int GetParamFromInputCfg(SHARED_PTR_ALIA<ProfAclConfig> aclCfg,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void InitWholeConfigMap();
    void InitPrintMap();
    int ParamsCheckAclJson() const;
    void GenAclJsonContainer(SHARED_PTR_ALIA<ProfAclConfig> aclCfg);
    void SetAclJsonContainerDefaultValue();
    void SetAclJsonContainerSysValue();
    std::string SetOutputDir(const std::string &outputDir) const;
    bool CheckHostSysAclJsonValid(const std::string &cfgStr) const;
    bool CheckInstrAndTaskParamBothSet(SHARED_PTR_ALIA<ProfAclConfig> aclCfg);

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