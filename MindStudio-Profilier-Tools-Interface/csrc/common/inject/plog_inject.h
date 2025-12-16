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
 * -------------------------------------------------------------------------
*/
#ifndef MSPTI_COMMON_INJECT_PLOG_INJECT_H
#define MSPTI_COMMON_INJECT_PLOG_INJECT_H

#include <functional>
#include "csrc/common/function_loader.h"
#include "csrc/common/inject/inject_base.h"

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

int CheckLogLevelForC(int moduleId, int level);

template<typename... T>
void DlogInnerForC(int moduleId, int level, const char *fmt, T... args)
{
    using dlogInnerForCFunc = std::function<void(int, int, const char*, T...)>;
    static dlogInnerForCFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction("libascendalog", "DlogInnerForC", func);
    }
    if (func) {
        return func(moduleId, level, fmt, args...);
    }
}

#endif
