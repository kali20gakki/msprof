/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: handle params from user's input config
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-08-10
 */

#include "param_adapter_impl.h"

namespace Collector {
namespace Dvvp {
namespace ParamAdapter {


SHARED_PTR_ALIA<ProfileParams> MsprofParamAdapter::GetParamFromInputCfg(std::vector<std::pair<MsprofArgsType, MsprofCmdInfo>> msprofCfg)
{
    // [1] 黑名单校验；

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

SHARED_PTR_ALIA<ProfileParams> AclJsonParamAdapter::GetParamFromInputCfg(ProfAclConfig aclCfg)
{
    // [1] 黑名单校验；

    // [2] 默认值开启还是关闭（用户层面）

    // [3] 参数转换（补全表）

// ---------------------------- 表格的每一个参数都是确认值（用户层面） --------------------

    // [4] 私有参数校验（派生类实现）

    // [5] 公有参数校验（调基类接口）

    // [6] 参数转换，转成Params（软件栈转uint64_t， 非软件栈保留在Params）

    // 报错打印（MSPROF_LOG）

}

SHARED_PTR_ALIA<ProfileParams> GeOptParamAdapter::GetParamFromInputCfg(ProfGeOptionsConfig geCfg)
{

}

SHARED_PTR_ALIA<ProfileParams> AclApiParamAdapter::GetParamFromInputCfg(const ProfConfig * apiCfg)
{
    
}

} // ParamAdapter
} // Dvvp
} // Collector