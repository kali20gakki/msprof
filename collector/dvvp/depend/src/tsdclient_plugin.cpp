/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: tsd client plugin interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-07-22
 */
#include "tsdclient_plugin.h"
#include "msprof_dlog.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using namespace analysis::dvvp::common::error;

SHARED_PTR_ALIA<PluginHandle> TsdClientPlugin::pluginHandle_ = nullptr;

void TsdClientPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<uint32_t, uint32_t, int32_t, uint64_t>(
        "TsdCapabilityGet", tsdCapabilityGetFunc_);
    pluginHandle_->GetFunction<uint32_t, uint32_t, ProcOpenArgs *>(
        "TsdProcessOpen", tsdProcessOpenFunc_);
    pluginHandle_->GetFunction<uint32_t, uint32_t, ProcStatusParam *, uint32_t>(
        "TsdGetProcListStatus", tsdGetProcListStatusFunc_);
    pluginHandle_->GetFunction<uint32_t, uint32_t, ProcStatusParam *, uint32_t>(
        "ProcessCloseSubProcList", processCloseSubProcListFunc_);
}

void TsdClientPlugin::LoadTsdClientSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            MSPROF_LOGE("TsdClientPlugin failed to load tsdclient so");
            return;
        }

        if (!isSoValid) {
            MSPROF_LOGW("tsdclient so it may not be secure because there are incorrect permissions");
        }
    }
    GetAllFunction();
}

bool TsdClientPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

int TsdClientPlugin::MsprofTsdCapabilityGet(uint32_t logicDeviceId, int32_t type, uint64_t ptr)
{
    PthreadOnce(&loadFlag_, []()->void {TsdClientPlugin::instance()->LoadTsdClientSo();});
    if (tsdCapabilityGetFunc_ == nullptr) {
        MSPROF_LOGW("TsdClientPlugin TsdCapabilityGet function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return tsdCapabilityGetFunc_(logicDeviceId, type, ptr) == TSD_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int TsdClientPlugin::MsprofTsdProcessOpen(uint32_t logicDeviceId, ProcOpenArgs *openArgs)
{
    PthreadOnce(&loadFlag_, []()->void {TsdClientPlugin::instance()->LoadTsdClientSo();});
    if (tsdProcessOpenFunc_ == nullptr) {
        MSPROF_LOGW("TsdClientPlugin TsdProcessOpen function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return tsdProcessOpenFunc_(logicDeviceId, openArgs) == TSD_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int TsdClientPlugin::MsprofTsdGetProcListStatus(uint32_t logicDeviceId, ProcStatusParam *pidInfo, uint32_t arrayLen)
{
    PthreadOnce(&loadFlag_, []()->void {TsdClientPlugin::instance()->LoadTsdClientSo();});
    if (tsdGetProcListStatusFunc_ == nullptr) {
        MSPROF_LOGW("TsdClientPlugin TsdGetProcListStatus function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return tsdGetProcListStatusFunc_(logicDeviceId, pidInfo, arrayLen) == TSD_OK ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int TsdClientPlugin::MsprofProcessCloseSubProcList(uint32_t logicDeviceId, ProcStatusParam *closeList,
                                                   uint32_t listSize)
{
    PthreadOnce(&loadFlag_, []()->void {TsdClientPlugin::instance()->LoadTsdClientSo();});
    if (processCloseSubProcListFunc_ == nullptr) {
        MSPROF_LOGW("TsdClientPlugin ProcessCloseSubProcList function is null.");
        return PROFILING_NOTSUPPORT;
    }
    return processCloseSubProcListFunc_(logicDeviceId, closeList, listSize) == TSD_OK ?
        PROFILING_SUCCESS : PROFILING_FAILED;
}

}
}
}