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
ProfApiPlugin::~ProfApiPlugin()
{
    pluginHandle_.CloseHandle();
}

bool ProfApiPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// profRegReporterCallback
int32_t ProfApiPlugin::MsprofProfRegReporterCallback(ProfReportHandle reporter)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFREGREPORTERCALLBACK_T func;
    ret = pluginHandle_.GetFunction<int32_t, ProfReportHandle>("profRegReporterCallback", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(reporter);
}

// profRegCtrlCallback
int32_t ProfApiPlugin::MsprofProfRegCtrlCallback(ProfCtrlHandle handle)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFREGCTRLCALLBACK_T func;
    ret = pluginHandle_.GetFunction<int32_t, ProfCtrlHandle>("profRegCtrlCallback", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(handle);
}

// profRegDeviceStateCallback
int32_t ProfApiPlugin::MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFREGDEVICESTATECALLBACK_T func;
    ret = pluginHandle_.GetFunction<int32_t, ProfSetDeviceHandle>("profRegDeviceStateCallback", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(handle);
}

// profGetDeviceIdByGeModelIdx
int32_t ProfApiPlugin::MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFGETDEVICEIDBYGEMODELIDX_T func;
    ret = pluginHandle_.GetFunction<int32_t, uint32_t, uint32_t *>("profGetDeviceIdByGeModelIdx", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(modelIdx, deviceId);
}

// profSetProfCommand
int32_t ProfApiPlugin::MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFSETPROFCOMMAND_T func;
    ret = pluginHandle_.GetFunction<int32_t, PROFAPI_PROF_COMMAND_PTR, uint32_t>("profSetProfCommand", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(command, len);
}

// profSetStepInfo
int32_t ProfApiPlugin::MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream)
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFSETSTEPINFO_T func;
    ret = pluginHandle_.GetFunction<int32_t, uint64_t, uint16_t, void*>("profSetStepInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(indexId, tagId, stream);
}
} // Plugin
} // Dvvp
} // Analysis