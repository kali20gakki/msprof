/**
* @file ai_drv_dsmi_api.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "ai_drv_dsmi_api.h"
#include <cerrno>
#include <map>
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "config/config_manager.h"
#include "driver_plugin.h"
namespace Analysis {
namespace Dvvp {
namespace Driver {
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::Plugin;

int DrvGetAicoreInfo(int deviceId, int64_t &freq)
{
    if (deviceId < 0) {
        return PROFILING_FAILED;
    }
    return DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId, MODULE_TYPE_AICORE, INFO_TYPE_FREQUE, &freq);
}

std::string DrvGeAicFrq(int deviceId)
{
    const std::string defAicFrq = ConfigManager::instance()->GetAicDefFrequency();
    if (deviceId < 0) {
        return defAicFrq;
    }

    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        return defAicFrq;
    }
    int64_t freq = 0;
    int ret = DrvGetAicoreInfo(deviceId, freq);
    if (ret != PROFILING_SUCCESS || freq == 0) {
        MSPROF_LOGW("DrvGetAicoreInfo failed, ret:%d", ret);
        return defAicFrq;
    }

    MSPROF_LOGI("DrvGetAicoreInfo curFreq %u", freq);
    return std::to_string(freq);
}
}}}
