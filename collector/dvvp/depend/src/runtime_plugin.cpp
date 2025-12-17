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
#include "runtime_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
static const int32_t RT_ERROR_NONE = 0;
SHARED_PTR_ALIA<PluginHandle> RuntimePlugin::pluginHandle_ = nullptr;

void RuntimePlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<int32_t, int32_t, int32_t*>("rtGetVisibleDeviceIdByLogicDeviceId",
        rtGetVisibleDeviceIdByLogicDeviceIdFunc_);
    pluginHandle_->GetFunction<int32_t, uint64_t, uint64_t, uint16_t, aclrtStream>("rtProfilerTraceEx",
        rtProfilerTraceExFunc_);
}

void RuntimePlugin::LoadRuntimeSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            MSPROF_LOGE("RuntimePlugin failed to load runtime so");
            return;
        }

        if (!isSoValid) {
            MSPROF_LOGW("runtime so it may not be secure because there are incorrect permissions");
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
    PthreadOnce(&loadFlag_, []()->void {RuntimePlugin::instance()->LoadRuntimeSo();});
    if (rtGetVisibleDeviceIdByLogicDeviceIdFunc_ == nullptr) {
        MSPROF_LOGW("RuntimePlugin rtGetVisibleDeviceIdByLogicDeviceId function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return rtGetVisibleDeviceIdByLogicDeviceIdFunc_(logicDeviceId, visibleDeviceId) == RT_ERROR_NONE ?
        PROFILING_SUCCESS : PROFILING_FAILED;
}

int32_t RuntimePlugin::MsprofRtProfilerTraceEx(uint64_t indexId, uint64_t modelId, uint16_t tagId, aclrtStream stream)
{
    PthreadOnce(&loadFlag_, []()->void {RuntimePlugin::instance()->LoadRuntimeSo();});
    if (rtProfilerTraceExFunc_ == nullptr) {
        MSPROF_LOGW("RuntimePlugin rtProfilerTraceEx function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return rtProfilerTraceExFunc_(indexId, modelId, tagId, stream) == RT_ERROR_NONE ?
        PROFILING_SUCCESS : PROFILING_FAILED;
}
}
}
}