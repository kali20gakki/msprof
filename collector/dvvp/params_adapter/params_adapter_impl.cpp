/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */

#include "params_adapter_impl.h"

#include "errno/error_code.h"
#include "config/config.h"
#include "config_manager.h"
#include "cmd_log.h"
#include "message/prof_params.h"
#include "platform/platform.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace Analysis::Dvvp::Msprof;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::message;
using namespace Collector::Dvvp::Msprofbin;
int MsprofParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
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

int MsprofParamAdapter::ParamsCheckMsprof(std::vector<std::pair<InputCfg, std::string>> &cfgList)
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : msprofConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_MSPROF_APPLICATION:
                ret = MsprofCheckAppValid(paramContainer_[inputCfg]);
                break;
            case INPUT_CFG_MSPROF_ENVIRONMENT:
                ret = MsprofCheckEnvValid(cfgValue);
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
                ret = CheckSwitchValid(cfgValue);
                break;
            case INPUT_CFG_COM_AIV_MODE:
            case INPUT_CFG_COM_AIC_MODE:
                ret = MsprofCheckAiModeValid(cfgValue, inputCfg);
                break;
            case INPUT_CFG_COM_AIC_FREQ:
            case INPUT_CFG_COM_AIV_FREQ:
            case INPUT_CFG_COM_BIU_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            case INPUT_CFG_COM_SYS_DEVICES:
                ret = MsprofCheckSysDeviceValid(cfgValue);
                break;
            case INPUT_CFG_COM_SYS_PERIOD:
                ret = MsprofCheckSysPeriodValid(cfgValue);
                break;
            case INPUT_CFG_HOST_SYS:
                ret = MsprofCheckHostSysValid(cfgValue);
                break;
            case INPUT_CFG_HOST_SYS_PID:
                ret = MsprofCheckHostSysPidValid(cfgValue);
                break;
            case INPUT_CFG_PYTHON_PATH:
                ret = MsprofCheckPythonPathValid(cfgValue);
                break;
            case INPUT_CFG_SUMMARY_FORMAT:
                ret = MsprofCheckSummaryFormatValid(cfgValue);
                break;
            case INPUT_CFG_ITERATION_ID:
            case INPUT_CFG_MODEL_ID:
                ret = MsprofCheckExportIdValid(cfgValue, "");
                break;
            default:
                ret = PROFILING_FAILED;
        }
        if (ret != PROFILING_SUCCESS) {
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}


void MsprofParamAdapter::SetDefaultParamsApp()
{
    if (paramContainer_[INPUT_CFG_COM_OUTPUT].empty()) {
        paramContainer_[INPUT_CFG_COM_OUTPUT] = appDir_;
    }
    if (paramContainer_[INPUT_CFG_COM_ASCENDCL].empty()) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_TASK_TIME].empty()) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_AI_CORE].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
        paramContainer_[INPUT_CFG_COM_AIC_FREQ] = std::to_string(THOUSAND / DEFAULT_PROFILING_INTERVAL_10MS);
    }
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = (paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIC_METRICS];
    if (paramContainer_[INPUT_CFG_COM_AI_VECTOR].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
        paramContainer_[INPUT_CFG_COM_AIV_FREQ] = std::to_string(THOUSAND / DEFAULT_PROFILING_INTERVAL_10MS);
    }
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = (paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
}

int MsprofParamAdapter::GetMsprofMode()
{
    if (!paramContainer_[INPUT_CFG_MSPROF_APPLICATION].empty()) {
        msprofMode_ = MSPROF_MODE_APP;
        SpliteAppPath(paramContainer_[INPUT_CFG_MSPROF_APPLICATION]);
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_COM_SYS_DEVICES].empty()) {
        msprofMode_ = MSPROF_MODE_SYSTEM;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_HOST_SYS].empty()) {
        msprofMode_ = MSPROF_MODE_SYSTEM;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_PARSE].empty()) {
        msprofMode_ = MSPROF_MODE_PARSE;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_QUERY].empty()) {
        msprofMode_ = MSPROF_MODE_QUERY;
        return PROFILING_SUCCESS;
    }
    if (!paramContainer_[INPUT_CFG_EXPORT].empty()) {
        msprofMode_ = MSPROF_MODE_EXPORT;
        return PROFILING_SUCCESS;
    }
    return PROFILING_FAILED;
}

void MsprofParamAdapter::SetDefaultParamsSystem()
{
    return;
}

void MsprofParamAdapter::SetDefaultParamsParse()
{
    return;
}

void MsprofParamAdapter::SetDefaultParamsQuery()
{
    return;
}

void MsprofParamAdapter::SetDefaultParamsExport()
{
    return;
}

void MsprofParamAdapter::SpliteAppPath(const std::string &appParams)
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
    cmdPath_ = Utils::RelativePathToAbsolutePath(cmdPath_);
    if (!Utils::IsAppName(cmdPath_)) {
        // bash xxx.sh args...
        index = tmpAppParamers.find_first_of(" ");
        if (index != std::string::npos) {
            appParameters_ = tmpAppParamers.substr(index + 1);
            Utils::SplitPath(tmpAppParamers.substr(0, index), appDir_, app_);
        }
    } else {
        // ./main args...
        appParameters_ = tmpAppParamers;
        Utils::SplitPath(cmdPath_, appDir_, app_);
    }
}

void MsprofParamAdapter::SetParamsSelf()
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

static void PrintContainer(std::array<std::string, INPUT_CFG_MAX> paramContainer)
{
    for (int i = 0; i < INPUT_CFG_MAX; i++) {
        MSPROF_LOGI("[qqq]%d:%s", i, paramContainer[i].c_str());
    }
}

int MsprofParamAdapter::GetParamFromInputCfg(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap,
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
    for (const std::pair<int, std::pair<MsprofCmdInfo, std::string>> kv : argvMap) {
        MsprofArgsType argsType = static_cast<MsprofArgsType>(kv.first);
        InputCfg cfgType = cfgMap_[argsType];
        if (!BlackSwitchCheck(cfgType)) {
            std::cout << Utils::GetSelfPath() << ": unrecognized option '"
                  << kv.second.second << "'" << std::endl;
            std::cout << "PlatformType:" << static_cast<uint8_t>(GetPlatform()) << std::endl;
            return PROFILING_FAILED;
        }
        if (!kv.second.first.args[argsType]) {
            return PROFILING_FAILED;
        }
        paramContainer_[cfgType] = kv.second.first.args[argsType];
        setConfig_.insert(cfgType);
    }
    std::vector<std::pair<InputCfg, std::string>> errCfgList;
    ret = ParamsCheckMsprof(errCfgList);
    if (ret != PROFILING_SUCCESS && !errCfgList.empty()) {
        for (auto errCfg : errCfgList) {
            MsprofArgsType argsType = reCfgMap_[errCfg.first];
            CmdLog::instance()->CmdErrorLog("Argument %s set invalid.", argvMap[argsType].second.c_str());
        }
        return PROFILING_FAILED;
    }
    errCfgList.clear();
    ret = ComCfgCheck(ENABLE_MSPROF, paramContainer_, setConfig_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        for (auto errCfg : errCfgList) {
            MsprofArgsType argsType = reCfgMap_[errCfg.first];
            CmdLog::instance()->CmdErrorLog("Argument %s set invalid.", argvMap[argsType].second.c_str());
        }
        return PROFILING_FAILED;
    }
    ret = GetMsprofMode();
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Get msprof running mode fail.");
        return PROFILING_FAILED;
    }
    ret = SetModeDefaultParams(msprofMode_);
        CmdLog::instance()->CmdErrorLog("msprof running mode invalid.");
        return PROFILING_FAILED;
    }
    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("[qqq]dataTypeConfig:%lx", params_->dataTypeConfig);
    SetParamsSelf();
    return PROFILING_SUCCESS;
}

int MsprofParamAdapter::SetModeDefaultParams(MsprofMode modeType)
{
    if (modeType < MSPROF_MODE_APP || modeType > MSPROF_MODE_EXPORT) {
        MSPROF_LOGE("msprof mode invalid.");
        return PROFILING_FAILED;
    }
    switch (modeType) {
        case MSPROF_MODE_APP:
            SetDefaultParamsApp();
        case MSPROF_MODE_SYSTEM:
            SetDefaultParamsSystem();
            break;
        case MSPROF_MODE_PARSE:
            SetDefaultParamsParse();
            break;
        case MSPROF_MODE_QUERY:
            SetDefaultParamsQuery();
            break;
        case MSPROF_MODE_EXPORT:
            SetDefaultParamsExport();
            break;
        default:
            return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void MsprofParamAdapter::CreateCfgMap()
        {ARGS_OUTPUT, INPUT_CFG_COM_OUTPUT},
        {ARGS_ENVIRONMENT, INPUT_CFG_MSPROF_ENVIRONMENT},
        {ARGS_AIC_MODE, INPUT_CFG_COM_AIC_MODE},
        {ARGS_AIC_METRICE, INPUT_CFG_COM_AIC_METRICS},
        {ARGS_AIV_MODE, INPUT_CFG_COM_AIV_MODE},
        {ARGS_AIV_METRICS, INPUT_CFG_COM_AIV_METRICS},
        {ARGS_SYS_DEVICES, INPUT_CFG_COM_SYS_DEVICES},
        {ARGS_LLC_PROFILING, INPUT_CFG_COM_LLC_MODE},
        {ARGS_PYTHON_PATH, INPUT_CFG_PYTHON_PATH},
        {ARGS_SUMMARY_FORMAT, INPUT_CFG_SUMMARY_FORMAT},
        {ARGS_ASCENDCL, INPUT_CFG_COM_ASCENDCL},
        {ARGS_AI_CORE, INPUT_CFG_COM_AI_CORE},
        {ARGS_AIV, INPUT_CFG_COM_AI_VECTOR},
        {ARGS_MODEL_EXECUTION, INPUT_CFG_COM_MODEL_EXECUTION},
        {ARGS_RUNTIME_API, INPUT_CFG_COM_RUNTIME_API},
        {ARGS_TASK_TIME, INPUT_CFG_COM_TASK_TIME},
        {ARGS_AICPU, INPUT_CFG_COM_AICPU},
        {ARGS_MSPROFTX, INPUT_CFG_COM_MSPROFTX},
        {ARGS_CPU_PROFILING, INPUT_CFG_COM_SYS_CPU},
        {ARGS_SYS_PROFILING, INPUT_CFG_COM_SYS_USAGE},
        {ARGS_PID_PROFILING, INPUT_CFG_COM_SYS_PID_USAGE},
        {ARGS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM},
        {ARGS_IO_PROFILING, INPUT_CFG_COM_SYS_IO},
        {ARGS_INTERCONNECTION_PROFILING, INPUT_CFG_COM_SYS_INTERCONNECTION},
        {ARGS_DVPP_PROFILING, INPUT_CFG_COM_DVPP},
        {ARGS_POWER, INPUT_CFG_COM_POWER},
        {ARGS_HCCL, INPUT_CFG_COM_HCCL},
        {ARGS_BIU, INPUT_CFG_COM_BIU},
        {ARGS_L2_PROFILING, INPUT_CFG_COM_L2},
        {ARGS_PARSE, INPUT_CFG_PARSE},
        {ARGS_QUERY, INPUT_CFG_QUERY},
        {ARGS_EXPORT, INPUT_CFG_EXPORT},
        {ARGS_AIC_FREQ, INPUT_CFG_COM_AIC_FREQ},
        {ARGS_AIV_FREQ, INPUT_CFG_COM_AIV_FREQ},
        {ARGS_BIU_FREQ, INPUT_CFG_COM_BIU_FREQ},
        {ARGS_SYS_PERIOD, INPUT_CFG_COM_SYS_PERIOD},
        {ARGS_SYS_SAMPLING_FREQ, INPUT_CFG_COM_SYS_USAGE_FREQ},
        {ARGS_PID_SAMPLING_FREQ, INPUT_CFG_COM_SYS_PID_USAGE_FREQ},
        {ARGS_HARDWARE_MEM_SAMPLING_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ},
        {ARGS_IO_SAMPLING_FREQ, INPUT_CFG_COM_SYS_IO_FREQ},
        {ARGS_DVPP_FREQ, INPUT_CFG_COM_DVPP_FREQ},
        {ARGS_CPU_SAMPLING_FREQ, INPUT_CFG_COM_SYS_CPU_FREQ},
        {ARGS_INTERCONNECTION_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ},
        {ARGS_EXPORT_ITERATION_ID, INPUT_CFG_ITERATION_ID},
        {ARGS_EXPORT_MODEL_ID, INPUT_CFG_MODEL_ID},
        {ARGS_HOST_SYS, INPUT_CFG_HOST_SYS},
        {ARGS_HOST_SYS_PID, INPUT_CFG_HOST_SYS_PID},
        {ARGS_HOST_USAGE, INPUT_HOST_SYS_USAGE},
    }).swap(cfgMap_);
    for (auto index : cfgMap_) {
        reCfgMap_.insert({index.second, static_cast<MsprofArgsType>(index.first)});
    }
}
// ================================================= Acl json ==========================================

int AclJsonParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]acl json Init failed.");
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
    }).swap(aclJsonConfig_);
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT,
        INPUT_CFG_COM_STORAGE_LIMIT,
        INPUT_CFG_COM_MSPROFTX,
        INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_AICPU,
        INPUT_CFG_COM_L2,
        INPUT_CFG_COM_HCCL,
        INPUT_CFG_COM_ASCENDCL,
        INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_AIC_METRICS,
        INPUT_CFG_COM_AIV_METRICS,
        INPUT_CFG_COM_BIU,
        INPUT_CFG_COM_BIU_FREQ
    }).swap(aclJsonWholeConfig_);
    std::map<InputCfg, std::string>({
        {INPUT_CFG_COM_OUTPUT, "output"},
        {INPUT_CFG_COM_STORAGE_LIMIT, "storage_limit"},
        {INPUT_CFG_COM_MSPROFTX, "msproftx"},
        {INPUT_CFG_COM_TASK_TIME, "task_time"},
        {INPUT_CFG_COM_AICPU, "aicpu"},
        {INPUT_CFG_COM_L2, "l2"},
        {INPUT_CFG_COM_HCCL, "hccl"},
        {INPUT_CFG_COM_ASCENDCL, "ascendcl"},
        {INPUT_CFG_COM_RUNTIME_API, "runtime_api"},
        {INPUT_CFG_COM_AIC_METRICS, "aic_metrics"},
        {INPUT_CFG_COM_AIV_METRICS, "aiv_metrics"},
        {INPUT_CFG_COM_BIU, "biu"},
        {INPUT_CFG_COM_BIU_FREQ, "biu_freq"},
    }).swap(aclJsonPrintMap_);
    return PROFILING_SUCCESS;
}

int AclJsonParamAdapter::ParamsCheckAclJson(std::vector<std::pair<InputCfg, std::string>> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : aclJsonConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_BIU:
                ret = CheckSwitchValid(cfgValue);
                break;
            case INPUT_CFG_COM_BIU_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            default:
                ret = PROFILING_FAILED;
        }
        if (ret != PROFILING_SUCCESS) {
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int AclJsonParamAdapter::GenAclJsonContainer(SHARED_PTR_ALIA<ProfAclConfig> aclCfg)
{
    std::string aclJsonSwitch = aclCfg->switch_();
    if (aclJsonSwitch.empty() || aclJsonSwitch != MSVP_PROF_ON) {
        MSPROF_LOGW("Profiling switch is off");
        return PROFILING_FAILED;
    }
    paramContainer_[INPUT_CFG_COM_OUTPUT] = aclCfg->output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = aclCfg->storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = aclCfg->msproftx();
    paramContainer_[INPUT_CFG_COM_TASK_TIME] = aclCfg->task_time();
    paramContainer_[INPUT_CFG_COM_AICPU] = aclCfg->aicpu();
    paramContainer_[INPUT_CFG_COM_L2] = aclCfg->l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = aclCfg->hccl();
    paramContainer_[INPUT_CFG_COM_ASCENDCL] = aclCfg->ascendcl();
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = aclCfg->runtime_api();
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = aclCfg->aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = aclCfg->aiv_metrics();
    paramContainer_[INPUT_CFG_COM_BIU] = aclCfg->biu();
    std::string biuFreqParam = std::to_string(aclCfg->biu_freq());
    if (biuFreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_BIU_FREQ] = biuFreqParam;
    }
    for (auto configOpt : aclJsonWholeConfig_) {
        if (!paramContainer_[configOpt].empty()) {
            if (!BlackSwitchCheck(configOpt)) {
                MSPROF_LOGW("Unrecognized option:%s for PlatformType:%d.",
                    aclJsonPrintMap_[configOpt].c_str(), static_cast<uint8_t>(GetPlatform()));
                paramContainer_[configOpt].clear(); // ignore switch
            } else {
                setConfig_.insert(configOpt);
            }
        }
    }
    return PROFILING_SUCCESS;
}

int AclJsonParamAdapter::SetAclJsonContainerDefaultValue()
{
    paramContainer_[INPUT_CFG_COM_OUTPUT] = SetOutputDir(paramContainer_[INPUT_CFG_COM_OUTPUT]);
    if (paramContainer_[INPUT_CFG_COM_ASCENDCL].empty()) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
    }
    if (paramContainer_[INPUT_CFG_COM_RUNTIME_API].empty()) {
        paramContainer_[INPUT_CFG_COM_RUNTIME_API] = MSVP_PROF_ON;
    }
    paramContainer_[INPUT_CFG_COM_MODEL_EXECUTION] = MSVP_PROF_ON;
    if (paramContainer_[INPUT_CFG_COM_TASK_TIME].empty()) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
    }

    paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
    paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty() ?
        PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIC_METRICS];

    if (std::find(setConfig_.begin(), setConfig_.end(), INPUT_CFG_COM_AI_VECTOR) != setConfig_.end()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
        paramContainer_[INPUT_CFG_COM_AIV_METRICS] = paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty() ?
            PIPE_UTILIZATION : paramContainer_[INPUT_CFG_COM_AIV_METRICS];
    }
    
    return PROFILING_SUCCESS;
}

std::string AclJsonParamAdapter::SetOutputDir(const std::string &outputDir)
{
    std::string result;
    if (outputDir.empty()) {
        MSPROF_LOGI("No output set, use default path");
    } else {
        std::string path = Utils::RelativePathToAbsolutePath(outputDir);
        if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
        }
        result = Utils::CanonicalizePath(path);
    }
    if (result.empty() || !Utils::IsDirAccessible(result)) {
        MSPROF_LOGI("No output set or is not accessible, use app dir instead");
        result = Utils::GetSelfPath();
        size_t pos = result.rfind(MSVP_SLASH);
        if (pos != std::string::npos) {
            result = result.substr(0, pos + 1);
        }
    }
    MSPROF_LOGI("Profiling result path: %s", Utils::BaseName(result).c_str());

    return result;
}

int AclJsonParamAdapter::GetParamFromInputCfg(SHARED_PTR_ALIA<ProfAclConfig> aclCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory for params is empty.");
        return PROFILING_FAILED;
    }
    if (!aclCfg) {
        MSPROF_LOGE("memory for acljson config is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]acljson Init failed.");
        return PROFILING_FAILED;
    }
    
    ret = GenAclJsonContainer(aclCfg);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    std::vector<std::pair<InputCfg, std::string>> errCfgList;
    ret = ParamsCheckAclJson(errCfgList);
    if (ret == PROFILING_FAILED && !errCfgList.empty()) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                aclJsonPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
        return PROFILING_FAILED;
    }

    errCfgList.clear();
    ret = ComCfgCheck(ENABLE_ACL_JSON, paramContainer_, setConfig_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                aclJsonPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
        return PROFILING_FAILED;
    }

    ret = SetAclJsonContainerDefaultValue();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

// ============================================ ge opt ======================================
int GeOptParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_TASK_TRACE, INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
    }).swap(geOptConfig_);
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT,
        INPUT_CFG_COM_STORAGE_LIMIT,
        INPUT_CFG_COM_MSPROFTX,
        INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_TASK_TRACE,
        INPUT_CFG_COM_TRAINING_TRACE,
        INPUT_CFG_COM_AICPU,
        INPUT_CFG_COM_L2,
        INPUT_CFG_COM_HCCL,
        INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_AIC_METRICS,
        INPUT_CFG_COM_AIV_METRICS,
        INPUT_CFG_COM_BIU,
        INPUT_CFG_COM_BIU_FREQ
    }).swap(geOptionsWholeConfig_);
    std::map<InputCfg, std::string>({
        {INPUT_CFG_COM_OUTPUT, "output"},
        {INPUT_CFG_COM_STORAGE_LIMIT, "storage_limit"},
        {INPUT_CFG_COM_MSPROFTX, "msproftx"},
        {INPUT_CFG_COM_TASK_TIME, "task_time"},
        {INPUT_CFG_COM_TASK_TRACE, "task_trace"},
        {INPUT_CFG_COM_TRAINING_TRACE, "training_trace"},
        {INPUT_CFG_COM_AICPU, "aicpu"},
        {INPUT_CFG_COM_L2, "l2"},
        {INPUT_CFG_COM_HCCL, "hccl"},
        {INPUT_CFG_COM_RUNTIME_API, "runtime_api"},
        {INPUT_CFG_COM_AIC_METRICS, "aic_metrics"},
        {INPUT_CFG_COM_AIV_METRICS, "aiv_metrics"},
        {INPUT_CFG_COM_BIU, "biu"},
        {INPUT_CFG_COM_BIU_FREQ, "biu_freq"},
    }).swap(geOptionsPrintMap_);
}

int GeOptParamAdapter::ParamsCheckGeOpt(std::vector<std::pair<InputCfg, std::string>> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : geOptConfig_) {
        if (setConfig_.find(inputCfg) == setConfig_.end()) {
            continue;
        }
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_TASK_TRACE:
            case INPUT_CFG_COM_TRAINING_TRACE:
            case INPUT_CFG_COM_BIU:
                ret = CheckSwitchValid(cfgValue);
                break;
            case INPUT_CFG_COM_BIU_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            default:
                ret = PROFILING_FAILED;
        }
        if (ret != PROFILING_SUCCESS) {
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int GeOptParamAdapter::GenGeOptionsContainer(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg)
{
    paramContainer_[INPUT_CFG_COM_OUTPUT] = geCfg->output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = geCfg->storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = geCfg->msproftx();
    paramContainer_[INPUT_CFG_COM_TASK_TIME] = geCfg->task_time();
    paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = geCfg->training_trace();
    paramContainer_[INPUT_CFG_COM_TASK_TRACE] = geCfg->task_trace();
    paramContainer_[INPUT_CFG_COM_AICPU] = geCfg->aicpu();
    paramContainer_[INPUT_CFG_COM_L2] = geCfg->l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = geCfg->hccl();
    paramContainer_[INPUT_CFG_COM_ASCENDCL] = "off";
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = geCfg->runtime_api();
    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = geCfg->aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = geCfg->aiv_metrics();
    paramContainer_[INPUT_CFG_COM_POWER] = geCfg->power();
    paramContainer_[INPUT_CFG_COM_BIU] = geCfg->biu();
    std::string biuFreqParam = std::to_string(geCfg->biu_freq());
    if (biuFreqParam.compare("0") != 0) {
        paramContainer_[INPUT_CFG_COM_BIU_FREQ] = biuFreqParam;
    }
    for (auto configOpt : geOptionsWholeConfig_) {
        if (!paramContainer_[configOpt].empty()) {
            if (!BlackSwitchCheck(configOpt)) {
                MSPROF_LOGW("Unrecognized option:%s for PlatformType:%d",
                    geOptionsPrintMap_[configOpt].c_str(), static_cast<uint8_t>(GetPlatform()));
                paramContainer_[configOpt].clear(); // ignore switch
            } else {
                setConfig_.insert(configOpt);
            }
        }
    }
    return PROFILING_SUCCESS;
}

int GeOptParamAdapter::SetGeOptionsContainerDefaultValue()
{
    if (!Platform::instance()->PlatformIsHelperHostSide()) {
        int ret = SetOutputDir(paramContainer_[INPUT_CFG_COM_OUTPUT]);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    } else {
        paramContainer_[INPUT_CFG_COM_OUTPUT].clear();
    }
    if (!paramContainer_[INPUT_CFG_COM_AIC_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
    }
    if (!paramContainer_[INPUT_CFG_COM_AIV_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
    }
    return PROFILING_SUCCESS;
}

int GeOptParamAdapter::SetOutputDir(std::string &outputDir)
{
    std::string result;
    if (outputDir.empty()) {
        MSPROF_LOGE("Result path is empty");
        return PROFILING_FAILED;
    }
    std::string path = Utils::RelativePathToAbsolutePath(outputDir);
    if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
    }
    result = analysis::dvvp::common::utils::Utils::CanonicalizePath(path);
    if (result.empty() || !analysis::dvvp::common::utils::Utils::IsDirAccessible(result)) {
        MSPROF_LOGE("Result path is not accessible or not exist, result path: %s", Utils::BaseName(outputDir).c_str());
        std::string errReason = "result path is not accessible or not exist";
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"output", outputDir, errReason}));
        return PROFILING_FAILED;
    }
    outputDir = result;
    return PROFILING_SUCCESS;
}

int GeOptParamAdapter::GetParamFromInputCfg(SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (!params) {
        MSPROF_LOGE("memory for params is empty.");
        return PROFILING_FAILED;
    }
    if (!geCfg) {
        MSPROF_LOGE("memory for ge options config is empty.");
        return PROFILING_FAILED;
    }
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetParamFromInputCfg]ge options Init failed.");
        return PROFILING_FAILED;
    }

    ret = GenGeOptionsContainer(geCfg);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    std::vector<std::pair<InputCfg, std::string>> errCfgList;
    ret = ParamsCheckGeOpt(errCfgList);
    if (ret == PROFILING_FAILED && !errCfgList.empty()) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                geOptionsPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
        return PROFILING_FAILED;
    }

    errCfgList.clear();
    ret = ComCfgCheck(ENABLE_GE_OPTION, paramContainer_, setConfig_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGE("Argument --%s:%s set invalid.",
                geOptionsPrintMap_[errCfg.first].c_str(), paramContainer_[errCfg.first].c_str());
        }
        return PROFILING_FAILED;
    }

    ret = SetGeOptionsContainerDefaultValue();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

// ============================================ Acl api ==================================
int AclApiParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_SYS_DEVICES
    }).swap(aclApiConfig_);
}

int AclApiParamAdapter::ParamsCheckAclApi(std::vector<std::pair<InputCfg, std::string>> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : aclApiConfig_) {
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_COM_TRAINING_TRACE:
                ret = CheckSwitchValid(cfgValue);
                break;
            case INPUT_CFG_COM_SYS_DEVICES:
                ret = MsprofCheckSysDeviceValid(cfgValue);
                break;
            default:
                ret = PROFILING_FAILED;
        }
        if (ret != PROFILING_SUCCESS) {
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

std::string AclApiParamAdapter::devIdToStr(uint32_t devNum, const uint32_t *devList)
{
    std::string devStr;
    bool flag = false;
    for (uint32_t i = 0; i < devNum; i++) {
        if (!flag) {
            flag = true;
            devStr += std::to_string(devList[i]);
            continue;
        }
        devStr += ",";
        devStr += std::to_string(devList[i]);
    }
    return devStr;
}

void AclApiParamAdapter::ProfTaskCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    std::string devStr = devIdToStr(apiCfg->devNums, apiCfg->devIdList);
    if (!devStr.empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_DEVICES] = devStr;
        setConfig_.insert(INPUT_CFG_COM_SYS_DEVICES);
    }

    if (!argsArr[ACL_PROF_STORAGE_LIMIT].empty()) {
        paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = argsArr[ACL_PROF_STORAGE_LIMIT];
        setConfig_.insert(INPUT_CFG_COM_STORAGE_LIMIT);
    }

    uint64_t dataTypeConfig = apiCfg->dataTypeConfig;
    ProfAicoreMetrics aicMetrics = apiCfg->aicoreMetrics;
    if (GetPlatform() == PlatformType::MINI_TYPE) {
        if ((dataTypeConfig & PROF_SCHEDULE_TIMELINE_MASK) ||
            (dataTypeConfig & PROF_TASK_TIME_MASK)) {
            paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
            setConfig_.insert(INPUT_CFG_COM_TASK_TIME);
        }
    } else if (dataTypeConfig & PROF_TASK_TIME_MASK) {
        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TASK_TIME);
    }
    // training trace
    if (dataTypeConfig & PROF_KEYPOINT_TRACE_MASK) {
        paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TRAINING_TRACE);
    }
    
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    if ((dataTypeConfig & PROF_AICORE_METRICS_MASK) && !metrics.empty()) {
        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AI_CORE);
        paramContainer_[INPUT_CFG_COM_AIC_METRICS] = metrics;
        setConfig_.insert(INPUT_CFG_COM_AIC_METRICS);
        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
        setConfig_.insert(INPUT_CFG_COM_AIC_MODE);
        paramContainer_[INPUT_CFG_COM_TASK_TRACE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TASK_TRACE);
    }
    if (!argsArr[ACL_PROF_AIV_METRICS].empty()) {
        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AI_VECTOR);
        paramContainer_[INPUT_CFG_COM_AIV_METRICS] = argsArr[ACL_PROF_AIV_METRICS];
        setConfig_.insert(INPUT_CFG_COM_AIV_METRICS);
        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
        setConfig_.insert(INPUT_CFG_COM_AIV_MODE);
        paramContainer_[INPUT_CFG_COM_TASK_TRACE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TASK_TRACE);
    }
    if (dataTypeConfig & PROF_L2CACHE_MASK) {
        paramContainer_[INPUT_CFG_COM_L2] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_L2);
        paramContainer_[INPUT_CFG_COM_TASK_TRACE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_TASK_TRACE);
    }
    if (dataTypeConfig & PROF_ACL_API) {
        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_ASCENDCL);
    }
    if (dataTypeConfig & PROF_AICPU_TRACE) {
        paramContainer_[INPUT_CFG_COM_AICPU] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_AICPU);
    }
    if (dataTypeConfig & PROF_RUNTIME_API) {
        paramContainer_[INPUT_CFG_COM_RUNTIME_API] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_RUNTIME_API);
    }
    if (dataTypeConfig & PROF_HCCL_TRACE) {
        paramContainer_[INPUT_CFG_COM_HCCL] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_HCCL);
    }
}

void AclApiParamAdapter::ProfSystemCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    uint64_t dataTypeConfig = apiCfg->dataTypeConfig;
    ProfAicoreMetrics aicMetrics = apiCfg->aicoreMetrics;
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    if (!argsArr[ACL_PROF_SYS_CPU_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_CPU] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_CPU);
        paramContainer_[INPUT_CFG_COM_SYS_CPU_FREQ] = argsArr[ACL_PROF_SYS_CPU_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_CPU_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_HARDWARE_MEM);
        paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ);
    }
    if (!argsArr[ACL_PROF_LLC_MODE].empty() && !argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_LLC_MODE] = argsArr[ACL_PROF_LLC_MODE];
        setConfig_.insert(INPUT_CFG_COM_LLC_MODE);
    }
    if (!argsArr[ACL_PROF_SYS_IO_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_IO] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_IO);
        paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ] = argsArr[ACL_PROF_SYS_IO_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_IO_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_INTERCONNECTION);
        paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ);
    }
    if (!argsArr[ACL_PROF_DVPP_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_DVPP] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_DVPP);
        paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = argsArr[ACL_PROF_DVPP_FREQ];
        setConfig_.insert(INPUT_CFG_COM_DVPP_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_USAGE_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_USAGE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_USAGE);
        paramContainer_[INPUT_CFG_COM_SYS_USAGE_FREQ] = argsArr[ACL_PROF_SYS_USAGE_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_USAGE_FREQ);
    }
    if (!argsArr[ACL_PROF_SYS_PID_USAGE_FREQ].empty()) {
        paramContainer_[INPUT_CFG_COM_SYS_PID_USAGE] = MSVP_PROF_ON;
        setConfig_.insert(INPUT_CFG_COM_SYS_PID_USAGE);
        paramContainer_[INPUT_CFG_COM_SYS_PID_USAGE_FREQ] = argsArr[ACL_PROF_SYS_PID_USAGE_FREQ];
        setConfig_.insert(INPUT_CFG_COM_SYS_PID_USAGE_FREQ);
    }
    if (!argsArr[ACL_PROF_HOST_SYS].empty()) {
        paramContainer_[INPUT_HOST_SYS_USAGE] = argsArr[ACL_PROF_HOST_SYS];
        setConfig_.insert(INPUT_HOST_SYS_USAGE);
    }
}

void AclApiParamAdapter::ProfCfgToContainer(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr)
{
    ProfTaskCfgToContainer(apiCfg, argsArr);
    ProfSystemCfgToContainer(apiCfg, argsArr);
}

int AclApiParamAdapter::GetParamFromInputCfg(const ProfConfig * apiCfg,
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    int ret = Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init Failed");
        return PROFILING_FAILED;
    }
    if (!params_->result_dir.empty()) {
        paramContainer_[INPUT_CFG_COM_OUTPUT] = params_->result_dir;
        setConfig_.insert(INPUT_CFG_COM_OUTPUT);
    }
    ProfCfgToContainer(apiCfg, argsArr);
    
    // =================================
    std::vector<std::pair<InputCfg, std::string>> errCfgList;
    ret = ParamsCheckAclApi(errCfgList);
    if (ret != PROFILING_SUCCESS && !errCfgList.empty()) {
        for (auto errCfg : errCfgList) {
            MSPROF_LOGW("Warm");
        }
        return PROFILING_FAILED;
    }
    // [5] 公有参数校验（调基类接口）
    errCfgList.clear();
    ret = ComCfgCheck(ENABLE_API, paramContainer_, setConfig_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        // todo 打印errCfgList中的错误
        return PROFILING_FAILED;
    }

    // [6] 参数转换，转成Params（软件栈转uint64_t， 非软件栈保留在Params）
    ret = TransToParam(paramContainer_, params_);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

} // ParamsAdapter
} // Dvvp
} // Collector