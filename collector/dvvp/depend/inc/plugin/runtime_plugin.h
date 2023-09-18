/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: runtime interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-08-29
 */
#ifndef RUNTIME_PLUGIN_H
#define RUNTIME_PLUGIN_H

#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using RtGetVisibleDeviceIdByLogicDeviceIdFunc = std::function<int32_t(int32_t, int32_t*)>;
class RuntimePlugin : public analysis::dvvp::common::singleton::Singleton<RuntimePlugin> {
public:
    RuntimePlugin() : soName_("libruntime.so"), loadFlag_(0) {}
    bool IsFuncExist(const std::string &funcName) const;
    void GetAllFunction();
    int32_t MsprofRtGetVisibleDeviceIdByLogicDeviceId(int32_t logicDeviceId, int32_t* visibleDeviceId);
private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    RtGetVisibleDeviceIdByLogicDeviceIdFunc rtGetVisibleDeviceIdByLogicDeviceIdFunc_ = nullptr;
private:
    void LoadRuntimeSo();
};
}
}
}

#endif