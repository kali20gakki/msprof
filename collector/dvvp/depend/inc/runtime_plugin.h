#ifndef RUNTIME_PLUGIN_H
#define RUNTIME_PLUGIN_H

#include "singleton/singleton.h"
#include "runtime/base.h"
#include "plugin_manager.h"

/* Msprof调用 ge_executor.so 使用的函数原型  
*  ProfSetStepInfo
*/
namespace Analysis {
namespace Dvvp {
namespace Plugin {

class RtPlugin : public analysis::dvvp::common::singleton::Singleton<RtPlugin> {
public:
    RtPlugin()
     :load_(false),
     :soName_("libge_executor.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~RtPlugin() = default;

    // rtRegDeviceStateCallback
    // rtError_t rtRegDeviceStateCallback(const char_t *regName, rtDeviceStateCallback callback);
    using MSPROF_RTREGDEVICESTATECALLBACK_T = rtError_t (*)(const char_t *, rtDeviceStateCallback);
    rtError_t MsprofRtRegDeviceStateCallback(const char_t *regName, rtDeviceStateCallback callback);

    // rtSetMsprofReporterCallback
    // rtError_t rtSetMsprofReporterCallback(MsprofReporterCallback callback);
    using MSPROF_RTSETMSPROFREPORTERCALLBACK_T = rtError_t (*)(MsprofReporterCallback);
    rtError_t MsprofRtSetMsprofReporterCallback(MsprofReporterCallback callback);

    // rtGetDeviceIdByGeModelIdx
    // rtError_t rtGetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t *deviceId);
    using MSPROF_RTGETDEVICEIDBYGEMODELIDX_T = rtError_t (*)(uint32_t, uint32_t *);
    rtError_t MsprofRtGetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t *deviceId);

    // rtProfSetProSwitch
    // rtError_t rtProfSetProSwitch(void *data, uint32_t len);
    using MSPROF_RTPROFSETPROFSWITCH_T = rtError_t (*)(void *, uint32_t);
    rtError_t MsprofRtProfSetProSwitch(void *data, uint32_t len);

private:
    bool load_;
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis

#endif