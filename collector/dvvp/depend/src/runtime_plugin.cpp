/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "runtime_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> RuntimePlugin::pluginHandle_ = nullptr;

void RuntimePlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<int32_t, int32_t, int32_t*>("rtGetVisibleDeviceIdByLogicDeviceId",
        rtGetVisibleDeviceIdByLogicDeviceIdFunc_);
}

void RuntimePlugin::LoadDriverSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        ret = pluginHandle_->OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PROFILING_SUCCESS) {
            return;
        }
    }
    GetAllFunction();
}

bool RuntimePlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

int32_t RuntimePlugin::MsprofRtGetVisibleDeviceIdByLogicDeviceId(int32_t logicDeviceId,
    int32_t* visibleDeviceId)
{
    PthreadOnce(&loadFlag_, []()->void {RuntimePlugin::instance()->LoadDriverSo();});
    if (rtGetVisibleDeviceIdByLogicDeviceIdFunc_ == nullptr) {
        MSPROF_LOGW("RuntimePlugin rtGetVisibleDeviceIdByLogicDeviceId function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return rtGetVisibleDeviceIdByLogicDeviceIdFunc_(logicDeviceId, visibleDeviceId);
}
}
}
}