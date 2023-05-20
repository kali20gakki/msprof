/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: profapi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "profapi_plugin.h"
#include "msprof_dlog.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> ProfApiPlugin::pluginHandle_ = nullptr;

void ProfApiPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<int32_t, ProfReportHandle>("profRegReporterCallback", profRegReporterCallback_);
    pluginHandle_->GetFunction<int32_t, int32_t, VOID_PTR, uint32_t>("MsprofRegisterProfileCallback",
        profRegProfilerCallback_);
    pluginHandle_->GetFunction<int32_t, ProfCtrlHandle>("profRegCtrlCallback", profRegCtrlCallback_);
    pluginHandle_->GetFunction<int32_t, ProfSetDeviceHandle>("profRegDeviceStateCallback", profRegDeviceStateCallback_);
    pluginHandle_->GetFunction<int32_t, uint32_t, uint32_t *>(
        "profGetDeviceIdByGeModelIdx", profGetDeviceIdByGeModelIdx_);
    pluginHandle_->GetFunction<int32_t, PROFAPI_PROF_COMMAND_PTR, uint32_t>("profSetProfCommand", profSetProfCommand_);
    pluginHandle_->GetFunction<int32_t, uint64_t, uint16_t, void*>("profSetStepInfo", profSetStepInfo_);
}

void ProfApiPlugin::LoadProfApiSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        ret = pluginHandle_->OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[ProfApiPlugin]LoadProfApiSo OpenPlugin for LD_LIBRARY_PATH failed");
            return;
        }
    }
    GetAllFunction();
}

bool ProfApiPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

// profRegReporterCallback
int32_t ProfApiPlugin::MsprofProfRegReporterCallback(ProfReportHandle reporter)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegReporterCallback_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profRegReporterCallback function is null.");
        return PROFILING_FAILED;
    }
    return profRegReporterCallback_(reporter);
}

// profRegProfilerCallback
int32_t ProfApiPlugin::MsprofProfRegProfilerCallback(int32_t callbackType, VOID_PTR callback, uint32_t len)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegProfilerCallback_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profRegProfilerCallback function is null.");
        return PROFILING_FAILED;
    }
    return profRegProfilerCallback_(callbackType, callback, len);
}

// profRegCtrlCallback
int32_t ProfApiPlugin::MsprofProfRegCtrlCallback(ProfCtrlHandle handle)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegCtrlCallback_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profRegCtrlCallback function is null.");
        return PROFILING_FAILED;
    }
    return profRegCtrlCallback_(handle);
}

// profRegDeviceStateCallback
int32_t ProfApiPlugin::MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegDeviceStateCallback_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profRegDeviceStateCallback function is null.");
        return PROFILING_FAILED;
    }
    return profRegDeviceStateCallback_(handle);
}

// profGetDeviceIdByGeModelIdx
int32_t ProfApiPlugin::MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profGetDeviceIdByGeModelIdx_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profGetDeviceIdByGeModelIdx function is null.");
        return PROFILING_FAILED;
    }
    return profGetDeviceIdByGeModelIdx_(modelIdx, deviceId);
}

// profSetProfCommand
int32_t ProfApiPlugin::MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profSetProfCommand_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profSetProfCommand function is null.");
        return PROFILING_FAILED;
    }
    return profSetProfCommand_(command, len);
}

// profSetStepInfo
int32_t ProfApiPlugin::MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profSetStepInfo_ == nullptr) {
        MSPROF_LOGE("ProfApiPlugin profSetStepInfo function is null.");
        return PROFILING_FAILED;
    }
    return profSetStepInfo_(indexId, tagId, stream);
}
} // Plugin
} // Dvvp
} // Collector