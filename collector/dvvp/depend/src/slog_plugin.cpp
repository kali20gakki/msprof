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
#include "slog_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> SlogPlugin::pluginHandle_ = nullptr;

void SlogPlugin::LoadSlogSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            return;
        }
    }
    pluginHandle_->GetFunction<int, int, int>("CheckLogLevelForC", checkLogLevelForC_);
    dlogInnerForC_ = pluginHandle_->GetFunctionForC("DlogInnerForC");
}

bool SlogPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

// CheckLogLevelForC
int SlogPlugin::MsprofCheckLogLevelForC(int moduleId, int logLevel)
{
    PthreadOnce(&loadFlag_, []()->void {SlogPlugin::instance()->LoadSlogSo();});
    if (checkLogLevelForC_ == nullptr) {
        return -1;
    }
    return checkLogLevelForC_(moduleId, logLevel);
}
} // Plugin
} // Dvvp
} // Collector