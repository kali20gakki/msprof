/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for msprof
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-05
 */
#include "params_adapter_msprof.h"

#include "config/config.h"
#include "cmd_log.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "prof_params.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Msprofbin;
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::message;
int ParamsAdapterMsprof::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Msprof]params adapter init failed.");
        return PROFILING_FAILED;
    }
    CreateCfgMap();
    std::vector<InputCfg>({
        INPUT_CFG_MSPROF_APPLICATION, INPUT_CFG_MSPROF_ENVIRONMENT, INPUT_CFG_COM_AI_CORE,
        INPUT_CFG_COM_AIC_MODE, INPUT_CFG_COM_AIC_FREQ, INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_MODEL_EXECUTION, INPUT_CFG_COM_SYS_DEVICES,
        INPUT_CFG_COM_SYS_PERIOD, INPUT_CFG_COM_SYS_USAGE, INPUT_CFG_COM_SYS_PID_USAGE,
        INPUT_CFG_COM_SYS_CPU, INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_IO,
        INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_DVPP, INPUT_CFG_COM_POWER,
        INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ, INPUT_CFG_HOST_SYS, INPUT_CFG_HOST_SYS_PID,
        INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY,
        INPUT_CFG_EXPORT, INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID
    }).swap(msprofConfig_);
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::ParamsCheckMsprof()
{
    bool ret = true;
    std::map<int, std::string> switchNameMap = {
        {INPUT_CFG_COM_AI_CORE, "ai-core"}, {INPUT_CFG_COM_AI_VECTOR, "ai-vector-core"},
        {INPUT_CFG_COM_MODEL_EXECUTION, "model-execution"}, {INPUT_CFG_COM_SYS_USAGE, "sys-profiling"},
        {INPUT_CFG_COM_SYS_PID_USAGE, "sys-pid-profiling"}, {INPUT_CFG_COM_SYS_CPU, "sys-cpu-profiling"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM, "sys-hardware-mem"}, {INPUT_CFG_COM_SYS_IO, "sys-io-profiling"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION, "sys-interconnection-profiling"}, {INPUT_CFG_COM_DVPP, "dvpp-profiling"},
        {INPUT_CFG_COM_POWER, "power"}, {INPUT_CFG_COM_BIU, "biu"},
        {INPUT_CFG_PARSE, "parse"}, {INPUT_CFG_QUERY, "query"},
        {INPUT_CFG_EXPORT, "export"}
    };
    for (auto inputCfg : msprofConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_MSPROF_APPLICATION:
                ret = ParamValidation::instance()->MsprofCheckAppValid(paramContainer_[inputCfg]);
                break;
            case INPUT_CFG_MSPROF_ENVIRONMENT:
                ret = ParamValidation::instance()->MsprofCheckEnvValid(cfgValue);
                break;
            case INPUT_CFG_COM_AI_CORE:
            case INPUT_CFG_COM_AI_VECTOR:
            case INPUT_CFG_COM_MODEL_EXECUTION:
            case INPUT_CFG_COM_SYS_USAGE:
            case INPUT_CFG_COM_SYS_PID_USAGE:
            case INPUT_CFG_COM_SYS_CPU:
            case INPUT_CFG_COM_SYS_HARDWARE_MEM:
            case INPUT_CFG_COM_SYS_IO:
            case INPUT_CFG_COM_SYS_INTERCONNECTION:
            case INPUT_CFG_COM_DVPP:
            case INPUT_CFG_COM_POWER:
            case INPUT_CFG_COM_BIU:
            case INPUT_CFG_PARSE:
            case INPUT_CFG_QUERY:
            case INPUT_CFG_EXPORT:
                ret = ParamValidation::instance()->IsValidInputCfgSwitch(switchNameMap[inputCfg], cfgValue);
                break;
            default:
                ret = ParamsCheckMsprofV1(inputCfg, cfgValue);
        }
        if (ret != true) {
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

bool ParamsAdapterMsprof::ParamsCheckMsprofV1(InputCfg inputCfg, std::string cfgValue) const
{
    std::map<InputCfg, std::string> aiModeTypeList = {
        {INPUT_CFG_COM_AIC_MODE, "aic-mode"},
        {INPUT_CFG_COM_AIV_MODE, "aiv-mode"},
    };
    std::map<InputCfg, std::string> exportIdList = {
        {INPUT_CFG_ITERATION_ID, "iteration-id"},
        {INPUT_CFG_MODEL_ID, "model-id"},
    };
    bool ret = true;
    switch (inputCfg) {
        case INPUT_CFG_COM_AIV_MODE:
        case INPUT_CFG_COM_AIC_MODE:
            ret = ParamValidation::instance()->MsprofCheckAiModeValid(cfgValue, aiModeTypeList[inputCfg]);
            break;
        case INPUT_CFG_COM_AIC_FREQ:
        case INPUT_CFG_COM_AIV_FREQ:
        case INPUT_CFG_COM_BIU_FREQ:
            ret = CheckFreqValid(cfgValue, inputCfg);
            break;
        case INPUT_CFG_COM_SYS_DEVICES:
            ret = ParamValidation::instance()->MsprofCheckSysDeviceValid(cfgValue);
            break;
        case INPUT_CFG_COM_SYS_PERIOD:
            ret = ParamValidation::instance()->MsprofCheckSysPeriodValid(cfgValue);
            break;
        case INPUT_CFG_HOST_SYS:
            ret = ParamValidation::instance()->MsprofCheckHostSysValid(cfgValue);
            break;
        case INPUT_CFG_HOST_SYS_PID:
            ret = ParamValidation::instance()->CheckHostSysPidValid(cfgValue);
            break;
        case INPUT_CFG_PYTHON_PATH:
            ret = ParamValidation::instance()->CheckPythonPathIsValid(cfgValue);
            break;
        case INPUT_CFG_SUMMARY_FORMAT:
            ret = ParamValidation::instance()->CheckExportSummaryFormatIsValid(cfgValue);
            break;
        case INPUT_CFG_ITERATION_ID:
        case INPUT_CFG_MODEL_ID:
            ret = ParamValidation::instance()->CheckExportIdIsValid(cfgValue, exportIdList[inputCfg]);
            break;
        default:
            ret = false;
    }
    return ret;
}

void ParamsAdapterMsprof::SetDefaultParamsApp()
{
    if (paramContainer_[INPUT_CFG_COM_OUTPUT].empty()) {
        paramContainer_[INPUT_CFG_COM_OUTPUT] = appDir_;
    }
    paramContainer_[INPUT_CFG_COM_OUTPUT] = Utils::RelativePathToAbsolutePath(paramContainer_[INPUT_CFG_COM_OUTPUT]);
    paramContainer_[INPUT_CFG_COM_OUTPUT] = Utils::CanonicalizePath(paramContainer_[INPUT_CFG_COM_OUTPUT]);
    if (paramContainer_[INPUT_CFG_COM_ASCENDCL].empty()) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_TASK_TIME].empty()) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_AI_CORE].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_AIC_FREQ] = paramContainer_[INPUT_CFG_COM_AIC_FREQ].empty() ?
        std::to_string(THOUSAND / DEFAULT_PROFILING_INTERVAL_10MS) : paramContainer_[INPUT_CFG_COM_AIC_FREQ];
    paramContainer_[INPUT_CFG_COM_AIC_MODE] = (paramContainer_[INPUT_CFG_COM_AIC_MODE].empty()) ?
        PROFILING_MODE_TASK_BASED : paramContainer_[INPUT_CFG_COM_AIC_MODE];
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = (paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIC_METRICS];
    if (paramContainer_[INPUT_CFG_COM_AI_VECTOR].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_AIV_FREQ] = paramContainer_[INPUT_CFG_COM_AIV_FREQ].empty() ?
        std::to_string(THOUSAND / DEFAULT_PROFILING_INTERVAL_10MS) : paramContainer_[INPUT_CFG_COM_AIV_FREQ];
    paramContainer_[INPUT_CFG_COM_AIV_MODE] = (paramContainer_[INPUT_CFG_COM_AIV_MODE].empty()) ?
        PROFILING_MODE_TASK_BASED : paramContainer_[INPUT_CFG_COM_AIV_MODE];
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = (paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
}

int ParamsAdapterMsprof::SetMsprofMode()
{
    if (!paramContainer_[INPUT_CFG_MSPROF_APPLICATION].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_APP;
        SpliteAppPath(paramContainer_[INPUT_CFG_MSPROF_APPLICATION]);
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_COM_SYS_DEVICES].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_SYSTEM;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_HOST_SYS].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_SYSTEM;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_PARSE].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_PARSE;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_QUERY].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_QUERY;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_EXPORT].empty()) {
        msprofMode_ = MsprofMode::MSPROF_MODE_EXPORT;
        return PROFILING_SUCCESS;
    }
    CmdLog::instance()->CmdErrorLog("No valid argument found in --application "
    "--sys-devices --host-sys --parse --query --export");
    return PROFILING_FAILED;
}

void ParamsAdapterMsprof::SetDefaultParamsSystem() const
{
    return;
}

void ParamsAdapterMsprof::SetDefaultParamsParse() const
{
    return;
}

void ParamsAdapterMsprof::SetDefaultParamsQuery() const
{
    return;
}

void ParamsAdapterMsprof::SetDefaultParamsExport() const
{
    return;
}

void ParamsAdapterMsprof::SpliteAppPath(const std::string &appParams)
{
    if (appParams.empty()) {
        return;
    }
    std::string tmpAppParamers;
    size_t index = appParams.find_first_of(" ");
    if (index != std::string::npos) {
        tmpAppParamers = appParams.substr(index + 1);
    }
    cmdPath_ = appParams.substr(0, index);
    if (!Utils::IsAppName(cmdPath_)) {
        // bash xxx.sh args...
        if (cmdPath_.find('/') != std::string::npos) {
            cmdPath_ = Utils::RelativePathToAbsolutePath(cmdPath_);
        }
        index = tmpAppParamers.find_first_of(" ");
        if (index != std::string::npos) {
            appParameters_ = tmpAppParamers.substr(index + 1);
        }
        Utils::SplitPath(tmpAppParamers.substr(0, index), appDir_, app_);
    } else {
        // ./main args...
        cmdPath_ = Utils::RelativePathToAbsolutePath(cmdPath_);
        appParameters_ = tmpAppParamers;
        Utils::SplitPath(cmdPath_, appDir_, app_);
    }
}

void ParamsAdapterMsprof::SetParamsSelf()
{
    params_->app = app_;
    params_->cmdPath = cmdPath_;
    params_->app_parameters = appParameters_;
    params_->app_dir = appDir_;
    params_->app_env = paramContainer_[INPUT_CFG_MSPROF_ENVIRONMENT].empty() ?
        params_->app_env : paramContainer_[INPUT_CFG_MSPROF_ENVIRONMENT];
    params_->pythonPath = paramContainer_[INPUT_CFG_PYTHON_PATH].empty() ?
        params_->pythonPath : paramContainer_[INPUT_CFG_PYTHON_PATH];
    params_->parseSwitch = paramContainer_[INPUT_CFG_PARSE].empty() ?
        params_->parseSwitch : paramContainer_[INPUT_CFG_PARSE];
    params_->querySwitch = paramContainer_[INPUT_CFG_QUERY].empty() ?
        params_->querySwitch : paramContainer_[INPUT_CFG_QUERY];
    params_->exportSwitch = paramContainer_[INPUT_CFG_EXPORT].empty() ?
        params_->exportSwitch : paramContainer_[INPUT_CFG_EXPORT];
    params_->exportSummaryFormat = paramContainer_[INPUT_CFG_SUMMARY_FORMAT].empty() ?
        params_->exportSummaryFormat : paramContainer_[INPUT_CFG_SUMMARY_FORMAT];
    params_->exportIterationId = paramContainer_[INPUT_CFG_ITERATION_ID].empty() ?
        params_->exportIterationId : paramContainer_[INPUT_CFG_ITERATION_ID];
    params_->exportModelId = paramContainer_[INPUT_CFG_MODEL_ID].empty() ?
        params_->exportModelId : paramContainer_[INPUT_CFG_MODEL_ID];
    params_->msprofBinPid = Utils::GetPid();
}

int ParamsAdapterMsprof::SystemToolsIsExist() const
{
    if (setConfig_.find(INPUT_CFG_HOST_SYS) != setConfig_.end()) {
        return ParamValidation::instance()->CheckHostSysToolsExit(paramContainer_[INPUT_CFG_HOST_SYS],
            paramContainer_[INPUT_CFG_COM_OUTPUT], appDir_) ? PROFILING_SUCCESS : PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::GenMsprofContainer(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap)
{
    for (const std::pair<int, std::pair<MsprofCmdInfo, std::string>> kv : argvMap) {
        MsprofArgsType argsType = static_cast<MsprofArgsType>(kv.first);
        InputCfg cfgType = cfgMap_[argsType];
        if (!BlackSwitchCheck(cfgType)) {
            CmdLog::instance()->CmdErrorLog("%s: unrecognized option '%s'", Utils::GetSelfPath().c_str(),
                kv.second.second.c_str());
            ArgsManager::instance()->PrintHelp();
            MSPROF_LOGE("Blacklist check failed, PlatformType:%d", static_cast<int>(GetPlatform()));
            return PROFILING_FAILED;
        }
        if (kv.second.first.args[argsType] != nullptr && kv.second.first.args[argsType][0] == '\0') {
            CmdLog::instance()->CmdErrorLog("Argument %s : expected one argument.", argvMap[argsType].second.c_str());
            return PROFILING_FAILED;
        }
        paramContainer_[cfgType] = std::string(kv.second.first.args[argsType]);
        setConfig_.insert(cfgType);
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::GetParamFromInputCfg(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory of params is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof Init failed.");
        return PROFILING_FAILED;
    }
    ret = GenMsprofContainer(argvMap);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]generate container for msprof failed.");
        return PROFILING_FAILED;
    }
    ret = ParamsCheck();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof input param check failed.");
        return PROFILING_FAILED;
    }
    ret = SetMsprofMode();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]Set msprof mode failed.");
        return PROFILING_FAILED;
    }
    ret = SetModeDefaultParams(msprofMode_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof set default value failed.");
        return PROFILING_FAILED;
    }
    ret = SystemToolsIsExist();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]system tools check failed.");
        return PROFILING_FAILED;
    }
    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("msprof trans to params fail.");
        return PROFILING_FAILED;
    }
    SetParamsSelf();
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::ParamsCheck()
{
    int ret = ParamsCheckMsprof();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("private param check fail.");
        return PROFILING_FAILED;
    }
    ret = ComCfgCheck(paramContainer_, setConfig_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("common param check fail.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::SetModeDefaultParams(MsprofMode modeType)
{
    if (modeType < MsprofMode::MSPROF_MODE_APP || modeType >= MsprofMode::MSPROF_MODE_INVALID) {
        MSPROF_LOGE("msprof mode invalid.");
        return PROFILING_FAILED;
    }
    switch (modeType) {
        case MsprofMode::MSPROF_MODE_APP:
            SetDefaultParamsApp();
            break;
        case MsprofMode::MSPROF_MODE_SYSTEM:
            SetDefaultParamsSystem();
            break;
        case MsprofMode::MSPROF_MODE_PARSE:
            SetDefaultParamsParse();
            break;
        case MsprofMode::MSPROF_MODE_QUERY:
            SetDefaultParamsQuery();
            break;
        case MsprofMode::MSPROF_MODE_EXPORT:
            SetDefaultParamsExport();
            break;
        default:
            return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void ParamsAdapterMsprof::CreateCfgMap()
{
    std::unordered_map<int, InputCfg>({
        {ARGS_OUTPUT, INPUT_CFG_COM_OUTPUT}, {ARGS_STORAGE_LIMIT, INPUT_CFG_COM_STORAGE_LIMIT},
        {ARGS_APPLICATION, INPUT_CFG_MSPROF_APPLICATION}, {ARGS_ENVIRONMENT, INPUT_CFG_MSPROF_ENVIRONMENT},
        {ARGS_AIC_MODE, INPUT_CFG_COM_AIC_MODE}, {ARGS_AIC_METRICE, INPUT_CFG_COM_AIC_METRICS},
        {ARGS_AIV_MODE, INPUT_CFG_COM_AIV_MODE}, {ARGS_AIV_METRICS, INPUT_CFG_COM_AIV_METRICS},
        {ARGS_SYS_DEVICES, INPUT_CFG_COM_SYS_DEVICES}, {ARGS_LLC_PROFILING, INPUT_CFG_COM_LLC_MODE},
        {ARGS_PYTHON_PATH, INPUT_CFG_PYTHON_PATH}, {ARGS_SUMMARY_FORMAT, INPUT_CFG_SUMMARY_FORMAT},
        {ARGS_ASCENDCL, INPUT_CFG_COM_ASCENDCL}, {ARGS_AI_CORE, INPUT_CFG_COM_AI_CORE},
        {ARGS_AIV, INPUT_CFG_COM_AI_VECTOR}, {ARGS_MODEL_EXECUTION, INPUT_CFG_COM_MODEL_EXECUTION},
        {ARGS_RUNTIME_API, INPUT_CFG_COM_RUNTIME_API}, {ARGS_TASK_TIME, INPUT_CFG_COM_TASK_TIME},
        {ARGS_AICPU, INPUT_CFG_COM_AICPU}, {ARGS_MSPROFTX, INPUT_CFG_COM_MSPROFTX},
        {ARGS_CPU_PROFILING, INPUT_CFG_COM_SYS_CPU}, {ARGS_SYS_PROFILING, INPUT_CFG_COM_SYS_USAGE},
        {ARGS_PID_PROFILING, INPUT_CFG_COM_SYS_PID_USAGE}, {ARGS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM},
        {ARGS_IO_PROFILING, INPUT_CFG_COM_SYS_IO}, {ARGS_INTERCONNECTION_PROFILING, INPUT_CFG_COM_SYS_INTERCONNECTION},
        {ARGS_DVPP_PROFILING, INPUT_CFG_COM_DVPP}, {ARGS_POWER, INPUT_CFG_COM_POWER},
        {ARGS_HCCL, INPUT_CFG_COM_HCCL}, {ARGS_BIU, INPUT_CFG_COM_BIU}, {ARGS_L2_PROFILING, INPUT_CFG_COM_L2},
        {ARGS_PARSE, INPUT_CFG_PARSE}, {ARGS_QUERY, INPUT_CFG_QUERY}, {ARGS_EXPORT, INPUT_CFG_EXPORT},
        {ARGS_AIC_FREQ, INPUT_CFG_COM_AIC_FREQ}, {ARGS_AIV_FREQ, INPUT_CFG_COM_AIV_FREQ},
        {ARGS_BIU_FREQ, INPUT_CFG_COM_BIU_FREQ}, {ARGS_SYS_PERIOD, INPUT_CFG_COM_SYS_PERIOD},
        {ARGS_SYS_SAMPLING_FREQ, INPUT_CFG_COM_SYS_USAGE_FREQ},
        {ARGS_PID_SAMPLING_FREQ, INPUT_CFG_COM_SYS_PID_USAGE_FREQ},
        {ARGS_HARDWARE_MEM_SAMPLING_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ},
        {ARGS_IO_SAMPLING_FREQ, INPUT_CFG_COM_SYS_IO_FREQ}, {ARGS_DVPP_FREQ, INPUT_CFG_COM_DVPP_FREQ},
        {ARGS_CPU_SAMPLING_FREQ, INPUT_CFG_COM_SYS_CPU_FREQ},
        {ARGS_INTERCONNECTION_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ},
        {ARGS_EXPORT_ITERATION_ID, INPUT_CFG_ITERATION_ID}, {ARGS_EXPORT_MODEL_ID, INPUT_CFG_MODEL_ID},
        {ARGS_HOST_SYS, INPUT_CFG_HOST_SYS}, {ARGS_HOST_SYS_PID, INPUT_CFG_HOST_SYS_PID},
        {ARGS_HOST_USAGE, INPUT_HOST_SYS_USAGE},
    }).swap(cfgMap_);
    for (auto index : cfgMap_) {
        reCfgMap_.insert({index.second, static_cast<MsprofArgsType>(index.first)});
    }
}
} // ParamsAdapter
} // Dvvp
} // Collector