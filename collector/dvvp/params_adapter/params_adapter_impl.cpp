/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */

#include "params_adapter_impl.h"

#include "errno/error_code.h"
#include "config/config.h"
namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
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
}

int MsprofParamAdapter::ParamsCheckMsprof(std::vector<InputCfg> &cfgList) const
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
                ret = MsprofCheckAppValid(cfgValue);
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
            cfgList.push_back(inputCfg);
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}


int MsprofParamAdapter::TransToParams()
{
    params_->acl = MSVP_PROF_ON;
    params_->result_dir = "/usr/local/Ascend";
    return PROFILING_SUCCESS;
}

int MsprofParamAdapter::GetParamFromInputCfg(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    int ret = Init(); // 派生类初始化+基类初始化
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    
    // [1]黑名单校验+表参数映射（用户参数-->全量表）
    for (auto argPair : msprofCfg) {
        MsprofArgsType argsType = argPair.first;
        InputCfg cfgType = cfgMap_[argsType];
        if (!BlackSwitchCheck(cfgType)) {
            return PROFILING_FAILED;
        }
        // TODO 判断argPair.second.args[argsType]是不是空字符串
        if (!argPair.second.args[argsType]) {
            return PROFILING_FAILED;
        }
        paramContainer_[cfgType] = argPair.second.args[argsType];
        setConfig_.insert(cfgType);
    }
    // 

    // [2] 默认值开启还是关闭（用户层面）+ 补全表 + 表中每一个参数都有一个具体的值[TODO]


// ---------------------------- 表格的每一个参数都是确认值（用户层面） --------------------

    // [4] 私有参数校验（派生类实现）
    std::vector<InputCfg> errCfgList;
    ret = ParamsCheckMsprof(errCfgList);
    if (ret != PROFILING_SUCCESS && !errCfgList.empty()) {
        // todo 打印errCfgList中的错误
        return PROFILING_FAILED;
    }
    // [5] 公有参数校验（调基类接口）
    errCfgList.clear();
    ret = ComCfgCheck(ENABLE_MSPROF, paramContainer_, errCfgList);
    if (ret != PROFILING_SUCCESS) {
        // todo 打印errCfgList中的错误
        return PROFILING_FAILED;
    }

    // [6] 参数转换，转成Params（软件栈转uint64_t， 非软件栈保留在Params）
    ret = TransToParams();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;

}

void MsprofParamAdapter::CreateCfgMap()
{
    std::unordered_map<int, InputCfg>({
        {ARGS_OUTPUT, INPUT_CFG_COM_OUTPUT},
        {ARGS_STORAGE_LIMIT, INPUT_CFG_COM_STORAGE_LIMIT},
        {ARGS_APPLICATION, INPUT_CFG_MSPROF_APPLICATION},
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
}


//void MsprofParamAdapter::DefaultCfgSet(RunningMode rMode)
//{
//    // ===================== APP ========================
//    // ascendcl
//    if (paramContainer_[INPUT_CFG_COM_ASCENDCL].empty()) {
//        paramContainer_[INPUT_CFG_COM_ASCENDCL] = MSVP_PROF_ON;
//    }
//    if (paramContainer_[INPUT_CFG_COM_TASK_TIME].empty()) {
//        paramContainer_[INPUT_CFG_COM_TASK_TIME] = MSVP_PROF_ON;
//    }
//    if (paramContainer_[INPUT_CFG_COM_AI_CORE].empty()) {
//        paramContainer_[INPUT_CFG_COM_AI_CORE] = MSVP_PROF_ON;
//    }
//    if (paramContainer_[INPUT_CFG_COM_AIC_MODE].empty()) {
//        paramContainer_[INPUT_CFG_COM_AIC_MODE] = PROFILING_MODE_TASK_BASED;
//    }
//    if (paramContainer_[INPUT_CFG_COM_AI_VECTOR].empty()) {
//        paramContainer_[INPUT_CFG_COM_AI_VECTOR] = MSVP_PROF_ON;
//    }
//    if (paramContainer_[INPUT_CFG_COM_AIV_MODE].empty()) {
//        paramContainer_[INPUT_CFG_COM_AIV_MODE] = PROFILING_MODE_TASK_BASED;
//    }
//    
//}


// ================================================= Acl json ==========================================

int AclJsonParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    std::vector<InputCfg>({
        INPUT_CFG_ACL_JSON_SWITCH, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
    }).swap(aclJsonConfig_);
}

int AclJsonParamAdapter::ParamsCheckAclJson(std::vector<InputCfg> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : aclJsonConfig_) {
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_ACL_JSON_SWITCH:
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
            cfgList.push_back(inputCfg);
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int AclJsonParamAdapter::GetParamFromInputCfg(ProfAclConfig aclCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    Init();
    
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
        INPUT_CFG_GE_OPT_FP_POINT, INPUT_CFG_GE_OPT_BP_POINT, INPUT_CFG_COM_TASK_TRACE,
        INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_BIU, INPUT_CFG_COM_BIU_FREQ
    }).swap(geOptConfig_);
}

int GeOptParamAdapter::ParamsCheckGeOpt(std::vector<InputCfg> &cfgList) const
{
    int ret = PROFILING_SUCCESS;
    bool flag = true;
    for (auto inputCfg : geOptConfig_) {
        std::string cfgValue = paramContainer_[inputCfg];
        switch (inputCfg) {
            case INPUT_CFG_GE_OPT_FP_POINT:
            case INPUT_CFG_GE_OPT_BP_POINT:
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
            cfgList.push_back(inputCfg);
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int GeOptParamAdapter::GetParamFromInputCfg(ProfGeOptionsConfig geCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    Init();
}

// ============================================ acl json ==================================
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

int AclApiParamAdapter::ParamsCheckAclApi(std::vector<InputCfg> &cfgList) const
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
            cfgList.push_back(inputCfg);
            flag = false;
        }
    }
    return flag ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int AclApiParamAdapter::GetParamFromInputCfg(const ProfConfig * apiCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    Init();
}

} // ParamsAdapter
} // Dvvp
} // Collector