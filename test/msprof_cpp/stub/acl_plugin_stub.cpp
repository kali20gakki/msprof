/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: stub file for acl api
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-08-27
 */
#include "acl_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {

bool AclPlugin::IsFuncExist(const std::string &funcName) const
{
    return true;
}

int32_t AclPlugin::MsprofAclrtGetLogicDevIdByUserDevId(int32_t userDevid, int32_t *logicDevId)
{
    *logicDevId = 0;
    return PROFILING_SUCCESS;
}

int32_t AclPlugin::MsprofAclrtProfTrace(uint64_t indexId, uint64_t modelId, uint16_t tagId, aclrtStream stream)
{
    return PROFILING_SUCCESS;
}

}
}
}