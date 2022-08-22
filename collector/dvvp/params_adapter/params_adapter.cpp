/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#include "params_adapter.h"

#include <map>
#include "param_validation.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "platform/platform.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

int ParamsAdapter::CheckListInit()
{
    // get platform info
    platformType_ = ConfigManager::instance()->GetPlatformType();
    if (platformType_ < PlatformType::MINI_TYPE || platformType_ >= PlatformType::END_TYPE) {
        return PROFILING_FAILED;
    }
    switch(platformType_) {
        case PlatformType::MINI_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
                INPUT_CFG_COM_L2, INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ,
                INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER,
                INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::CLOUD_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
                INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::MDC_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION,
                INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_AICPU, INPUT_CFG_PYTHON_PATH,
                INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY, INPUT_CFG_EXPORT,
                INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
                INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::LHISI_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
                INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
                INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
                INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_DVPP, INPUT_CFG_COM_DVPP_FREQ,
                INPUT_CFG_COM_SYS_CPU, INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_L2,
                INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE,
                INPUT_CFG_QUERY, INPUT_CFG_EXPORT, INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID,
                INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::DC_TYPE:
            std::vector<InputCfg>({
                INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
                INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
                INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
            }).swap(blackSwitch_);
            break;
        case PlatformType::CHIP_V4_1_0:
            std::vector<InputCfg>({}).swap(blackSwitch_);
            break;
        default:
            return PROFILING_FAILED;
    }
    // init common config list
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT, INPUT_CFG_COM_STORAGE_LIMIT, INPUT_CFG_COM_MSPROFTX, INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_AIC_METRICS, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_ASCENDCL, INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_HCCL, INPUT_CFG_COM_L2, INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_USAGE_FREQ,
        INPUT_CFG_COM_SYS_PID_USAGE_FREQ, INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_DVPP_FREQ, INPUT_HOST_SYS_USAGE
    }).swap(commonConfig_);
    return PROFILING_SUCCESS;
}

bool ParamsAdapter::BlackSwitchCheck(InputCfg inputCfg) const
{
    if (inputCfg < INPUT_CFG_MSPROF_APPLICATION || inputCfg >= INPUT_CFG_MAX) {
        return false;
    }
    if (std::find(blackSwitch_.begin(), blackSwitch_.end(), inputCfg) != blackSwitch_.end()) {
        return false;
    }
    return true;
}

PlatformType ParamsAdapter::GetPlatform() const
{
    return platformType_;
}

int ParamsAdapter::ComCfgCheck(EnableType enableType, std::array<std::string, INPUT_CFG_MAX> paramContainer,
    std::vector<InputCfg> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : commonConfig_) {
        std::string cfgValue = paramContainer[inputCfg];
        switch (inputCfg) {
            // output check
            case INPUT_CFG_COM_OUTPUT:
                ret = CheckOutputValid(cfgValue);
                break;
            // storage-limit check
            case INPUT_CFG_COM_STORAGE_LIMIT:
                ret = CheckStorageLimitValid(cfgValue);
                break;
            // switch check
            case INPUT_CFG_COM_MSPROFTX:
            case INPUT_CFG_COM_TASK_TIME:
            case INPUT_CFG_COM_ASCENDCL:
            case INPUT_CFG_COM_RUNTIME_API:
            case INPUT_CFG_COM_HCCL:
            case INPUT_CFG_COM_L2:
            case INPUT_CFG_COM_AICPU:
                ret = CheckSwitchValid(cfgValue);
                break;
            // aic aiv metrics check
            case INPUT_CFG_COM_AIC_METRICS:
            case INPUT_CFG_COM_AIV_METRICS:
                ret = CheckAiMetricsValid(cfgValue);
                break;
            // frequency check
            case INPUT_CFG_COM_SYS_USAGE_FREQ:
            case INPUT_CFG_COM_SYS_PID_USAGE_FREQ:
            case INPUT_CFG_COM_SYS_CPU_FREQ:
            case INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ:
            case INPUT_CFG_COM_SYS_IO_FREQ:
            case INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ:
            case INPUT_CFG_COM_DVPP_FREQ:
                ret = CheckFreqValid(cfgValue, inputCfg);
                break;
            // llc-mode(TODO)
            case INPUT_CFG_COM_LLC_MODE:
                ret = CheckLlcModeValid(cfgValue);
                break;
            // cpu|mem
            case INPUT_HOST_SYS_USAGE:
                ret = CheckHostSysUsageValid(cfgValue);
                break;
            default:
                ret = PROFILING_FAILED;
        }
        if (ret != PROFILING_SUCCESS) {
            cfgList.push_back(inputCfg);
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}


int ParamsAdapter::CheckOutputValid(const std::string &outputParam) const
{
    if (!ParamValidation::instance()->CheckOutputIsValid(outputParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckStorageLimitValid(const std::string &storageLimitParam) const
{
    if (!ParamValidation::instance()->CheckStorageLimitIsValid(storageLimitParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckSwitchValid(const std::string &switchParam) const
{
    if (!ParamValidation::instance()->IsValidSwitch(switchParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckAiMetricsValid(const std::string &aiMetrics) const
{
    if (!ParamValidation::instance()->CheckProfilingAicoreMetricsIsValid(aiMetrics)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckFreqValid(const std::string &freq, const InputCfg freqOpt) const
{
    std::map<InputCfg, std::vector<int>> freqRangeMap = {
        {INPUT_CFG_COM_AIC_FREQ, {1, 100}},
        {INPUT_CFG_COM_AIV_FREQ, {1, 100}},
        {INPUT_CFG_COM_SYS_USAGE_FREQ, {1, 10}},
        {INPUT_CFG_COM_SYS_CPU_FREQ, {1, 50}},
        {INPUT_CFG_COM_SYS_PID_USAGE_FREQ, {1, 10}},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, {1, 1000}},
        {INPUT_CFG_COM_SYS_IO_FREQ, {1, 100}},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, {1, 50}},
        {INPUT_CFG_COM_DVPP_FREQ, {1, 100}},
        {INPUT_CFG_COM_BIU_FREQ, {300, 30000}},
    };
    std::vector<int> checkFreqRange = freqRangeMap[freqOpt];
    if (!ParamValidation::instance()->CheckFreqIsValid(freq, checkFreqRange[0], checkFreqRange[1])) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckLlcModeValid(const std::string &LlcMode) const
{
    if (!ParamValidation::instance()->CheckLlcModeIsValid(LlcMode)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckHostSysUsageValid(const std::string &HostSysUsage) const
{
    if (!ParamValidation::instance()->CheckHostSysUsageIsValid(HostSysUsage)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckAppValid(const std::string &appParam) const
{
    if (appParam.empty()) {
        MSPROF_LOGE("Argument --application: expected one argument.");
        return PROFILING_FAILED;
    }
    std::string remainingAppParams;
    size_t index = appParam.find_first_of(" ");
    if (index != std::string::npos) {
        remainingAppParams = appParam.substr(index + 1);
    }
    std::string cmdParam = appParam.substr(0, index);
    if (!Utils::IsAppName(cmdParam)) {
        if (cmdParam.find("/") != std::string::npos) {
            std::string absolutePathCmd = Utils::RelativePathToAbsolutePath(cmdParam);
            if (Utils::CanonicalizePath(absolutePathCmd).empty()) {
                MSPROF_LOGE("App path(%s) does not exist or permission denied.", absolutePathCmd.c_str());
                return PROFILING_FAILED;
            }
            if (MmAccess2(absolutePathCmd, M_X_OK) != PROFILING_SUCCESS) {
                MSPROF_LOGE("This app(%s) has no executable permission.", absolutePathCmd.c_str());
                return PROFILING_FAILED;
            }
        }
        if (CheckAppScrValid(remainingAppParams) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        return PROFILING_SUCCESS;
    }
    std::string absolutePathApp = Utils::RelativePathToAbsolutePath(cmdParam);
    if (CheckAppParamValid(absolutePathApp) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS; 
}

int ParamsAdapter::CheckAppScrValid(const std::string &appScript) const
{
    if (appScript.empty()) {
        MSPROF_LOGE("No input script to run.");
        return PROFILING_FAILED;
    }
    size_t index = appScript.find_first_of(" ");
    std::string inputScriptParam = appScript.substr(0, index);
    std::string scriptParam = Utils::CanonicalizePath(inputScriptParam);
    if (scriptParam.empty()) {
        MSPROF_LOGE("Invalid input script.");
        return PROFILING_FAILED;
    }
    if (Utils::IsSoftLink(scriptParam)) {
        MSPROF_LOGE("Input script is soft link, not support", scriptParam.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::CheckAppParamValid(const std::string &appParam) const
{
    if (Utils::IsSoftLink(appParam)) {
        MSPROF_LOGE("App path(%s) is soft link, not support!", appParam.c_str());
        return PROFILING_FAILED;
    }
    if (Utils::CanonicalizePath(appParam).empty()) {
        MSPROF_LOGE("App path(%s) does not exist or permission denied!!!", appParam.c_str());
        return PROFILING_FAILED;
    }
    if (MmAccess2(appParam, M_X_OK) != PROFILING_SUCCESS) {
        MSPROF_LOGE("This app(%s) has no executable permission.", appParam.c_str());
        return PROFILING_FAILED;
    }
    if (Utils::IsDir(appParam)) {
        MSPROF_LOGE("Argument --application\%s is a directory, "
            "please enter the executable file path.", appParam.c_str());
        return PROFILING_FAILED;
    }
    std::string appDir;
    std::string appName;
    int ret = Utils::SplitPath(appParam, appDir, appName);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to get app name");
        return PROFILING_FAILED;
    }
    if (!ParamValidation::instance()->CheckAppNameIsValid(appName)) {
        MSPROF_LOGE("Argument --application(%s) is invalid.", appName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckEnvValid(const std::string &envParam) const
{
    if (envParam.empty()) {
        MSPROF_LOGE("Argument --environmet: expected one argument.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckAiModeValid(const std::string &aiModeParam, const InputCfg aiModeTypeOpt) const
{
    std::map<InputCfg, std::string> aiModeTypeList = {
        {INPUT_CFG_COM_AIC_MODE, "aic-mode"},
        {INPUT_CFG_COM_AIV_MODE, "aiv-mode"},
    };
    if (aiModeParam.empty()) {
        MSPROF_LOGE("Argument --%s: expected one argument.", aiModeTypeList[aiModeTypeOpt].c_str());
        return PROFILING_FAILED;
    }
    if (aiModeParam != "task-based" && aiModeParam != "sample-based") {
        MSPROF_LOGE("Argument %s: invalid value: %s."
            "Please input 'task-based' or 'sample-based'.", aiModeTypeList[aiModeTypeOpt].c_str(), aiModeParam.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckSysDeviceValid(const std::string &devListParam) const
{
    if (devListParam.empty()) {
        MSPROF_LOGE("Argument --sys-devices is empty, Please enter a valid --sys-devices value.");
        return PROFILING_FAILED;
    }
    if (devListParam == "all") {
        return PROFILING_SUCCESS;
    }
    std::vector<std::string> devices = Utils::Split(devListParam, false, "", ",");
    for (size_t i = 0; i < devices.size(); ++i) {
        if (!(ParamValidation::instance()->CheckDeviceIdIsValid(devices[i]))) {
            MSPROF_LOGE("Argument --sys-devices: invalid value: %s."
                "Please input data is a id num.", devices[i].c_str());
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckSysPeriodValid(const std::string &sysPeriodParam) const
{
    if (sysPeriodParam.empty()) {
        MSPROF_LOGE("Argument --sys-period is empty, Please enter a valid --sys-period value.");
        return PROFILING_FAILED;
    }
    if (Utils::CheckStringIsNonNegativeIntNum(sysPeriodParam)) {
        auto syspeRet = std::stoi(sysPeriodParam);
        if (!(ParamValidation::instance()->IsValidSleepPeriod(syspeRet))) {
            MSPROF_LOGE("Argument --sys-period: invalid int value: %d."
                "The max period is 30 days.", syspeRet);
            return PROFILING_FAILED;
        } else {
            return PROFILING_SUCCESS;
        }
    } else {
        MSPROF_LOGE("Argument --sys-period: invalid value: %s."
            "Please input an integer value.The max period is 30 days.", sysPeriodParam);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckHostSysValid(const std::string &hostSysParam) const
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    MSPROF_LOGE("Currently, --host-sys can be used only in the Linux environment.");
#endif
    if (Platform::instance()->RunSocSide()) {
        MSPROF_LOGE("Not in host side, --host-sys is not supported");
    }
    if (hostSysParam.empty()) {
        MSPROF_LOGE("Argument --host-sys is empty. Please input in the range of "
            "'cpu|mem|disk|network|osrt'");
        return PROFILING_FAILED;
    }
    std::vector<std::string> hostSysArray = Utils::Split(hostSysParam, false, "", ",");
    for (size_t i = 0; i < hostSysArray.size(); ++i) {
        if (!(ParamValidation::instance()->CheckHostSysOptionsIsValid(hostSysArray[i]))) {
            MSPROF_LOGE("Argument --host-sys: invalid value:%s. Please input in the range of "
                "'cpu|mem|disk|network|osrt'", hostSysArray[i].c_str());
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckHostSysPidValid(const std::string &hostSysPidParam) const
{
   if (!ParamValidation::instance()->CheckHostSysPidValid(hostSysPidParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckPythonPathValid(const std::string &pythonPathParam) const
{
    if (!ParamValidation::instance()->CheckPythonPathIsValid(pythonPathParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckSummaryFormatValid(const std::string &formatParam) const
{
    if (!ParamValidation::instance()->CheckExportSummaryFormatIsValid(formatParam)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::MsprofCheckExportIdValid(const std::string &idParam, const std::string &exportIdType) const
{
    if (!ParamValidation::instance()->CheckExportIdIsValid(idParam, exportIdType)) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void ParamsAdapter::Print(std::array<std::string, INPUT_CFG_MAX> paramContainer)
{
    for(int i = 0; i < INPUT_CFG_MAX; i++) {
        MSPROF_LOGE("[Debug_q] %d : %s", i, paramContainer[i].c_str());
    }
}

} // ParamsAdapter
} // Dvvp
} // Collector