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

namespace Collector {
namespace Dvvp {
namespace Plugin {
using ProfRegReporterCallbackFunc = std::function<int32_t(ProfReportHandle)>;              // profRegReporterCallback
using ProfRegCtrlCallbackFunc = std::function<int32_t(ProfCtrlHandle)>;                    // profRegCtrlCallback
using ProfRegDeviceStateCallbackFunc = std::function<int32_t(ProfSetDeviceHandle)>;        // profRegDeviceStateCallback
using ProfGetDeviceIdByGeModelIdxFunc = std::function<int32_t(uint32_t, uint32_t *)>;      // profGetDeviceIdByGeModelIdx
using ProfSetProfCommandFunc = std::function<int32_t(PROFAPI_PROF_COMMAND_PTR, uint32_t)>; // profSetProfCommand
using ProfSetStepInfoFunc = std::function<int32_t(uint64_t, uint16_t, void*)>;             // profSetStepInfo
class ProfApiPlugin : public analysis::dvvp::common::singleton::Singleton<ProfApiPlugin> {
public:
    ProfApiPlugin() : soName_("libprofapi.so"), pluginHandle_(PluginHandle(soName_)), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    // profRegReporterCallback
    int32_t MsprofProfRegReporterCallback(ProfReportHandle reporter);

    // profRegCtrlCallback
    int32_t MsprofProfRegCtrlCallback(ProfCtrlHandle handle);

    // profRegDeviceStateCallback
    int32_t MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle);

    // profGetDeviceIdByGeModelIdx
    int32_t MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId);

    // profSetProfCommand
    int32_t MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len);

    // profSetStepInfo
    int32_t MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream);

private:
    std::string soName_;
    PluginHandle pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    ProfRegReporterCallbackFunc profRegReporterCallback_ = nullptr;
    ProfRegCtrlCallbackFunc profRegCtrlCallback_ = nullptr;
    ProfRegDeviceStateCallbackFunc profRegDeviceStateCallback_ = nullptr;
    ProfGetDeviceIdByGeModelIdxFunc profGetDeviceIdByGeModelIdx_ = nullptr;
    ProfSetProfCommandFunc profSetProfCommand_ = nullptr;
    ProfSetStepInfoFunc profSetStepInfo_ = nullptr;

private:
    void LoadProfApiSo();
};
} // Plugin
} // Dvvp
} // Collector
#endif
