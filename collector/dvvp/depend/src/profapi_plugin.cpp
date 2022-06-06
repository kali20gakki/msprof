/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: profapi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "profapi_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
void ProfApiPlugin::LoadProfApiSo()
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
    }
}

bool ProfApiPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// profRegReporterCallback
int32_t ProfApiPlugin::MsprofProfRegReporterCallback(ProfReportHandle reporter)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegReporterCallback_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, ProfReportHandle>("profRegReporterCallback",
            profRegReporterCallback_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profRegReporterCallback_(reporter);
}

// profRegCtrlCallback
int32_t ProfApiPlugin::MsprofProfRegCtrlCallback(ProfCtrlHandle handle)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegCtrlCallback_  == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, ProfCtrlHandle>("profRegCtrlCallback",
            profRegCtrlCallback_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profRegCtrlCallback_(handle);
}

// profRegDeviceStateCallback
int32_t ProfApiPlugin::MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profRegDeviceStateCallback_  == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, ProfSetDeviceHandle>("profRegDeviceStateCallback",
            profRegDeviceStateCallback_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profRegDeviceStateCallback_(handle);
}

// profGetDeviceIdByGeModelIdx
int32_t ProfApiPlugin::MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profGetDeviceIdByGeModelIdx_   == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, uint32_t, uint32_t *>("profGetDeviceIdByGeModelIdx",
            profGetDeviceIdByGeModelIdx_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profGetDeviceIdByGeModelIdx_(modelIdx, deviceId);
}

// profSetProfCommand
int32_t ProfApiPlugin::MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profSetProfCommand_  == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, PROFAPI_PROF_COMMAND_PTR, uint32_t>("profSetProfCommand",
            profSetProfCommand_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profSetProfCommand_(command, len);
}

// profSetStepInfo
int32_t ProfApiPlugin::MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream)
{
    PthreadOnce(&loadFlag_, []()->void {ProfApiPlugin::instance()->LoadProfApiSo();});
    if (profSetStepInfo_  == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int32_t, uint64_t, uint16_t, void*>("profSetStepInfo",
            profSetStepInfo_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return profSetStepInfo_(indexId, tagId, stream);
}
} // Plugin
} // Dvvp
} // Analysis