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
#include "platform/platform_adapter.h"
#include "ai_drv_dev_api.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {

using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::driver;
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
    return (std::find(blackSwitch_.begin(), blackSwitch_.end(), inputCfg) != blackSwitch_.end()) ? false : true;
}

PlatformType ParamsAdapter::GetPlatform() const
{
    return platformType_;
}

void ParamsAdapter::SpliteAppPath(const std::string &appParams, std::string &cmdPath, std::string &appParameters, std::string &appDir, std::string &app)
{
    if (appParams.empty()) {
        return;
    }
    std::string tmpAppParamers;
    size_t index = appParams.find_first_of(" ");
    if (index != std::string::npos) {
        tmpAppParamers = appParams.substr(index + 1);
    }
    // cmd path
    cmdPath = appParams.substr(0, index);
    cmdPath = Utils::RelativePathToAbsolutePath(cmdPath);
    if (!Utils::IsAppName(cmdPath)) {
        // bash xxx.sh args...
        index = tmpAppParamers.find_first_of(" ");
        if (index != std::string::npos) {
            appParameters = tmpAppParamers.substr(index + 1);
            Utils::SplitPath(tmpAppParamers.substr(0, index), appDir, app);
        }
    } else {
        // ./main args...
        appParameters = tmpAppParamers;
        Utils::SplitPath(cmdPath, appDir, app);
    }
}

int ParamsAdapter::TransToParam(std::array<std::string, INPUT_CFG_MAX> paramContainer, SHARED_PTR_ALIA<ProfileParams> params)
{
    if (params == nullptr) {
        return PROFILING_FAILED;
    }
    auto platformAdapter = PlatformAdapter();
    auto adapter = platformAdapter.Init(params, platformType_);
    // =============================== Common ===============================
    struct CommonParams commonParams;
    commonParams.output = paramContainer[INPUT_CFG_COM_OUTPUT];
    commonParams.storageLimit = paramContainer[INPUT_CFG_COM_STORAGE_LIMIT];
    commonParams.msproftx = paramContainer[INPUT_CFG_COM_MSPROFTX];
    commonParams.hostSysPid = (paramContainer[INPUT_CFG_HOST_SYS_PID].empty()) ? -1 : std::stoi(paramContainer[INPUT_CFG_HOST_SYS_PID]);
    commonParams.device = (paramContainer[INPUT_CFG_COM_SYS_DEVICES].compare("all") == 0) ? DrvGetDevIdsStr() : paramContainer[INPUT_CFG_COM_SYS_DEVICES];
    commonParams.profilingPeriod = (paramContainer[INPUT_CFG_COM_SYS_PERIOD].empty()) ? -1 : std::stoi(paramContainer[INPUT_CFG_COM_SYS_PERIOD]);
    adapter->SetParamsForGlobal(commonParams);
    // =============================== Task =================================
    if (paramContainer[INPUT_CFG_COM_TASK_TIME].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForTaskTime(); // Task Time
    }
    if (paramContainer[INPUT_CFG_COM_TASK_TRACE].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForTaskTrace(); // Task Trace
    }
    if (paramContainer[INPUT_CFG_COM_TRAINING_TRACE].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForTrainingTrace(); // Training Trace
    }
    if (paramContainer[INPUT_CFG_COM_ASCENDCL].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForAscendCL(); // AscendCL
    }
    if (paramContainer[INPUT_CFG_COM_MODEL_EXECUTION].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForGE(); // GE
    }
    if (paramContainer[INPUT_CFG_COM_RUNTIME_API].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForRuntime(); // Runtime
    }
    if (paramContainer[INPUT_CFG_COM_AICPU].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForAICPU(); // AI CPU
    }
    if (paramContainer[INPUT_CFG_COM_HCCL].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForHCCL(); // HCCL
    }
    if (paramContainer[INPUT_CFG_COM_L2].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForL2Cache(); // L2 Cache
    }

    int samplingInterval = 0;
    if (paramContainer[INPUT_CFG_COM_AI_CORE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_AIC_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_AIC_FREQ]));
        adapter->SetParamsForAicMetrics(paramContainer[INPUT_CFG_COM_AIC_MODE], paramContainer[INPUT_CFG_COM_AIC_METRICS], samplingInterval); // AI Core
    }
    if (paramContainer[INPUT_CFG_COM_AI_VECTOR].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_AIV_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_AIV_FREQ]));
        adapter->SetParamsForAivMetrics(paramContainer[INPUT_CFG_COM_AIV_MODE], paramContainer[INPUT_CFG_COM_AIV_METRICS], samplingInterval); // AI Core
    }

    // ================================= Dev-System =============================
    if (paramContainer[INPUT_CFG_COM_SYS_USAGE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_USAGE_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_100MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_USAGE_FREQ]));
        adapter->SetParamsForDeviceSysCpuMemUsage(samplingInterval); // Dev-Sys cpu & mem
    }
    if (paramContainer[INPUT_CFG_COM_SYS_PID_USAGE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_PID_USAGE_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_100MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_PID_USAGE_FREQ]));
        adapter->SetParamsForDeviceAllPidCpuMemUsage(samplingInterval); // Dev-All Pid cpu & mem
    }
    if (paramContainer[INPUT_CFG_COM_SYS_CPU].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_CPU_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_CPU_FREQ]));
        adapter->SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(samplingInterval); // Dev-All Pid cpu & mem
    }
    if (paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]));
        adapter->SetParamsForDeviceHardwareMem(samplingInterval, paramContainer[INPUT_CFG_COM_LLC_MODE]); // Hardware memory
    }
    if (paramContainer[INPUT_CFG_COM_SYS_IO].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_IO_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_IO_FREQ]));
        adapter->SetParamsForDeviceIO(samplingInterval); // Dev-Sys IO
    }
    if (paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]));
        adapter->SetParamsForDeviceIntercommection(samplingInterval); // Dev-Sys IO
    }
    if (paramContainer[INPUT_CFG_COM_DVPP].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_DVPP_FREQ].empty() ? DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_DVPP_FREQ]));
        adapter->SetParamsForDeviceDVPP(samplingInterval); // Dev-Sys Dvpp
    }

    if (paramContainer[INPUT_CFG_COM_POWER].compare(MSVP_PROF_ON) == 0) {
        adapter->SetParamsForDevicePower(); // Dev-Sys Power
    }

    if (paramContainer[INPUT_CFG_COM_BIU].compare(MSVP_PROF_ON) == 0) {
        int biuFreq = paramContainer[INPUT_CFG_COM_BIU_FREQ].empty() ? DEFAULT_PROFILING_BIU_FREQ : std::stoi(paramContainer[INPUT_CFG_COM_BIU_FREQ]);
        adapter->SetParamsForDeviceBIU(biuFreq); // Dev-Sys BIU
    }

    // ============================== Host-Sys Pid============================
    std::vector<std::string> hostSysPidList = Utils::Split(paramContainer[INPUT_CFG_HOST_SYS], false, "", ",");
    for (auto entry : hostSysPidList) {
        if (entry.compare(HOST_SYS_CPU) == 0) {
            adapter->SetParamsForHostPidCpu(); // Host-Sys Pid CPU
        } else if (entry.compare(HOST_SYS_MEM) == 0) {
            adapter->SetParamsForHostPidMem(); // Host-Sys Pid Mem
        } else if (entry.compare(HOST_SYS_DISK) == 0) {
            adapter->SetParamsForHostPidDisk(); // Host-Sys Pid Disk
        } else if (entry.compare(HOST_SYS_OSRT) == 0) {
            adapter->SetParamsForHostPidOSRT(); // Host-Sys Pid osrt
        } else if (entry.compare(HOST_SYS_NETWORK) == 0) {
            adapter->SetParamsForHostNetwork(); // Host-Sys All Pid Network
        }
    }

    // ============================== Host-Sys============================
    std::vector<std::string> hostSysList = Utils::Split(paramContainer[INPUT_HOST_SYS_USAGE], false, "", ",");
    for (auto entry : hostSysList) {
        if (entry.compare(HOST_SYS_CPU) == 0) {
            adapter->SetParamsForHostSysAllPidCpuUsage(); // Host-Sys CPU
        } else if (entry.compare(HOST_SYS_MEM) == 0) {
            adapter->SetParamsForHostSysAllPidMemUsage(); // Host-Sys Mem
        }
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::ComCfgCheck(EnableType enableType, std::array<std::string, INPUT_CFG_MAX> paramContainer,
    std::set<InputCfg> &setArgs,
    std::vector<std::pair<InputCfg, std::string>> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : commonConfig_) {
        if (setArgs.find(inputCfg) == setArgs.end()) {
            continue;
        }
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
            cfgList.push_back(std::pair<InputCfg, std::string>(inputCfg, cfgValue));
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
        {INPUT_CFG_COM_AIC_FREQ, {AIC_FREQ_MIN, AIC_FREQ_MAX}},
        {INPUT_CFG_COM_AIV_FREQ, {AIV_FREQ_MIN, AIV_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_USAGE_FREQ, {SYS_USAGE_FREQ_MIN, SYS_USAGE_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_CPU_FREQ, {SYS_CPU_FREQ_MIN, SYS_CPU_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_PID_USAGE_FREQ, {SYS_PID_USAGE_FREQ_MIN, SYS_PID_USAGE_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, {SYS_HARDWARE_MEM_FREQ_MIN, ACC_PMU_MODE_THRED}},
        {INPUT_CFG_COM_SYS_IO_FREQ, {SYS_IO_FREQ_MIN, SYS_IO_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, {SYS_INTERCONNECTION_FREQ_MIN, ACC_PMU_MODE_THRED}},
        {INPUT_CFG_COM_DVPP_FREQ, {DVPP_FREQ_MIN, DVPP_FREQ_MAX}},
        {INPUT_CFG_COM_BIU_FREQ, {BIU_FREQ_MIN, BIU_FREQ_MAX}},
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

int ParamsAdapter::MsprofCheckAppValid(std::string &appParam) const
{
    if (appParam.empty()) {
        MSPROF_LOGE("Argument --application: expected one argument.");
        return PROFILING_FAILED;
    }
    std::string resultAppParam;
    std::vector<std::string> AppParamsList = Utils::Split(appParam, false, "", " ");
    std::string cmdParam = AppParamsList[0];
    if (!Utils::IsAppName(cmdParam)) {
        if (cmdParam.find("/") != std::string::npos) {
            std::string absolutePathCmdParam = Utils::CanonicalizePath(cmdParam);
            if (absolutePathCmdParam.empty()) {
                MSPROF_LOGE("App path(%s) does not exist or permission denied.", absolutePathCmdParam.c_str());
                return PROFILING_FAILED;
            }
            if (MmAccess2(absolutePathCmdParam, M_X_OK) != PROFILING_SUCCESS) {
                MSPROF_LOGE("This app(%s) has no executable permission.", absolutePathCmdParam.c_str());
                return PROFILING_FAILED;
            }
            resultAppParam = absolutePathCmdParam + " ";
        } else {
            resultAppParam = cmdParam + " ";
        }
        if (CheckAppScriptValid(AppParamsList) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        for (size_t i = 1; i < AppParamsList.size(); i++) {
            std::string tmpStr = Utils::CanonicalizePath(AppParamsList[i]);
            if (!tmpStr.empty()) {
                resultAppParam += tmpStr;
            } else {
                resultAppParam += AppParamsList[i];
            }
            if (i < (AppParamsList.size() - 1)) {
                resultAppParam += " ";
            }
        }
        appParam = resultAppParam;
        return PROFILING_SUCCESS;
    }
    std::string absolutePathApp = Utils::RelativePathToAbsolutePath(cmdParam);
    if (CheckAppParamValid(absolutePathApp) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    for (size_t i = 0; i < AppParamsList.size(); i++) {
        std::string tmpStr = Utils::CanonicalizePath(AppParamsList[i]);
        if (!tmpStr.empty()) {
            resultAppParam += tmpStr;
        } else {
            resultAppParam += AppParamsList[i];
        }
        if (i < (AppParamsList.size() - 1)) {
            resultAppParam += " ";
        }
    }
    appParam = resultAppParam;
    return PROFILING_SUCCESS; 
}

int ParamsAdapter::CheckAppScriptValid(const std::vector<std::string> &appParams) const
{
    if (appParams.size() < 2) {
        MSPROF_LOGE("No input script to run.");
        return PROFILING_FAILED;
    }
    std::string appScript = appParams[1];
    std::string scriptParam = Utils::CanonicalizePath(appScript);
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
        MSPROF_LOGI("[Debug_q] %d : %s", i, paramContainer[i].c_str());
    }
}

std::string ParamsAdapter::SetOutputDir(const std::string &outputDir)
{
    std::string result;
    if (outputDir.empty()) {
        MSPROF_LOGI("No output set, use default path");
    } else {
        std::string path = Utils::RelativePathToAbsolutePath(outputDir);
        if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
        }
        result = analysis::dvvp::common::utils::Utils::CanonicalizePath(path);
    }
    if (result.empty() || !analysis::dvvp::common::utils::Utils::IsDirAccessible(result)) {
        MSPROF_LOGI("No output set or is not accessible, use app dir instead");
        result = analysis::dvvp::common::utils::Utils::GetSelfPath();
        size_t pos = result.rfind(analysis::dvvp::common::utils::MSVP_SLASH);
        if (pos != std::string::npos) {
            result = result.substr(0, pos + 1);
        }
    }
    MSPROF_LOGI("Profiling result path: %s", Utils::BaseName(result).c_str());

    return result;
}

} // ParamsAdapter
} // Dvvp
} // Collector