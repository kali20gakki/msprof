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
