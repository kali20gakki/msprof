/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: slog interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "slog_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
SlogPlugin::~SlogPlugin()
{
    pluginHandle_.CloseHandle();
}

bool SlogPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// CheckLogLevelForC
int SlogPlugin::MsprofCheckLogLevelForC(int moduleId, int logLevel)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");;
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_CHECKLOGLEVELFORC_T func;
    ret = pluginHandle_.GetFunction<int, int, int>("CheckLogLevelForC", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(moduleId, logLevel);
}
} // Plugin
} // Dvvp
} // Analysis