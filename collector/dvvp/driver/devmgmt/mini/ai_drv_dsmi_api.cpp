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
#include "config/config_manager.h"
namespace Analysis {
namespace Dvvp {
namespace Driver {
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::error;

int DrvGetAicoreInfo(int deviceId, int64_t &freq)
{
    if (deviceId < 0) {
        return PROFILING_FAILED;
    }
    MSPROF_LOGD("DrvGetAicoreInfo Freq %u", freq);
    return PROFILING_SUCCESS;
}

std::string DrvGeAicFrq(int deviceId)
{
    MSPROF_LOGD("DrvGeAicFrq devId %d", deviceId);
    const std::string defAicFrq = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetAicDefFrequency();
    return defAicFrq;
}
}}}
