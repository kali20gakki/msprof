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
