/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: slog interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef SLOG_PLUGIN_H
#define SLOG_PLUGIN_H

#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {
enum {
    DLOG_DEBUG = 0,
    DLOG_INFO,
    DLOG_WARN,
    DLOG_ERROR,
    DLOG_NULL,
    DLOG_TRACE,
    DLOG_OPLOG,
    DLOG_EVENT = 0x10
};

class SlogPlugin : public analysis::dvvp::common::singleton::Singleton<SlogPlugin> {
public:
    SlogPlugin()
    : soName_("libalog.so"),
      pluginHandle_(PluginHandle(soName_))
    {}
    ~SlogPlugin();

    bool IsFuncExist(const std::string &funcName) const;

    // CheckLogLevelForC
    using MSPROF_CHECKLOGLEVELFORC_T = std::function<int(int, int)>;
    int MsprofCheckLogLevelForC(int moduleId, int logLevel);

    // DlogInnerForC
    template<typename... T>
    void MsprofDlogInnerForC(int moduleId, int level, const char *fmt, T... args)
    {
        Status ret = PLUGIN_LOAD_SUCCESS;
        if (!pluginHandle_.HasLoad()) {
            ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
            if (ret != PLUGIN_LOAD_SUCCESS) {
                return;
            }
        }
        using MSPROF_DLOGINNERFORC_T = std::function<void(int, int, const char *, T...)>;
        MSPROF_DLOGINNERFORC_T func;
        ret = pluginHandle_.GetFunction<void, int, int, const char *, T...>("DlogInnerForC", func);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return;
        }
        func(moduleId, level, fmt, args...);
    }

private:
    std::string soName_;
    PluginHandle pluginHandle_;
};
} // Plugin
} // Dvvp
} // Analysis

#endif