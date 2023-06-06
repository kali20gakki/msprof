/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for ge option/env
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_GEOPT_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_GEOPT_H

#include "params_adapter.h"
#include "proto/msprofiler_ext.pb.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::proto::ProfGeOptionsConfig;

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