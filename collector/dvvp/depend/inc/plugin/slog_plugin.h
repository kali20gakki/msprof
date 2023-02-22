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

namespace Collector {
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

using CheckLogLevelForCFunc = std::function<int(int, int)>;
class SlogPlugin : public analysis::dvvp::common::singleton::Singleton<SlogPlugin> {
public:
    SlogPlugin() : soName_("libascendalog.so"), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    // CheckLogLevelForC
    int MsprofCheckLogLevelForC(int moduleId, int logLevel);

    // DlogInnerForC
    template<typename... T>
    void MsprofDlogInnerForC(int moduleId, int level, const char *fmt, T... args)
    {
        if (dlogInnerForC_ == nullptr) {
            return;
        }
        using DlogInnerForCFunc = std::function<void(int, int, const char *, T...)>;
        static DlogInnerForCFunc func = (void(*)(int, int, const char *, T...))dlogInnerForC_;
        if (func != nullptr) {
            func(moduleId, level, fmt, args...);
        }
    }

private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    CheckLogLevelForCFunc checkLogLevelForC_ = nullptr;
    HandleType dlogInnerForC_ = nullptr;

private:
    void LoadSlogSo();
};
} // Plugin
} // Dvvp
} // Collector

#endif
