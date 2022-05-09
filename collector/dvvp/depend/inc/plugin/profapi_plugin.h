/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: profapi interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef PROFAPI_PLUGIN_H
#define PROFAPI_PLUGIN_H

#include "singleton/singleton.h"
#include "prof_api.h"
#include "plugin_handle.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
class ProfApiPlugin : public analysis::dvvp::common::singleton::Singleton<ProfApiPlugin> {
public:
    ProfApiPlugin()
     : soName_("libprofapi.so"),
       pluginHandle_(PluginHandle(soName_))
    {}
    ~ProfApiPlugin();

    bool IsFuncExist(const std::string &funcName) const;

    // profRegReporterCallback
    using MSPROF_PROFREGREPORTERCALLBACK_T = std::function<int32_t(ProfReportHandle)>;
    int32_t MsprofProfRegReporterCallback(ProfReportHandle reporter);

    // profRegCtrlCallback
    using MSPROF_PROFREGCTRLCALLBACK_T = std::function<int32_t(ProfCtrlHandle)>;
    int32_t MsprofProfRegCtrlCallback(ProfCtrlHandle handle);

    // profRegDeviceStateCallback
    using MSPROF_PROFREGDEVICESTATECALLBACK_T = std::function<int32_t(ProfSetDeviceHandle)>;
    int32_t MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle);

    // profGetDeviceIdByGeModelIdx
    using MSPROF_PROFGETDEVICEIDBYGEMODELIDX_T = std::function<int32_t(const uint32_t, uint32_t *)>;
    int32_t MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId);

    // profSetProfCommand
    using MSPROF_PROFSETPROFCOMMAND_T = std::function<int32_t(PROFAPI_PROF_COMMAND_PTR, uint32_t)>;
    int32_t MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len);

    // profSetStepInfo
    using MSPROF_PROFSETSTEPINFO_T = std::function<int32_t(const uint64_t, const uint16_t, void* const)>;
    int32_t MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream);

private:
    std::string soName_;
    PluginHandle pluginHandle_;
};
} // Plugin
} // Dvvp
} // Analysis
#endif