/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for msprof
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-13
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_MSPROF_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_MSPROF_H

#include <string>
#include <vector>

#include "params_adapter.h"
#include "msprofbin/include/input_parser.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using Analysis::Dvvp::Msprof::MsprofCmdInfo;
using Analysis::Dvvp::Msprof::MsprofArgsType;
enum class MsprofMode {
    MSPROF_MODE_APP,
    MSPROF_MODE_SYSTEM,
    MSPROF_MODE_PARSE,
    MSPROF_MODE_QUERY,
    MSPROF_MODE_EXPORT
};

class ParamsAdapterMsprof : public ParamsAdapter {
public:
    ParamsAdapterMsprof() {}
    ~ParamsAdapterMsprof() override {}
    int GetParamFromInputCfg(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap,
    SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void CreateCfgMap();
    int ParamsCheckMsprof(std::vector<std::pair<InputCfg, std::string>> &cfgList);
    bool ParamsCheckMsprofV1(InputCfg inputCfg, std::string cfgValue) const;
    int ParamsCheck(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap);
    void SetDefaultParamsApp();
    void SetDefaultParamsSystem() const;
    void SetDefaultParamsParse() const;
    void SetDefaultParamsQuery() const;
    void SetDefaultParamsExport() const;
    int SetMsprofMode();
    void SetParamsSelf();
    void SpliteAppPath(const std::string &appParams);
    int SetModeDefaultParams(MsprofMode modeType);

private:
    std::string cmdPath_;
    std::string appParameters_;
    std::string appDir_;
    std::string app_;
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::unordered_map<int, InputCfg> cfgMap_;
    std::unordered_map<int, MsprofArgsType> reCfgMap_;
    std::vector<InputCfg> msprofConfig_;
    std::set<InputCfg>setConfig_;
    MsprofMode msprofMode_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif