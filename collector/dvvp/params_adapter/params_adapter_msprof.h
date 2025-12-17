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
    MSPROF_MODE_EXPORT,
    MSPROF_MODE_ANALYZE,
    MSPROF_MODE_INVALID
};

class ParamsAdapterMsprof : public ParamsAdapter {
public:
    ParamsAdapterMsprof() : params_(nullptr), msprofMode_(MsprofMode::MSPROF_MODE_INVALID) {}
    ~ParamsAdapterMsprof() override {}
    int GetParamFromInputCfg(const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap,
    SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void CreateCfgMap();
    int CheckAnalysisParams();
    int ParamsCheckMsprof();
    bool ParamsCheckMsprofV1(InputCfg inputCfg, std::string cfgValue) const;
    int ParamsCheckDynProf() const;
    int ParamsCheck();
    void SetDefaultParamsApp();
    void SetDefaultParamsSystem();
    void SetDefaultParamsParse() const;
    void SetDefaultParamsQuery() const;
    void SetDefaultParamsExport() const;
    int CheckMsprofMode(const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap);
    void SetCollectParams();
    void SetAnalysisParams();
    void SplitAppPath(const std::string &appParams);
    int SetModeDefaultParams(MsprofMode modeType);
    int SystemToolsIsExist() const;
    int GenMsprofContainer(const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap);
    int AnalysisParamsAdapt(const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap);
    bool CheckAnalysisConfig(MsprofArgsType arg, const std::string &argsValue) const;

private:
    std::string cmdPath_;
    std::string appParameters_;
    std::string appDir_;
    std::string app_;
    SHARED_PTR_ALIA<ProfileParams> params_;
    // 存储params的中间容器，用来校验之后赋值给params_
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::unordered_map<MsprofArgsType, InputCfg> cfgMap_;
    std::vector<InputCfg> msprofConfig_;
    std::set<InputCfg> setConfig_;
    MsprofMode msprofMode_;
};
} // ParamsAdapter
} // Dvvp
} // Collector
#endif
