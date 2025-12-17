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
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_GEOPT_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_GEOPT_H

#include "params_adapter.h"
#include "message/prof_json_config.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::message;
class ParamsAdapterGeOpt : public ParamsAdapter {
public:
    ParamsAdapterGeOpt() : params_(nullptr) {};
    int GetParamFromInputCfg(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg, SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void InitWholeConfigMap();
    void InitPrintMap();
    int ParamsCheckGeOpt() const;
    bool CheckHostSysGeOptValid(const std::string &cfgStr) const;
    void GenGeOptionsContainer(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg);
    int SetGeOptionsContainerDefaultValue();
    void SetGeOptContainerSysValue();
    int SetOutputDir(std::string &outputDir) const;
    bool CheckInstrAndTaskParamBothSet(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg);

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::map<InputCfg, std::string> geOptionsPrintMap_;
    std::vector<InputCfg> geOptionsWholeConfig_;
    std::vector<InputCfg> geOptConfig_;
    std::set<InputCfg>setConfig_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif