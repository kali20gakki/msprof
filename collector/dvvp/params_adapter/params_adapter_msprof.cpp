/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config for msprof
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-09-05
 */
#include "params_adapter_msprof.h"

#include "config/config.h"
#include "cmd_log.h"
#include "dyn_prof_client.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "platform/platform.h"
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
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::message;
using namespace Collector::Dvvp::DynProf;
int ParamsAdapterMsprof::Init()
{
    paramContainer_.fill("");
    int ret = Platform::instance()->PlatformInitByDriver();
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Running profiling failed."
            " Please check the driver package is correctly installed.");
        MSPROF_LOGE("Init platform by driver faild.");
        return PROFILING_FAILED;
    }
    ret = CheckListInit();
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
        INPUT_CFG_COM_INSTR_PROFILING, INPUT_CFG_COM_INSTR_PROFILING_FREQ, INPUT_CFG_HOST_SYS, INPUT_CFG_HOST_SYS_PID,
        INPUT_CFG_MSPROF_DYNAMIC, INPUT_CFG_MSPROF_DYNAMIC_PID
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
        {INPUT_CFG_COM_POWER, "power"}, {INPUT_CFG_COM_INSTR_PROFILING, "instr-profiling"},
        {INPUT_CFG_MSPROF_DYNAMIC, "dynamic"},
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
            case INPUT_CFG_COM_INSTR_PROFILING:
            case INPUT_CFG_MSPROF_DYNAMIC:
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
    bool ret = false;
    switch (inputCfg) {
        case INPUT_CFG_COM_AIV_MODE:
            ret = ParamValidation::instance()->MsprofCheckAiModeValid(cfgValue, "aiv-mode");
            break;
        case INPUT_CFG_COM_AIC_MODE:
            ret = ParamValidation::instance()->MsprofCheckAiModeValid(cfgValue, "aic-mode");
            break;
        case INPUT_CFG_COM_AIC_FREQ:
        case INPUT_CFG_COM_AIV_FREQ:
        case INPUT_CFG_COM_INSTR_PROFILING_FREQ:
        case INPUT_CFG_HOST_SYS_USAGE_FREQ:
        case INPUT_CFG_MSPROF_DYNAMIC_PID:
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
        case INPUT_CFG_HOST_SYS_USAGE:
            ret = ParamValidation::instance()->CheckHostSysUsageValid(cfgValue);
            break;
        case INPUT_CFG_HOST_SYS_PID:
            ret = ParamValidation::instance()->CheckHostSysPidValid(cfgValue);
            break;
        default:
            break;
    }
    return ret;
}

int ParamsAdapterMsprof::ParamsCheckDynProf() const
{
    // --dynamic != on, no need check params
    if (paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] != MSPROF_SWITCH_ON) {
        return PROFILING_SUCCESS;
    }
    // --pid and --application is both empty
    if (paramContainer_[INPUT_CFG_MSPROF_DYNAMIC_PID].empty() &&
        paramContainer_[INPUT_CFG_MSPROF_APPLICATION].empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --dynamic=on, but --application/--pid is all empty.");
        MSPROF_LOGE("Argument --dynamic=on, but --application/--pid is all empty.");
        return PROFILING_FAILED;
    }
    // --pid and --application is both non-empty
    if (!paramContainer_[INPUT_CFG_MSPROF_DYNAMIC_PID].empty() &&
        !paramContainer_[INPUT_CFG_MSPROF_APPLICATION].empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --dynamic=on, can not set --application/--pid at the same time.");
        MSPROF_LOGE("Argument --dynamic=on, can not set --application/--pid at the same time.");
        return PROFILING_FAILED;
    }
    // --pid is non-empty, save pid to DynProfMngCli
    if (!paramContainer_[INPUT_CFG_MSPROF_DYNAMIC_PID].empty()) {
        int32_t pid = std::stoi(paramContainer_[INPUT_CFG_MSPROF_DYNAMIC_PID]);
        DynProfMngCli::instance()->SetAppPid(pid);
    }
    DynProfMngCli::instance()->EnableMode();
    return PROFILING_SUCCESS;
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
    paramContainer_[INPUT_CFG_COM_AIC_MODE] = (paramContainer_[INPUT_CFG_COM_AIC_MODE].empty()) ?
        PROFILING_MODE_TASK_BASED : paramContainer_[INPUT_CFG_COM_AIC_MODE];
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = (paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) ?
        SetDefaultAicMetricsType() : paramContainer_[INPUT_CFG_COM_AIC_METRICS];
    if (paramContainer_[INPUT_CFG_COM_AI_VECTOR].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_AIV_MODE] = (paramContainer_[INPUT_CFG_COM_AIV_MODE].empty()) ?
        PROFILING_MODE_TASK_BASED : paramContainer_[INPUT_CFG_COM_AIV_MODE];
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = (paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
    SetDefaultAivParams(paramContainer_);
    SetDefaultLlcMode(paramContainer_);
}

int ParamsAdapterMsprof::CheckMsprofMode(const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap)
{
    std::vector<std::pair<int, MsprofMode>> modeAdjustVec = {
        {ARGS_APPLICATION, MsprofMode::MSPROF_MODE_APP},
        {ARGS_SYS_DEVICES, MsprofMode::MSPROF_MODE_SYSTEM},
        {ARGS_HOST_SYS, MsprofMode::MSPROF_MODE_SYSTEM},
        {ARGS_HOST_SYS_USAGE, MsprofMode::MSPROF_MODE_SYSTEM},
        {ARGS_PARSE, MsprofMode::MSPROF_MODE_PARSE},
        {ARGS_QUERY, MsprofMode::MSPROF_MODE_QUERY},
        {ARGS_EXPORT, MsprofMode::MSPROF_MODE_EXPORT},
        {ARGS_DYNAMIC_PROF, MsprofMode::MSPROF_MODE_APP}
    };

    for (auto adjustArgs : modeAdjustVec) {
        if (argvMap.find(adjustArgs.first) != argvMap.end()) {
            msprofMode_ = adjustArgs.second;
            return PROFILING_SUCCESS;
        }
    }
    CmdLog::instance()->CmdErrorLog("No valid argument found in --dynamic --application "
        "--sys-devices --host-sys --host-sys-usage --parse --query --export");
    return PROFILING_FAILED;
}

void ParamsAdapterMsprof::SetDefaultParamsSystem()
{
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = (paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) ?
        SetDefaultAicMetricsType() : paramContainer_[INPUT_CFG_COM_AIC_METRICS];
    paramContainer_[INPUT_CFG_COM_AIC_MODE] = (paramContainer_[INPUT_CFG_COM_AIC_MODE].empty()) ?
        PROFILING_MODE_SAMPLE_BASED : paramContainer_[INPUT_CFG_COM_AIC_MODE];
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = (paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
    paramContainer_[INPUT_CFG_COM_AIV_MODE] = (paramContainer_[INPUT_CFG_COM_AIV_MODE].empty()) ?
        PROFILING_MODE_SAMPLE_BASED : paramContainer_[INPUT_CFG_COM_AIV_MODE];
    SetDefaultAivParams(paramContainer_);
    SetDefaultLlcMode(paramContainer_);
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
    if (Utils::IsPythonOrBash(cmdPath_)) {
        // bash xxx.sh args...
        if (cmdPath_.find('/') != std::string::npos) {
            cmdPath_ = Utils::RelativePathToAbsolutePath(cmdPath_);
        }
    } else {
        // ./main args...
        cmdPath_ = Utils::RelativePathToAbsolutePath(cmdPath_);
    }
    appParameters_ = tmpAppParamers;
    Utils::SplitPath(cmdPath_, appDir_, app_);
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

int ParamsAdapterMsprof::GenMsprofContainer(
    const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap)
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
            CmdLog::instance()->CmdErrorLog("Argument %s : expected one argument.",
                argvMap.at(argsType).second.c_str());
            return PROFILING_FAILED;
        }
        paramContainer_[cfgType] = std::string(kv.second.first.args[argsType]);
        setConfig_.insert(cfgType);
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapterMsprof::AnalysisParamsAdapt(
    const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap)
{
    paramContainer_.fill("");
    bool ret = false;
    std::set<MsprofArgsType> analysisArgs = {
        ARGS_OUTPUT, ARGS_PYTHON_PATH, ARGS_SUMMARY_FORMAT, ARGS_PARSE, ARGS_QUERY, ARGS_EXPORT,
        ARGS_EXPORT_ITERATION_ID, ARGS_EXPORT_MODEL_ID};
    std::unordered_map<int, InputCfg> argsTransMap = {
        {ARGS_OUTPUT, INPUT_CFG_COM_OUTPUT}, {ARGS_PYTHON_PATH, INPUT_CFG_PYTHON_PATH},
        {ARGS_SUMMARY_FORMAT, INPUT_CFG_SUMMARY_FORMAT}, {ARGS_PARSE, INPUT_CFG_PARSE},
        {ARGS_QUERY, INPUT_CFG_QUERY}, {ARGS_EXPORT, INPUT_CFG_EXPORT},
        {ARGS_EXPORT_ITERATION_ID, INPUT_CFG_ITERATION_ID}, {ARGS_EXPORT_MODEL_ID, INPUT_CFG_MODEL_ID}
    };
    for (auto arg : analysisArgs) {
        if (argvMap.find(arg) == argvMap.end()) {
            continue;
        }
        if (argvMap.at(arg).first.args[arg] != nullptr && argvMap.at(arg).first.args[arg][0] == '\0') {
            CmdLog::instance()->CmdErrorLog("Argument %s : expected one argument.", argvMap.at(arg).second.c_str());
            return PROFILING_FAILED;
        }
        std::string argsValue = std::string(argvMap.at(arg).first.args[arg]);
        if (!CheckAnalysisConfig(arg, argsValue)) {
            return PROFILING_FAILED;
        }
        paramContainer_[argsTransMap[arg]] = argsValue;
    }
    params_->result_dir = paramContainer_[INPUT_CFG_COM_OUTPUT].empty() ? params_->result_dir :
        paramContainer_[INPUT_CFG_COM_OUTPUT];
    SetParamsSelf();
    return PROFILING_SUCCESS;
}

bool ParamsAdapterMsprof::CheckAnalysisConfig(MsprofArgsType arg, const std::string &argsValue) const
{
    bool ret = false;
    switch (arg) {
        case ARGS_OUTPUT:
            ret = ParamValidation::instance()->CheckOutputIsValid(argsValue);
            break;
        case ARGS_PYTHON_PATH:
            ret = ParamValidation::instance()->CheckPythonPathIsValid(argsValue);
            break;
        case ARGS_SUMMARY_FORMAT:
            ret = ParamValidation::instance()->CheckExportSummaryFormatIsValid(argsValue);
            break;
        case ARGS_PARSE:
            ret = ParamValidation::instance()->IsValidInputCfgSwitch("parse", argsValue);
            break;
        case ARGS_QUERY:
            ret = ParamValidation::instance()->IsValidInputCfgSwitch("query", argsValue);
            break;
        case ARGS_EXPORT:
            ret = ParamValidation::instance()->IsValidInputCfgSwitch("export", argsValue);
            break;
        case ARGS_EXPORT_ITERATION_ID:
            ret = ParamValidation::instance()->CheckExportIdIsValid(argsValue, "iteration-id");
            break;
        case ARGS_EXPORT_MODEL_ID:
            ret = ParamValidation::instance()->CheckExportIdIsValid(argsValue, "model-id");
            break;
        default:
            ret = false;
            break;
    }
    return ret;
}

int ParamsAdapterMsprof::GetParamFromInputCfg(
    const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory of params is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    if (CheckMsprofMode(argvMap) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]Check msprof mode failed.");
        return PROFILING_FAILED;
    }
    if (msprofMode_ == MsprofMode::MSPROF_MODE_PARSE || msprofMode_ == MsprofMode::MSPROF_MODE_QUERY ||
        msprofMode_ == MsprofMode::MSPROF_MODE_EXPORT) {
        return AnalysisParamsAdapt(argvMap);
    }
    if (Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof Init failed.");
        return PROFILING_FAILED;
    }
    if (GenMsprofContainer(argvMap) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]generate container for msprof failed.");
        return PROFILING_FAILED;
    }
    if (PlatformAdapterInit(params_) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (ParamsCheck() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof input param check failed.");
        return PROFILING_FAILED;
    }
    if (msprofMode_ == MsprofMode::MSPROF_MODE_APP) {
        SpliteAppPath(paramContainer_[INPUT_CFG_MSPROF_APPLICATION]);
    }
    if (SetModeDefaultParams(msprofMode_) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]msprof set default value failed.");
        return PROFILING_FAILED;
    }
    if (SystemToolsIsExist() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]system tools check failed.");
        return PROFILING_FAILED;
    }
    if (TransToParam(paramContainer_, params_) != PROFILING_SUCCESS) {
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
    ret = ParamsCheckDynProf();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling param check fail.");
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
        {ARGS_HCCL, INPUT_CFG_COM_HCCL}, {ARGS_INSTR_PROFILING, INPUT_CFG_COM_INSTR_PROFILING},
        {ARGS_L2_PROFILING, INPUT_CFG_COM_L2}, {ARGS_PARSE, INPUT_CFG_PARSE}, {ARGS_QUERY, INPUT_CFG_QUERY},
        {ARGS_EXPORT, INPUT_CFG_EXPORT}, {ARGS_AIC_FREQ, INPUT_CFG_COM_AIC_FREQ},
        {ARGS_AIV_FREQ, INPUT_CFG_COM_AIV_FREQ}, {ARGS_INSTR_PROFILING_FREQ, INPUT_CFG_COM_INSTR_PROFILING_FREQ},
        {ARGS_SYS_PERIOD, INPUT_CFG_COM_SYS_PERIOD},
        {ARGS_SYS_SAMPLING_FREQ, INPUT_CFG_COM_SYS_USAGE_FREQ},
        {ARGS_PID_SAMPLING_FREQ, INPUT_CFG_COM_SYS_PID_USAGE_FREQ},
        {ARGS_HARDWARE_MEM_SAMPLING_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ},
        {ARGS_IO_SAMPLING_FREQ, INPUT_CFG_COM_SYS_IO_FREQ}, {ARGS_DVPP_FREQ, INPUT_CFG_COM_DVPP_FREQ},
        {ARGS_CPU_SAMPLING_FREQ, INPUT_CFG_COM_SYS_CPU_FREQ},
        {ARGS_INTERCONNECTION_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ},
        {ARGS_EXPORT_ITERATION_ID, INPUT_CFG_ITERATION_ID}, {ARGS_EXPORT_MODEL_ID, INPUT_CFG_MODEL_ID},
        {ARGS_HOST_SYS, INPUT_CFG_HOST_SYS}, {ARGS_HOST_SYS_PID, INPUT_CFG_HOST_SYS_PID},
        {ARGS_HOST_SYS_USAGE, INPUT_CFG_HOST_SYS_USAGE}, {ARGS_HOST_SYS_USAGE_FREQ, INPUT_CFG_HOST_SYS_USAGE_FREQ},
        {ARGS_DYNAMIC_PROF, INPUT_CFG_MSPROF_DYNAMIC}, {ARGS_DYNAMIC_PROF_PID, INPUT_CFG_MSPROF_DYNAMIC_PID},
        }).swap(cfgMap_);
    for (auto index : cfgMap_) {
        reCfgMap_.insert({index.second, static_cast<MsprofArgsType>(index.first)});
    }
}
} // ParamsAdapter
} // Dvvp
} // Collector
