/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#include "params_adapter.h"

#include <map>
#include "param_validation.h"
#include "errno/error_code.h"
#include "platform/platform.h"
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
ParamsAdapter::~ParamsAdapter()
{
}
int ParamsAdapter::CheckListInit()
{
    platformType_ = ConfigManager::instance()->GetPlatformType();
    if (platformType_ < PlatformType::MINI_TYPE || platformType_ >= PlatformType::END_TYPE) {
        MSPROF_LOGE("get invalid platform type for init.");
        return PROFILING_FAILED;
    }
    switch (platformType_) {
        case PlatformType::MINI_TYPE:
            SetMiniBlackSwitch();
            break;
        case PlatformType::CLOUD_TYPE:
            SetCloudBlackSwitch();
            break;
        case PlatformType::MDC_TYPE:
            SetMdcBlackSwitch();
            break;
        case PlatformType::LHISI_TYPE:
            SetLhisiBlackSwitch();
            break;
        case PlatformType::DC_TYPE:
            SetDcBlackSwitch();
            break;
        case PlatformType::CHIP_V4_1_0:
            SetCloudV2BlackSwitch();
            break;
        case PlatformType::CHIP_V4_2_0:
            SetMiniV2BlackSwitch();
            break;
        default:
            return PROFILING_FAILED;
    }
    SetCommonConfig();
    return PROFILING_SUCCESS;
}

void ParamsAdapter::SetMiniBlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_L2, INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ,
        INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER,
        INPUT_CFG_COM_INSTR_PROFILING, INPUT_CFG_COM_INSTR_PROFILING_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetCloudBlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_INSTR_PROFILING,
        INPUT_CFG_COM_INSTR_PROFILING_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetMdcBlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION,
        INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_AICPU, INPUT_CFG_PYTHON_PATH,
        INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY, INPUT_CFG_EXPORT,
        INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID, INPUT_CFG_COM_POWER, INPUT_CFG_COM_INSTR_PROFILING,
        INPUT_CFG_COM_INSTR_PROFILING_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetLhisiBlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_HARDWARE_MEM, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_DVPP, INPUT_CFG_COM_DVPP_FREQ,
        INPUT_CFG_COM_SYS_CPU, INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_L2,
        INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE,
        INPUT_CFG_QUERY, INPUT_CFG_EXPORT, INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID,
        INPUT_CFG_COM_POWER, INPUT_CFG_COM_INSTR_PROFILING, INPUT_CFG_COM_INSTR_PROFILING_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetDcBlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO, INPUT_CFG_COM_SYS_IO_FREQ,
        INPUT_CFG_COM_POWER, INPUT_CFG_COM_INSTR_PROFILING, INPUT_CFG_COM_INSTR_PROFILING_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetCloudV2BlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIV_METRICS
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetMiniV2BlackSwitch()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_AI_VECTOR, INPUT_CFG_COM_AIV_FREQ, INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_SYS_INTERCONNECTION, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_INSTR_PROFILING,
        INPUT_CFG_COM_INSTR_PROFILING_FREQ, INPUT_CFG_HOST_SYS, INPUT_CFG_HOST_SYS_PID,
        INPUT_CFG_HOST_SYS_USAGE, INPUT_CFG_HOST_SYS_USAGE_FREQ
        }).swap(blackSwitch_);
    return;
}

void ParamsAdapter::SetCommonConfig()
{
    std::vector<InputCfg>({
        INPUT_CFG_COM_OUTPUT, INPUT_CFG_COM_STORAGE_LIMIT, INPUT_CFG_COM_MSPROFTX, INPUT_CFG_COM_TASK_TIME,
        INPUT_CFG_COM_AIC_METRICS, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_ASCENDCL, INPUT_CFG_COM_RUNTIME_API,
        INPUT_CFG_COM_HCCL, INPUT_CFG_COM_L2, INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_USAGE_FREQ,
        INPUT_CFG_COM_SYS_PID_USAGE_FREQ, INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ,
        INPUT_CFG_COM_LLC_MODE, INPUT_CFG_COM_SYS_IO_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_DVPP_FREQ, INPUT_CFG_HOST_SYS_USAGE, INPUT_CFG_HOST_SYS_USAGE_FREQ
        }).swap(commonConfig_);
    return;
}

bool ParamsAdapter::BlackSwitchCheck(InputCfg inputCfg) const
{
    if (inputCfg < INPUT_CFG_MSPROF_APPLICATION || inputCfg >= INPUT_CFG_MAX) {
        MSPROF_LOGE("invalid input for black switch.");
        return false;
    }
    return (std::find(blackSwitch_.begin(), blackSwitch_.end(), inputCfg) != blackSwitch_.end()) ? false : true;
}

PlatformType ParamsAdapter::GetPlatform() const
{
    return platformType_;
}

void ParamsAdapter::SetDefaultAivParams(std::array<std::string, INPUT_CFG_MAX> &paramContainer) const
{
    if (platformType_ == PlatformType::CHIP_V4_1_0 || platformType_ == PlatformType::CHIP_V4_2_0) {
        paramContainer[INPUT_CFG_COM_AI_VECTOR] = paramContainer[INPUT_CFG_COM_AI_CORE];
        paramContainer[INPUT_CFG_COM_AIV_FREQ] = paramContainer[INPUT_CFG_COM_AIC_FREQ];
        paramContainer[INPUT_CFG_COM_AIV_MODE] = paramContainer[INPUT_CFG_COM_AIC_MODE];
        paramContainer[INPUT_CFG_COM_AIV_METRICS] = paramContainer[INPUT_CFG_COM_AIC_METRICS];
    }
}

void ParamsAdapter::SetDefaultLlcMode(std::array<std::string, INPUT_CFG_MAX> &paramContainer) const
{
    if (paramContainer[INPUT_CFG_COM_LLC_MODE].empty()) {
        if (platformType_ == PlatformType::MINI_TYPE) {
            paramContainer[INPUT_CFG_COM_LLC_MODE] = LLC_PROFILING_CAPACITY;
        } else {
            paramContainer[INPUT_CFG_COM_LLC_MODE] = LLC_PROFILING_READ;
        }
    }
}

std::string ParamsAdapter::SetDefaultAicMetricsType() const
{
    return (platformType_ == PlatformType::CHIP_V4_2_0) ? PIPE_EXECUTION_UTILIZATION : PIPE_UTILIZATION;
}

int ParamsAdapter::PlatformAdapterInit(SHARED_PTR_ALIA<ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("params to init platform adapter is nullptr.");
        return PROFILING_FAILED;
    }
    int ret = PlatformAdapter::instance()->Init(params, platformType_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("platformadapter init fail.");
        return PROFILING_FAILED;
    }
    platformAdapter_ = PlatformAdapter::instance()->GetAdapter();
    if (platformAdapter_ == nullptr) {
        MSPROF_LOGE("platformadapter null.");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("platformadapter init done.");
    return PROFILING_SUCCESS;
}

int ParamsAdapter::TransToParam(std::array<std::string, INPUT_CFG_MAX> paramContainer,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("params to trans is nullptr.");
        return PROFILING_FAILED;
    }

    SetCommonParams(paramContainer);

    SetTaskParams(paramContainer);

    SetAiMetricsParams(paramContainer);

    SetDeviceSysParams(paramContainer);

    SetHostSysParams(paramContainer);

    SetHostSysUsageParams(paramContainer);

    return PROFILING_SUCCESS;
}

void ParamsAdapter::SetCommonParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    struct CommonParams commonParams;
    commonParams.output = paramContainer[INPUT_CFG_COM_OUTPUT];
    commonParams.storageLimit = paramContainer[INPUT_CFG_COM_STORAGE_LIMIT];
    commonParams.msproftx = paramContainer[INPUT_CFG_COM_MSPROFTX];
    commonParams.hostSysPid = (paramContainer[INPUT_CFG_HOST_SYS_PID].empty()) ?
        -1 : std::stoi(paramContainer[INPUT_CFG_HOST_SYS_PID]);
    commonParams.device = (paramContainer[INPUT_CFG_COM_SYS_DEVICES].compare("all") == 0) ?
        DrvGetDevIdsStr() : paramContainer[INPUT_CFG_COM_SYS_DEVICES];
    commonParams.profilingPeriod = (paramContainer[INPUT_CFG_COM_SYS_PERIOD].empty()) ?
        -1 : std::stoi(paramContainer[INPUT_CFG_COM_SYS_PERIOD]);
    platformAdapter_->SetParamsForGlobal(commonParams);
    platformAdapter_->SetParamsForStorageLimit(commonParams);
}

void ParamsAdapter::SetTaskParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    if (paramContainer[INPUT_CFG_COM_TASK_TIME].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForTaskTime();
    }
    if (paramContainer[INPUT_CFG_COM_TASK_TRACE].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForTaskTrace();
    }
    if (paramContainer[INPUT_CFG_COM_TRAINING_TRACE].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForTrainingTrace();
    }
    if (paramContainer[INPUT_CFG_COM_ASCENDCL].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForAscendCL();
    }
    if (paramContainer[INPUT_CFG_COM_MODEL_EXECUTION].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForGE();
    }
    if (paramContainer[INPUT_CFG_COM_RUNTIME_API].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForRuntime();
    }
    if (paramContainer[INPUT_CFG_COM_AICPU].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForAICPU();
    }
    if (paramContainer[INPUT_CFG_COM_HCCL].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForHCCL();
    }
    if (paramContainer[INPUT_CFG_COM_L2].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForL2Cache();
    }
    return;
}

void ParamsAdapter::SetAiMetricsParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    int samplingInterval = 0;
    if (paramContainer[INPUT_CFG_COM_AI_CORE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_AIC_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_AIC_FREQ]));
        platformAdapter_->SetParamsForAicMetrics(paramContainer[INPUT_CFG_COM_AIC_MODE],
            paramContainer[INPUT_CFG_COM_AIC_METRICS], samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_AI_VECTOR].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_AIV_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_AIV_FREQ]));
        platformAdapter_->SetParamsForAivMetrics(paramContainer[INPUT_CFG_COM_AIV_MODE],
            paramContainer[INPUT_CFG_COM_AIV_METRICS], samplingInterval);
    }
    return;
}

void ParamsAdapter::SetDeviceSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    int samplingInterval = 0;
    if (paramContainer[INPUT_CFG_COM_SYS_USAGE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_USAGE_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_100MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_USAGE_FREQ]));
        platformAdapter_->SetParamsForDeviceSysCpuMemUsage(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_SYS_PID_USAGE].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_PID_USAGE_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_100MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_PID_USAGE_FREQ]));
        platformAdapter_->SetParamsForDeviceAllPidCpuMemUsage(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_SYS_CPU].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_CPU_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_CPU_FREQ]));
        platformAdapter_->SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_20MS :
            (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]));
        platformAdapter_->SetParamsForDeviceHardwareMem(samplingInterval, paramContainer[INPUT_CFG_COM_LLC_MODE]);
    }
    if (paramContainer[INPUT_CFG_COM_SYS_IO].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_IO_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_10MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_IO_FREQ]));
        platformAdapter_->SetParamsForDeviceIO(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_20MS :
            (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]));
        platformAdapter_->SetParamsForDeviceIntercommection(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_DVPP].compare(MSVP_PROF_ON) == 0) {
        samplingInterval = paramContainer[INPUT_CFG_COM_DVPP_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_COM_DVPP_FREQ]));
        platformAdapter_->SetParamsForDeviceDVPP(samplingInterval);
    }
    if (paramContainer[INPUT_CFG_COM_POWER].compare(MSVP_PROF_ON) == 0) {
        platformAdapter_->SetParamsForDevicePower();
    }
    if (paramContainer[INPUT_CFG_COM_INSTR_PROFILING].compare(MSVP_PROF_ON) == 0) {
        int instrFreq = paramContainer[INPUT_CFG_COM_INSTR_PROFILING_FREQ].empty() ?
            DEFAULT_PROFILING_INSTR_PROFILING_FREQ : std::stoi(paramContainer[INPUT_CFG_COM_INSTR_PROFILING_FREQ]);
        platformAdapter_->SetParamsForDeviceInstr(instrFreq);
    }

    return;
}

void ParamsAdapter::SetHostSysParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    std::vector<std::string> hostSysPidList = Utils::Split(paramContainer[INPUT_CFG_HOST_SYS], false, "", ",");
    for (auto entry : hostSysPidList) {
        if (entry.compare(HOST_SYS_CPU) == 0) {
            platformAdapter_->SetParamsForHostOnePidCpu(DEFAULT_PROFILING_INTERVAL_20MS);
        } else if (entry.compare(HOST_SYS_MEM) == 0) {
            platformAdapter_->SetParamsForHostOnePidMem(DEFAULT_PROFILING_INTERVAL_20MS);
        } else if (entry.compare(HOST_SYS_DISK) == 0) {
            platformAdapter_->SetParamsForHostPidDisk();
        } else if (entry.compare(HOST_SYS_OSRT) == 0) {
            platformAdapter_->SetParamsForHostPidOSRT();
        } else if (entry.compare(HOST_SYS_NETWORK) == 0) {
            platformAdapter_->SetParamsForHostNetwork();
        }
    }
    return;
}

void ParamsAdapter::SetHostSysUsageParams(std::array<std::string, INPUT_CFG_MAX> paramContainer) const
{
    std::vector<std::string> hostSysUsageList = Utils::Split(paramContainer[INPUT_CFG_HOST_SYS_USAGE], false, "", ",");
    int samplingInterval = paramContainer[INPUT_CFG_HOST_SYS_USAGE_FREQ].empty() ?
            DEFAULT_PROFILING_INTERVAL_20MS : (THOUSAND / std::stoi(paramContainer[INPUT_CFG_HOST_SYS_USAGE_FREQ]));
    if (paramContainer[INPUT_CFG_HOST_SYS_PID].empty()) {
        for (auto entry : hostSysUsageList) {
            if (entry.compare(HOST_SYS_CPU) == 0) {
                platformAdapter_->SetParamsForHostAllPidCpu(samplingInterval);
                platformAdapter_->SetParamsForHostSysCpu(samplingInterval);
            } else if (entry.compare(HOST_SYS_MEM) == 0) {
                platformAdapter_->SetParamsForHostAllPidMem(samplingInterval);
                platformAdapter_->SetParamsForHostSysMem(samplingInterval);
            }
        }
    } else {
        for (auto entry : hostSysUsageList) {
            if (entry.compare(HOST_SYS_CPU) == 0) {
                platformAdapter_->SetParamsForHostOnePidCpu(samplingInterval);
            } else if (entry.compare(HOST_SYS_MEM) == 0) {
                platformAdapter_->SetParamsForHostOnePidMem(samplingInterval);
            }
        }
    }
}

int ParamsAdapter::ComCfgCheck(std::array<std::string, INPUT_CFG_MAX> paramContainer,
    std::set<InputCfg> &setArgs) const
{
    bool ret = true;
    for (auto inputCfg : commonConfig_) {
        if (setArgs.find(inputCfg) == setArgs.end()) {
            continue;
        }
        std::string cfgValue = paramContainer[inputCfg];
        if (inputCfg <= INPUT_CFG_COM_AICPU) {
            ret = ComCfgCheck1(inputCfg, cfgValue);
        } else {
            ret = ComCfgCheck2(inputCfg, cfgValue);
        }
        if (!ret) {
            MSPROF_LOGE("Input argument check fail.");
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

bool ParamsAdapter::ComCfgCheck1(const InputCfg inputCfg, const std::string &cfgValue) const
{
    bool ret = true;
    std::map<int, std::string> switchNameMap = {
        {INPUT_CFG_COM_MSPROFTX, "msproftx"}, {INPUT_CFG_COM_TASK_TIME, "task-time"},
        {INPUT_CFG_COM_ASCENDCL, "ascendcl"}, {INPUT_CFG_COM_RUNTIME_API, "runtime-api"},
        {INPUT_CFG_COM_HCCL, "hccl"}, {INPUT_CFG_COM_L2, "l2"},
        {INPUT_CFG_COM_AICPU, "aicpu"}
    };
    std::map<int, std::string> metricsNameMap = {
        {INPUT_CFG_COM_AIC_METRICS, "aic-metrics"},
        {INPUT_CFG_COM_AIV_METRICS, "aiv-metrics"},
    };
    switch (inputCfg) {
        case INPUT_CFG_COM_OUTPUT:
            ret = ParamValidation::instance()->CheckOutputIsValid(cfgValue);
            break;
        case INPUT_CFG_COM_STORAGE_LIMIT:
            ret = ParamValidation::instance()->CheckStorageLimit(cfgValue);
            break;
        case INPUT_CFG_COM_MSPROFTX:
        case INPUT_CFG_COM_TASK_TIME:
        case INPUT_CFG_COM_ASCENDCL:
        case INPUT_CFG_COM_RUNTIME_API:
        case INPUT_CFG_COM_HCCL:
        case INPUT_CFG_COM_L2:
        case INPUT_CFG_COM_AICPU:
            ret = ParamValidation::instance()->IsValidInputCfgSwitch(switchNameMap[inputCfg], cfgValue);
            break;
        case INPUT_CFG_COM_AIC_METRICS:
        case INPUT_CFG_COM_AIV_METRICS:
            ret = ParamValidation::instance()->CheckProfilingMetricsIsValid(metricsNameMap[inputCfg], cfgValue);
            break;
        default:
            ret = false;
    }
    return ret;
}

bool ParamsAdapter::ComCfgCheck2(const InputCfg inputCfg, const std::string &cfgValue) const
{
    bool ret = true;
    switch (inputCfg) {
        case INPUT_CFG_COM_SYS_USAGE_FREQ:
        case INPUT_CFG_COM_SYS_PID_USAGE_FREQ:
        case INPUT_CFG_COM_SYS_CPU_FREQ:
        case INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ:
        case INPUT_CFG_COM_SYS_IO_FREQ:
        case INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ:
        case INPUT_CFG_COM_DVPP_FREQ:
        case INPUT_CFG_HOST_SYS_USAGE_FREQ:
            ret = CheckFreqValid(cfgValue, inputCfg);
            break;
        case INPUT_CFG_COM_LLC_MODE:
            ret = ParamValidation::instance()->CheckLlcModeIsValid(cfgValue);
            break;
        case INPUT_CFG_HOST_SYS_USAGE:
            ret = ParamValidation::instance()->CheckHostSysUsageValid(cfgValue);
            break;
        default:
            ret = false;
    }
    return ret;
}

bool ParamsAdapter::CheckFreqValid(const std::string &freq, const InputCfg freqOpt) const
{
    std::map<InputCfg, std::vector<int>> freqRangeMap = {
        {INPUT_CFG_COM_AIC_FREQ, {AIC_FREQ_MIN, AIC_FREQ_MAX}},
        {INPUT_CFG_COM_AIV_FREQ, {AIV_FREQ_MIN, AIV_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_USAGE_FREQ, {SYS_USAGE_FREQ_MIN, SYS_USAGE_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_CPU_FREQ, {SYS_CPU_FREQ_MIN, SYS_CPU_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_PID_USAGE_FREQ, {SYS_PID_USAGE_FREQ_MIN, SYS_PID_USAGE_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, {SYS_HARDWARE_MEM_FREQ_MIN, SYS_HARDWARE_MEM_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_IO_FREQ, {SYS_IO_FREQ_MIN, SYS_IO_FREQ_MAX}},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, {SYS_INTERCONNECTION_FREQ_MIN, SYS_INTERCONNECTION_FREQ_MAX}},
        {INPUT_CFG_COM_DVPP_FREQ, {DVPP_FREQ_MIN, DVPP_FREQ_MAX}},
        {INPUT_CFG_COM_INSTR_PROFILING_FREQ, {INSTR_PROFILING_FREQ_MIN, INSTR_PROFILING_FREQ_MAX}},
        {INPUT_CFG_HOST_SYS_USAGE_FREQ, {HOST_SYS_USAGE_FREQ_MIN, HOST_SYS_USAGE_FREQ_MAX}},
        {INPUT_CFG_MSPROF_DYNAMIC_PID, {APP_PID_MIN, APP_PID_MAX}},
    };
    std::map<InputCfg, std::string> freqCfgNameMap = {
        {INPUT_CFG_COM_AIC_FREQ, "aic-freq"},
        {INPUT_CFG_COM_AIV_FREQ, "aiv-freq"},
        {INPUT_CFG_COM_SYS_USAGE_FREQ, "sys-sampling-freq"},
        {INPUT_CFG_COM_SYS_CPU_FREQ, "sys-cpu-freq"},
        {INPUT_CFG_COM_SYS_PID_USAGE_FREQ, "sys-pid-sampling-freq"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, "sys-hardware-mem-freq"},
        {INPUT_CFG_COM_SYS_IO_FREQ, "sys-io-sampling-freq"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, "sys-interconnection-freq"},
        {INPUT_CFG_COM_DVPP_FREQ, "dvpp-freq"},
        {INPUT_CFG_COM_INSTR_PROFILING_FREQ, "instr-profiling-freq"},
        {INPUT_CFG_HOST_SYS_USAGE_FREQ, "host-sys-usage-freq"},
        {INPUT_CFG_MSPROF_DYNAMIC_PID, "pid"},
    };
    std::vector<int> checkFreqRange = freqRangeMap[freqOpt];
    return ParamValidation::instance()->CheckFreqIsValid(freqCfgNameMap[freqOpt], freq, checkFreqRange[0],
        checkFreqRange[1]);
}
} // ParamsAdapter
} // Dvvp
} // Collector