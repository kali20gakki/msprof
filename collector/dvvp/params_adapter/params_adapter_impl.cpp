/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */

#include "params_adapter_impl.h"

#include "errno/error_code.h"

namespace Collector {
namespace Dvvp {
namespace ParamsAdapter {
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::common::error;

int MsprofParamAdapter::Init()
{
    paramContainer_.fill("");
    int ret = CheckListInit();
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    CreateCfgMap();
}

int MsprofParamAdapter::GetParamFromInputCfg(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg,
    SHARED_PTR_ALIA<ProfileParams> params)
{
    params_ = params;
    Init(); // 派生类初始化+基类初始化
    
    // [1]黑名单校验+表参数映射
    for (auto argPair : msprofCfg) {
        MsprofArgsType argsType = argPair.first;
        InputCfg cfgType = cfgMap_[argsType];
        if (!BlackSwitchCheck(cfgType)) {
            return PROFILING_FAILED;
        }
        paramContainer_[cfgType] = argPair.second.args[argsType];
    }


    // [2] 默认值开启还是关闭（用户层面）

    // [3] 参数转换（补全表）

// ---------------------------- 表格的每一个参数都是确认值（用户层面） --------------------

    // [4] 私有参数校验（派生类实现）

    // [5] 公有参数校验（调基类接口）

    // [6] 参数转换，转成Params（软件栈转uint64_t， 非软件栈保留在Params）
    
    // 报错打印
    // analysis::dvvp::message::StatusInfo statusInfo_; 枚举值，状态，信息
    // 外部使能方式根据枚举值打印参数名 warm加信息到statusInfo_，逻辑往下走，success往下走，不加信息
    // fail 加信息+返回

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
// ===============================================================================================
int AclJsonParamAdapter::GetParamFromInputCfg(ProfAclConfig aclCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
}

int GeOptParamAdapter::GetParamFromInputCfg(ProfGeOptionsConfig geCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
}

int AclApiParamAdapter::GetParamFromInputCfg(const ProfConfig * apiCfg, SHARED_PTR_ALIA<ProfileParams> params)
{
}

} // ParamsAdapter
} // Dvvp
} // Collector