#include "runtime_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

rtError_t RtPlugin::MsprofRtRegDeviceStateCallback(const char_t *regName, rtDeviceStateCallback callback)
{
    MSPROF_RTREGDEVICESTATECALLBACK_T func;
    Status ret = pluginManager_.GetFunction<const char_t *, rtDeviceStateCallback>("rtRegDeviceStateCallback", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(regName, callback);
}

rtError_t MsprofRtSetMsprofReporterCallback(MsprofReporterCallback callback)
{
    MSPROF_RTSETMSPROFREPORTERCALLBACK_T func;
    Status ret = pluginManager_.GetFunction<MsprofReporterCallback>("rtSetMsprofReporterCallback", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(callback);
}

rtError_t MsprofRtGetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t *deviceId)
{
    MSPROF_RTGETDEVICEIDBYGEMODELIDX_T func;
    Status ret = pluginManager_.GetFunction<uint32_t, uint32_t *>("rtGetDeviceIdByGeModelIdx", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(geModelIdx, deviceId);
}

rtError_t MsprofRtProfSetProSwitch(void *data, uint32_t len)
{
    MSPROF_RTPROFSETPROFSWITCH_T func;
    Status ret = pluginManager_.GetFunction<void *, uint32_t>("rtProfSetProSwitch", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(data, len);
}

} // Plugin
} // Dvvp
} // Analysis
