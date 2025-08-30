/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: acl interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-08-27
 */
#include "acl_plugin.h"
#include "msprof_dlog.h"

using namespace analysis::dvvp::common::error;

namespace Collector {
namespace Dvvp {
namespace Plugin {
namespace {
static const int ACL_SUCCESS = 0;
struct TraceData {
    uint64_t indexId;
    uint64_t modelId;
    uint16_t tagId;
};
}
SHARED_PTR_ALIA<PluginHandle> AclPlugin::pluginHandle_ = nullptr;

void AclPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<int, int32_t, int32_t*>("aclrtGetLogicDevIdByUserDevId",
                                                       aclrtGetLogicDevIdByUserDevIdFunc_);
    pluginHandle_->GetFunction<int, void *, int32_t, aclrtStream>("aclrtProfTrace", aclrtProfTraceFunc_);
}

void AclPlugin::LoadAclSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            MSPROF_LOGE("AclPlugin failed to load acl so");
            return;
        }

        if (!isSoValid) {
            MSPROF_LOGW("acl so it may not be secure because there are incorrect permissions");
        }
    }
    GetAllFunction();
}

bool AclPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

int32_t AclPlugin::MsprofAclrtGetLogicDevIdByUserDevId(int32_t userDevId, int32_t *logicDevId)
{
    PthreadOnce(&loadFlag_, []()->void {AclPlugin::instance()->LoadAclSo();});
    if (aclrtGetLogicDevIdByUserDevIdFunc_ == nullptr) {
        MSPROF_LOGW("AclPlugin aclrtGetLogicDevIdByUserDevId function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return aclrtGetLogicDevIdByUserDevIdFunc_(userDevId, logicDevId) == ACL_SUCCESS ?
        PROFILING_SUCCESS : PROFILING_FAILED;
}

int32_t AclPlugin::MsprofAclrtProfTrace(uint64_t indexId, uint64_t modelId, uint16_t tagId, aclrtStream stream)
{
    PthreadOnce(&loadFlag_, []()->void {AclPlugin::instance()->LoadAclSo();});
    if (aclrtProfTraceFunc_ == nullptr) {
        MSPROF_LOGW("AclPlugin aclrtProfTrace function is null.");
        return PROFILING_NOTSUPPORT;
    }
    TraceData traceData{.indexId = indexId, .modelId = modelId, .tagId = tagId};
    return aclrtProfTraceFunc_((void *)&traceData, sizeof(TraceData), stream) == ACL_SUCCESS ?
       PROFILING_SUCCESS : PROFILING_FAILED;
}

}
}
}