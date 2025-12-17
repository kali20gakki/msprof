/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef RUNTIME_PLUGIN_H
#define RUNTIME_PLUGIN_H

#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using aclrtStream = void *;
using RtGetVisibleDeviceIdByLogicDeviceIdFunc = std::function<int32_t(int32_t, int32_t*)>;
using RtProfilerTraceExFunc = std::function<int32_t(uint64_t, uint64_t, uint16_t, aclrtStream)>;
class RuntimePlugin : public analysis::dvvp::common::singleton::Singleton<RuntimePlugin> {
public:
    RuntimePlugin() : soName_("libruntime.so"), loadFlag_(0) {}
    bool IsFuncExist(const std::string &funcName) const;
    void GetAllFunction();
    int32_t MsprofRtGetVisibleDeviceIdByLogicDeviceId(int32_t logicDeviceId, int32_t* visibleDeviceId);
    int32_t MsprofRtProfilerTraceEx(uint64_t indexId, uint64_t modelId, uint16_t tagId, aclrtStream stream);
private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    RtGetVisibleDeviceIdByLogicDeviceIdFunc rtGetVisibleDeviceIdByLogicDeviceIdFunc_ = nullptr;
    RtProfilerTraceExFunc rtProfilerTraceExFunc_ = nullptr;
private:
    void LoadRuntimeSo();
};
}
}
}

#endif