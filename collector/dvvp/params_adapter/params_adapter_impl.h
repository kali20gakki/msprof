/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#ifndef COLLECTOR_DVVP_PARAMS_ADAPTER_IMPL_H
#define COLLECTOR_DVVP_PARAMS_ADAPTER_IMPL_H

#include <unordered_map>
#include <map>
#include "params_adapter.h"
#include "proto/profiler_ext.pb.h"
#include "message/prof_params.h"
#include "msprofbin/include/input_parser.h"
#include "prof_api_common.h"
#include "acl/acl_prof.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using analysis::dvvp::message::ProfileParams;
using Analysis::Dvvp::Msprof::MsprofArgsType;
using Analysis::Dvvp::Msprof::MsprofCmdInfo;
using analysis::dvvp::proto::ProfAclConfig;
using analysis::dvvp::proto::ProfGeOptionsConfig;

enum MsprofMode {
    MSPROF_MODE_APP,
    MSPROF_MODE_SYSTEM,
    MSPROF_MODE_PARSE,
    MSPROF_MODE_QUERY,
    MSPROF_MODE_EXPORT
};

class MsprofParamAdapter : public ParamsAdapter {
public:
    MsprofParamAdapter() {}
    ~MsprofParamAdapter() {}
    int GetParamFromInputCfg(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap,
    SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    void CreateCfgMap();
    int ParamsCheckMsprof(std::vector<std::pair<InputCfg, std::string>> &cfgList);
    void SetDefaultParamsApp();
    void SetDefaultParamsSystem();
    void SetDefaultParamsParse();
    void SetDefaultParamsQuery();
    void SetDefaultParamsExport();
    int GetMsprofMode();
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

class AclJsonParamAdapter : public ParamsAdapter {
public:
    AclJsonParamAdapter() {};
    int GetParamFromInputCfg(SHARED_PTR_ALIA<ProfAclConfig> aclCfg,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclJson(std::vector<std::pair<InputCfg, std::string>> &cfgList) const;
    int GenAclJsonContainer(SHARED_PTR_ALIA<ProfAclConfig> aclCfg);
    int SetAclJsonContainerDefaultValue();
    std::string SetOutputDir(const std::string &outputDir);

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::map<InputCfg, std::string> aclJsonPrintMap_;
    std::vector<InputCfg> aclJsonWholeConfig_;
    std::vector<InputCfg> aclJsonConfig_;
    std::set<InputCfg>setConfig_;
};

class GeOptParamAdapter : public ParamsAdapter {
public:
    GeOptParamAdapter() {};
    int GetParamFromInputCfg(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg, SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckGeOpt(std::vector<std::pair<InputCfg, std::string>> &cfgList) const;
    int GenGeOptionsContainer(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg);
    int SetGeOptionsContainerDefaultValue();
    int SetOutputDir(std::string &outputDir);

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::map<InputCfg, std::string> geOptionsPrintMap_;
    std::vector<InputCfg> geOptionsWholeConfig_;
    std::vector<InputCfg> geOptConfig_;
    std::set<InputCfg>setConfig_;
};

class AclApiParamAdapter : public ParamsAdapter {
public:
    AclApiParamAdapter() {};
    int GetParamFromInputCfg(const ProfConfig * apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr,
        SHARED_PTR_ALIA<ProfileParams> params);

private:
    int Init();
    int ParamsCheckAclApi(std::vector<std::pair<InputCfg, std::string>> &cfgList) const;
    void ProfCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfTaskCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    void ProfSystemCfgToContainer(const ProfConfig* apiCfg,
        std::array<std::string, ACL_PROF_ARGS_MAX> argsArr);
    std::string devIdToStr(uint32_t devNum, const uint32_t* devList);

private:
    SHARED_PTR_ALIA<ProfileParams> params_;
    std::array<std::string, INPUT_CFG_MAX> paramContainer_;
    std::vector<InputCfg> aclApiConfig_;
    std::set<InputCfg>setConfig_;
};

} // ParamsAdapter
} // Dvvp
} // Collector

#endif