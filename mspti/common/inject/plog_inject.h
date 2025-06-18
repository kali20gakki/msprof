/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : plog_inject.h
 * Description        : Injection of plog.
 * Author             : msprof team
 * Creation Date      : 2024/05/31
 * *****************************************************************************
*/
#ifndef MSPTI_COMMON_INJECT_PLOG_INJECT_H
#define MSPTI_COMMON_INJECT_PLOG_INJECT_H

#include <functional>
#include "common/function_loader.h"
#include "common/inject/inject_base.h"

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
