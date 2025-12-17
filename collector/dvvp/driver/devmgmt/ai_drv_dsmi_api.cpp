/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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
