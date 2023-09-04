/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: stub file for runtime api
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-09-01
 */
#include "runtime_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {

bool RuntimePlugin::IsFuncExist(const std::string &funcName) const
{
    return true;
}

int32_t RuntimePlugin::MsprofRtGetVisibleDeviceIdByLogicDeviceId(int32_t logicDeviceId,
    int32_t* visibleDeviceId)
{
    *visibleDeviceId = 0;
    return PROFILING_SUCCESS;
}

}
}
}