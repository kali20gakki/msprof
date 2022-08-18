/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */
#include "param_adapter.h"
#include <map>
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "utils/utils.h"
#include "input_parser.h"
#include "cmd_log.h"
#include "config.h"
#include "config/config_manager.h"
#include "platform/platform.h"

namespace Collector {
namespace Dvvp {
namespace Param {

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Msprofbin;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::message;

ParamsAdapter::ParamsAdapter()
{
}

int ParamsAdapter::GenParamsFromMsprof(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    paramContainer_.fill("");
    int ret = TransFromMsprof(msprofCfg);
    ret = ValidCheck(ENABLE_MSPROF);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("params check failed");
    }
    return TransFromParamsContainer(ENABLE_MSPROF);
}

int ParamsAdapter::GenParamsFromAclJson(ProfAclConfig aclCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    paramContainer_.fill("");
    int ret = TransFromAclJson(aclCfg);
    ret = ValidCheck(ENABLE_ACL_JSON);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("params check failed");
    }
    return TransFromParamsContainer(ENABLE_ACL_JSON);
}

int ParamsAdapter::GenParamsFromGeOpt(ProfGeOptionsConfig geCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    paramContainer_.fill("");
    int ret = TransFromGeOpt(geCfg);
    ret = ValidCheck(ENABLE_GE_OPTION);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("params check failed");
    }
    return TransFromParamsContainer(ENABLE_GE_OPTION);

}

int ParamsAdapter::GenParamsFromApi(const ProfConfig * apiCfg, const std::array<std::string, ACL_PROF_ARGS_MAX> &argsArr, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    paramContainer_.fill("");
    return PROFILING_SUCCESS;
}

int ParamsAdapter::TransFromMsprof(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg)
{

    for (auto iter = msprofCfg.begin(); iter != msprofCfg.end(); iter++) {
        switch(iter->first) {
            case ARGS_OUTPUT:
                paramContainer_[INPUT_CFG_COM_OUTPUT] = std::string(iter->second.args[ARGS_OUTPUT]);
                break;
            case ARGS_STORAGE_LIMIT:
                paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = std::string(iter->second.args[ARGS_STORAGE_LIMIT]);
                break;
            case ARGS_APPLICATION:
                paramContainer_[INPUT_CFG_MSPROF_APPLICATION] = std::string(iter->second.args[ARGS_APPLICATION]);
                break;
            case ARGS_ENVIRONMENT:
                paramContainer_[INPUT_CFG_MSPROF_ENVIRONMENT] = std::string(iter->second.args[ARGS_ENVIRONMENT]);
                break;
            case ARGS_AIC_MODE:
                paramContainer_[INPUT_CFG_COM_AIC_MODE] = std::string(iter->second.args[ARGS_AIC_MODE]);
                break;
            case ARGS_AIC_METRICE:
                paramContainer_[INPUT_CFG_COM_AIC_METRICS] = std::string(iter->second.args[ARGS_AIC_METRICE]);
                break;
            case ARGS_AIV_MODE:
                paramContainer_[INPUT_CFG_COM_AIV_MODE] = std::string(iter->second.args[ARGS_AIV_MODE]);
                break;
            case ARGS_AIV_METRICS:
                paramContainer_[INPUT_CFG_COM_AIV_METRICS] = std::string(iter->second.args[ARGS_AIV_METRICS]);
                break;
            case ARGS_SYS_DEVICES:
                paramContainer_[INPUT_CFG_COM_AIV_METRICS] = std::string(iter->second.args[ARGS_AIV_METRICS]);
                break;
            case ARGS_LLC_PROFILING:
                paramContainer_[INPUT_CFG_COM_LLC_PROFILING] = std::string(iter->second.args[ARGS_LLC_PROFILING]);
                break;
            case ARGS_PYTHON_PATH:
                paramContainer_[INPUT_CFG_PYTHON_PATH] = std::string(iter->second.args[ARGS_PYTHON_PATH]);
                break;
            case ARGS_SUMMARY_FORMAT:
                paramContainer_[INPUT_CFG_SUMMARY_FORMAT] = std::string(iter->second.args[ARGS_SUMMARY_FORMAT]);
                break;
            case ARGS_ASCENDCL:
                paramContainer_[INPUT_CFG_COM_ASCENDCL] = std::string(iter->second.args[ARGS_ASCENDCL]);
                break;
            case ARGS_AI_CORE:
                paramContainer_[INPUT_CFG_COM_AI_CORE] = std::string(iter->second.args[ARGS_AI_CORE]);
                break;
            case ARGS_AIV:
                paramContainer_[INPUT_CFG_COM_AI_VECTOR_CORE] = std::string(iter->second.args[ARGS_AIV]);
                break;
            case ARGS_MODEL_EXECUTION:
                paramContainer_[INPUT_CFG_COM_MODEL_EXECUTION] = std::string(iter->second.args[ARGS_MODEL_EXECUTION]);
                break;
            case ARGS_RUNTIME_API:
                paramContainer_[INPUT_CFG_COM_MODEL_EXECUTION] = std::string(iter->second.args[ARGS_RUNTIME_API]);
                break;
            case ARGS_TASK_TIME:
                paramContainer_[INPUT_CFG_COM_TASK_TIME] = std::string(iter->second.args[ARGS_TASK_TIME]);
                break;
            case ARGS_AICPU:
                paramContainer_[INPUT_CFG_COM_AICPU] = std::string(iter->second.args[ARGS_AICPU]);
                break;
            case ARGS_MSPROFTX:
                paramContainer_[INPUT_CFG_COM_MSPROFTX] = std::string(iter->second.args[ARGS_MSPROFTX]);
                break;
            case ARGS_CPU_PROFILING:
                paramContainer_[INPUT_CFG_COM_SYS_CPU_PROFILING] = std::string(iter->second.args[ARGS_CPU_PROFILING]);
                break;
            case ARGS_SYS_PROFILING:
                paramContainer_[INPUT_CFG_COM_SYS_PROFILING] = std::string(iter->second.args[ARGS_SYS_PROFILING]);
                break;
            case ARGS_PID_PROFILING:
                paramContainer_[INPUT_CFG_COM_SYS_PID_PROFILING] = std::string(iter->second.args[ARGS_PID_PROFILING]);
                break;
            case ARGS_HARDWARE_MEM:
                paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM] = std::string(iter->second.args[ARGS_HARDWARE_MEM]);
                break;
            case ARGS_IO_PROFILING:
                paramContainer_[INPUT_CFG_COM_SYS_IO_PROFILING] = std::string(iter->second.args[ARGS_IO_PROFILING]);
                break;
            case ARGS_INTERCONNECTION_PROFILING:
                paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING] = std::string(iter->second.args[ARGS_INTERCONNECTION_PROFILING]);
                break;
            case ARGS_DVPP_PROFILING:
                paramContainer_[INPUT_CFG_COM_DVPP_PROFILING] = std::string(iter->second.args[ARGS_DVPP_PROFILING]);
                break;
            case ARGS_LOW_POWER:
                paramContainer_[INPUT_CFG_COM_POWER] = std::string(iter->second.args[ARGS_LOW_POWER]);
                break;
            case ARGS_HCCL:
                paramContainer_[INPUT_CFG_COM_HCCL] = std::string(iter->second.args[ARGS_HCCL]);
                break;
            case ARGS_BIU:
                paramContainer_[INPUT_CFG_COM_BIU] = std::string(iter->second.args[ARGS_BIU]);
                break;
            case ARGS_L2_PROFILING:
                paramContainer_[INPUT_CFG_COM_L2] = std::string(iter->second.args[ARGS_L2_PROFILING]);
                break;
            case ARGS_PARSE:
                paramContainer_[INPUT_CFG_PARSE] = std::string(iter->second.args[ARGS_PARSE]);
                break;
            case ARGS_QUERY:
                paramContainer_[INPUT_CFG_QUERY] = std::string(iter->second.args[ARGS_QUERY]);
                break;
            case ARGS_EXPORT:
                paramContainer_[INPUT_CFG_EXPORT] = std::string(iter->second.args[ARGS_EXPORT]);
                break;
            case ARGS_AIC_FREQ:
                paramContainer_[INPUT_CFG_COM_AIC_FREQ] = std::string(iter->second.args[ARGS_AIC_FREQ]);
                break;
            case ARGS_AIV_FREQ:
                paramContainer_[INPUT_CFG_COM_AIV_FREQ] = std::string(iter->second.args[ARGS_AIV_FREQ]);
                break;
            case ARGS_BIU_FREQ:
                paramContainer_[INPUT_CFG_COM_BIU_FREQ] = std::string(iter->second.args[ARGS_BIU_FREQ]);
                break;
            case ARGS_SYS_PERIOD:
                paramContainer_[INPUT_CFG_COM_SYS_PERIOD] = std::string(iter->second.args[ARGS_SYS_PERIOD]);
                break;
            case ARGS_SYS_SAMPLING_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_SAMPLING_FREQ] = std::string(iter->second.args[ARGS_SYS_SAMPLING_FREQ]);
                break;
            case ARGS_PID_SAMPLING_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_PID_SAMPLING_FREQ] = std::string(iter->second.args[ARGS_PID_SAMPLING_FREQ]);
                break;
            case ARGS_HARDWARE_MEM_SAMPLING_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = std::string(iter->second.args[ARGS_HARDWARE_MEM_SAMPLING_FREQ]);
                break;
            case ARGS_IO_SAMPLING_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ] = std::string(iter->second.args[ARGS_IO_SAMPLING_FREQ]);
                break;
            case ARGS_DVPP_FREQ:
                paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = std::string(iter->second.args[ARGS_DVPP_FREQ]);
                break;
            case ARGS_CPU_SAMPLING_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_CPU_FREQ] = std::string(iter->second.args[ARGS_CPU_SAMPLING_FREQ]);
                break;
            case ARGS_INTERCONNECTION_FREQ:
                paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = std::string(iter->second.args[ARGS_INTERCONNECTION_FREQ]);
                break;
            case ARGS_EXPORT_ITERATION_ID:
                paramContainer_[INPUT_CFG_ITERATION_ID] = std::string(iter->second.args[ARGS_EXPORT_ITERATION_ID]);
                break;
            case ARGS_EXPORT_MODEL_ID:
                paramContainer_[INPUT_CFG_MODEL_ID] = std::string(iter->second.args[ARGS_EXPORT_MODEL_ID]);
                break;
            case ARGS_HOST_SYS:
                paramContainer_[INPUT_CFG_HOST_SYS] = std::string(iter->second.args[ARGS_HOST_SYS]);
                break;
            case ARGS_HOST_SYS_PID:
                paramContainer_[INPUT_CFG_HOST_SYS_PID] = std::string(iter->second.args[ARGS_HOST_SYS_PID]);
                break;
        }
    }
}

int ParamsAdapter::TransFromAclJson(ProfAclConfig aclCfg)
{
    paramContainer_[INPUT_CFG_ACL_JSON_SWITCH] = aclCfg.switch_();

    paramContainer_[INPUT_CFG_COM_OUTPUT] = aclCfg.output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = aclCfg.storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = aclCfg.msproftx();

    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = aclCfg.aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = aclCfg.aiv_metrics();
    paramContainer_[INPUT_CFG_COM_TASK_TRACE] = aclCfg.task_trace();
    paramContainer_[INPUT_CFG_COM_AICPU] = aclCfg.aicpu();
    paramContainer_[INPUT_CFG_COM_ASCENDCL] = aclCfg.ascendcl();
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = aclCfg.runtime_api();
    paramContainer_[INPUT_CFG_COM_L2] = aclCfg.l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = aclCfg.hccl();

    paramContainer_[INPUT_CFG_COM_BIU] = aclCfg.biu();
    paramContainer_[INPUT_CFG_COM_BIU_FREQ] = aclCfg.biu_freq();

    paramContainer_[INPUT_CFG_COM_SYS_PROFILING] = aclCfg.sys_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_SAMPLING_FREQ] = aclCfg.sys_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_CPU_PROFILING] = aclCfg.sys_cpu_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_CPU_FREQ] = aclCfg.sys_cpu_freq();
    paramContainer_[INPUT_CFG_COM_SYS_PID_PROFILING] = aclCfg.sys_pid_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_PID_SAMPLING_FREQ] = aclCfg.sys_pid_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM] = aclCfg.sys_hardware_mem();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = aclCfg.sys_hardware_mem_freq();
    paramContainer_[INPUT_CFG_COM_SYS_IO_PROFILING] = aclCfg.sys_io_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ] = aclCfg.sys_io_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING] = aclCfg.sys_interconnection_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = aclCfg.sys_interconnection_freq();
    paramContainer_[INPUT_CFG_COM_DVPP_PROFILING] = aclCfg.dvpp_profiling();
    paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = aclCfg.dvpp_freq();

    paramContainer_[INPUT_CFG_HOST_SYS] = aclCfg.host_sys();

    return PROFILING_SUCCESS;
}

int ParamsAdapter::TransFromGeOpt(ProfGeOptionsConfig geCfg)
{
    paramContainer_[INPUT_CFG_GE_OPT_FP_POINT] = geCfg.fp_point();
    paramContainer_[INPUT_CFG_GE_OPT_BP_POINT] = geCfg.bp_point();

    paramContainer_[INPUT_CFG_COM_OUTPUT] = geCfg.output();
    paramContainer_[INPUT_CFG_COM_STORAGE_LIMIT] = geCfg.storage_limit();
    paramContainer_[INPUT_CFG_COM_MSPROFTX] = geCfg.msproftx();

    paramContainer_[INPUT_CFG_COM_AIC_METRICS] = geCfg.aic_metrics();
    paramContainer_[INPUT_CFG_COM_AIV_METRICS] = geCfg.aiv_metrics();
    paramContainer_[INPUT_CFG_COM_TASK_TRACE] = geCfg.task_trace();
    paramContainer_[INPUT_CFG_COM_TRAINING_TRACE] = geCfg.training_trace();
    paramContainer_[INPUT_CFG_COM_AICPU] = geCfg.aicpu();
    paramContainer_[INPUT_CFG_COM_RUNTIME_API] = geCfg.runtime_api();
    paramContainer_[INPUT_CFG_COM_L2] = geCfg.l2();
    paramContainer_[INPUT_CFG_COM_HCCL] = geCfg.hccl();

    paramContainer_[INPUT_CFG_COM_BIU] = geCfg.biu();
    paramContainer_[INPUT_CFG_COM_BIU_FREQ] = geCfg.biu_freq();

    paramContainer_[INPUT_CFG_COM_SYS_PROFILING] = geCfg.sys_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_SAMPLING_FREQ] = geCfg.sys_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_CPU_PROFILING] = geCfg.sys_cpu_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_CPU_FREQ] = geCfg.sys_cpu_freq();
    paramContainer_[INPUT_CFG_COM_SYS_PID_PROFILING] = geCfg.sys_pid_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_PID_SAMPLING_FREQ] = geCfg.sys_pid_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM] = geCfg.sys_hardware_mem();
    paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ] = geCfg.sys_hardware_mem_freq();
    paramContainer_[INPUT_CFG_COM_SYS_IO_PROFILING] = geCfg.sys_io_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ] = geCfg.sys_io_sampling_freq();
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING] = geCfg.sys_interconnection_profiling();
    paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ] = geCfg.sys_interconnection_freq();
    paramContainer_[INPUT_CFG_COM_DVPP_PROFILING] = geCfg.dvpp_profiling();
    paramContainer_[INPUT_CFG_COM_DVPP_FREQ] = geCfg.dvpp_freq();

    paramContainer_[INPUT_CFG_HOST_SYS] = geCfg.host_sys();

    return PROFILING_SUCCESS; 
}

int ParamsAdapter::TransFromApi(const ProfConfig * apiCfg, const std::array<std::string, ACL_PROF_ARGS_MAX> &argsArr)
{
    return PROFILING_SUCCESS;
}

int ParamsAdapter::BlackSwitchCheck(EnableType enableType)
{
    std::vector<InputCfg> miniBlackSwith = {INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING,
        INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_L2, INPUT_CFG_COM_AI_VECTOR_CORE, INPUT_CFG_COM_AIV_FREQ,
        INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
        INPUT_CFG_COM_BIU_FREQ};
    std::vector<InputCfg> cloudBlackSwith = {INPUT_CFG_COM_AI_VECTOR_CORE, INPUT_CFG_COM_AIV_FREQ,
        INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU,
        INPUT_CFG_COM_BIU_FREQ};
    std::vector<InputCfg> mdcBlackSwith = {INPUT_CFG_COM_SYS_IO_PROFILING, INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING, INPUT_CFG_COM_AICPU,
        INPUT_CFG_COM_POWER, INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY,
        INPUT_CFG_EXPORT, INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ};
    std::vector<InputCfg> dcBlackSwith = {INPUT_CFG_COM_AI_VECTOR_CORE, INPUT_CFG_COM_AIV_FREQ,
        INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO_PROFILING,
        INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ, INPUT_CFG_COM_POWER, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ};
    std::vector<InputCfg> lhisiBlackSwith = {INPUT_CFG_COM_AI_VECTOR_CORE, INPUT_CFG_COM_AIV_FREQ,
        INPUT_CFG_COM_AIV_MODE, INPUT_CFG_COM_AIV_METRICS, INPUT_CFG_COM_SYS_IO_PROFILING,
        INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ, INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ,
        INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING, INPUT_CFG_COM_AICPU, INPUT_CFG_COM_SYS_HARDWARE_MEM,
        INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, INPUT_CFG_COM_L2, INPUT_CFG_COM_DVPP_PROFILING, INPUT_CFG_COM_DVPP_FREQ,
        INPUT_CFG_COM_SYS_CPU_FREQ, INPUT_CFG_COM_SYS_CPU_PROFILING, INPUT_CFG_COM_LLC_PROFILING, INPUT_CFG_COM_POWER,
        INPUT_CFG_PYTHON_PATH, INPUT_CFG_SUMMARY_FORMAT, INPUT_CFG_PARSE, INPUT_CFG_QUERY, INPUT_CFG_EXPORT,
        INPUT_CFG_ITERATION_ID, INPUT_CFG_MODEL_ID, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ};
    std::vector<InputCfg> cloudBlackSwithV2 = {};

    std::map<int, std::string> blackSwitchOptions = {
        {INPUT_CFG_COM_AI_VECTOR_CORE, "--ai-vector-core"},
        {INPUT_CFG_COM_AIV_FREQ, "--aiv-freq"},
        {INPUT_CFG_COM_AIV_MODE, "--aiv-mode"},
        {INPUT_CFG_COM_AIV_METRICS, "--aiv-metrics"},
        {INPUT_CFG_COM_L2, "--l2"},
        {INPUT_CFG_COM_AICPU, "--aicpu"},
        {INPUT_CFG_COM_SYS_CPU_PROFILING,"--sys-cpu-profiling"},
        {INPUT_CFG_COM_SYS_CPU_FREQ, "--sys-cpu-freq"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM, "--sys-hardware-mem"},
        {INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ, "--sys-hardware-mem-freq"},
        {INPUT_CFG_COM_LLC_PROFILING, "--llc-profiling"},
        {INPUT_CFG_COM_SYS_IO_PROFILING, "--sys-io-profiling"},
        {INPUT_CFG_COM_SYS_IO_SAMPLING_FREQ, "--sys-io-sampling-freq"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_PROFILING, "--sys-interconnection-profiling"},
        {INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ, "--sys-interconnection-freq"},
        {INPUT_CFG_COM_DVPP_PROFILING, "--dvpp-profiling"},
        {INPUT_CFG_COM_DVPP_FREQ, "--dvpp-freq"},
        {INPUT_CFG_COM_POWER, "--power"},
        {INPUT_CFG_COM_BIU, "--biu"},
        {INPUT_CFG_COM_BIU_FREQ, "--biu-freq"},
        {INPUT_CFG_PYTHON_PATH, "--python-path"},
        {INPUT_CFG_SUMMARY_FORMAT, "--summary-format"},
        {INPUT_CFG_PARSE, "--parse"},
        {INPUT_CFG_QUERY, "--query"},
        {INPUT_CFG_EXPORT, "--export"},
        {INPUT_CFG_ITERATION_ID, "--iteration-id"},
        {INPUT_CFG_MODEL_ID, "--model-id"},
    };

    std::map<Analysis::Dvvp::Common::Config::PlatformType, std::vector<InputCfg>> platformArgsType = {
        {PlatformType::MINI_TYPE, miniBlackSwith},
        {PlatformType::CLOUD_TYPE, cloudBlackSwith},
        {PlatformType::MDC_TYPE, mdcBlackSwith},
        {PlatformType::LHISI_TYPE, lhisiBlackSwith},
        {PlatformType::DC_TYPE, dcBlackSwith},
        {PlatformType::CHIP_V4_1_0, cloudBlackSwithV2},
    };
    Analysis::Dvvp::Common::Config::PlatformType platformType = ConfigManager::instance()->GetPlatformType();
    if (platformType < PlatformType::MINI_TYPE || platformType >= PlatformType::END_TYPE) {
        return PROFILING_FAILED;
    }
    std::vector<InputCfg> platBlackSwitch = platformArgsType[platformType];
    InputParser parser;
    for (auto iter : platBlackSwitch) {
        if (!paramContainer_[iter].empty()) {
            if (enableType == ENABLE_MSPROF) {
                std::cout << Utils::GetSelfPath() << ": unsupported option '"
                          << blackSwitchOptions.find(iter)->second << "'" << std::endl;
                std::cout << "platformType:" << static_cast<uint8_t>(platformType) << std::endl;
                parser.MsprofCmdUsage("");
                return PROFILING_FAILED;
            }else {
                MSPROF_LOGE("Unsupported option : %s in platformType : %d!", blackSwitchOptions.find(iter)->second,
                static_cast<uint8_t>(platformType));
            }
        }
    }
    return PROFILING_SUCCESS;
}

int ParamsAdapter::ValidCheck(EnableType enableType)
{
    int ret = BlackSwitchCheck(enableType);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    //2、common params check

    //3、specific params check
    switch (enableType) {
        case ENABLE_MSPROF:

            break;
        case ENABLE_ACL_JSON:
            if (!paramContainer_[INPUT_CFG_ACL_JSON_SWITCH].empty() &&
                paramContainer_[INPUT_CFG_ACL_JSON_SWITCH] != MSVP_PROF_ON &&
                paramContainer_[INPUT_CFG_ACL_JSON_SWITCH] != MSVP_PROF_OFF) {
                CmdLog::instance()->CmdErrorLog("Invalid value %s for Argument 'switch'. Please input 'on' or 'off'.",
                    paramContainer_[INPUT_CFG_ACL_JSON_SWITCH].c_str());
                return PROFILING_FAILED;
            }
            break;
        case ENABLE_GE_OPTION:

            break;
        case ENABLE_API:
            break;
        default:
            break;
    }
}

void ParamsAdapter::SetDefaultMsprof()
{
    if (params_->acl.empty()) {
        params_->acl = MSVP_PROF_ON;
    }

    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        if (params_->ts_timeline.empty()) {
            params_->ts_timeline = MSVP_PROF_ON;
        }
    } else {
        if (params_->hwts_log.empty()) {
            params_->hwts_log = MSVP_PROF_ON;
        }
        if (params_->hwts_log1.empty()) {
            params_->hwts_log1 = MSVP_PROF_ON;
        }
    }
    if (params_->ts_memcpy.empty()) {
        params_->ts_memcpy = MSVP_PROF_ON;
    }
    if (params_->ts_keypoint.empty()) {
        params_->ts_keypoint = MSVP_PROF_ON;
    }
    if (params_->ai_core_profiling.empty()) {
        params_->ai_core_profiling = MSVP_PROF_ON;
    }
    if (params_->ai_core_profiling_mode.empty()) {
        params_->ai_core_profiling_mode = PROFILING_MODE_TASK_BASED;
    }
    if (params_->aiv_profiling.empty()) {
        params_->aiv_profiling = MSVP_PROF_ON;
    }
    if (params_->aiv_profiling_mode.empty()) {
        params_->aiv_profiling_mode = PROFILING_MODE_TASK_BASED;
    }
}

void ParamsAdapter::ComDefValueSet()
{
    
}

void ParamsAdapter::SetDefaultAclJson()
{
    // TO Check
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    params_->job_id = Utils::ProfCreateId(0);
    params_->ai_core_profiling = MSVP_PROF_ON;
    params_->ai_core_profiling_mode = PROFILING_MODE_TASK_BASED;
    params_->acl = MSVP_PROF_ON;
    params_->modelExecution = MSVP_PROF_ON;
    params_->runtimeApi = MSVP_PROF_ON;
    params_->runtimeTrace = MSVP_PROF_ON;
    params_->ts_keypoint = MSVP_PROF_ON;
    params_->ts_memcpy = MSVP_PROF_ON;
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        params_->ts_timeline = MSVP_PROF_ON;
    } else {
        params_->hwts_log = MSVP_PROF_ON;
    }
}

void ParamsAdapter::SetDefaultGeOpt()
{
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    params_->job_id = Utils::ProfCreateId(0);
    params_->ts_keypoint = MSVP_PROF_ON;
}

void ParamsAdapter::SetDefaultApi()
{
    return;
}

int ParamsAdapter::TransFromParamsContainer(EnableType enableType)
{
    params_->result_dir = paramContainer_[INPUT_CFG_COM_OUTPUT];
    ComDefValueSet();
    switch(enableType) {
        case ENABLE_MSPROF:
            SetDefaultMsprof();
            break;
        case ENABLE_ACL_JSON:
            SetDefaultAclJson();
            break;
        case ENABLE_GE_OPTION:
            SetDefaultGeOpt();
            break;
        case ENABLE_API:
            SetDefaultApi();
            break;
        default:
            return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
} // Param
} // Dvpp
} // Collector