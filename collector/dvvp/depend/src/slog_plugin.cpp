/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: slog interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "slog_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
void SlogPlugin::LoadSlogSo()
{
    PluginStatus ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
    }
}

bool SlogPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// CheckLogLevelForC
int SlogPlugin::MsprofCheckLogLevelForC(int moduleId, int logLevel)
{
    PthreadOnce(&loadFlag_, []()->void {SlogPlugin::instance()->LoadSlogSo();});
    if (checkLogLevelForC_ == nullptr) {
        PluginStatus ret = pluginHandle_.GetFunction<int, int, int>("CheckLogLevelForC", checkLogLevelForC_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    return checkLogLevelForC_(moduleId, logLevel);
}
} // Plugin
} // Dvvp
} // Collector