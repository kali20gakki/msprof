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
#include "csrc/common/inject/plog_inject.h"
#include "csrc/common/utils.h"

namespace {
enum PlogFunctionIndex {
    FUNC_CHECK_LOG_LEVEL_FOR_C,
    FUNC_DLOG_INNER_FOR_C,
    FUNC_PLOG_COUNT
};

pthread_once_t g_once = PTHREAD_ONCE_INIT;
void* g_plogFuncArray[FUNC_PLOG_COUNT];

void LoadPlogFunction()
{
    g_plogFuncArray[FUNC_CHECK_LOG_LEVEL_FOR_C] = Mspti::Common::RegisterFunction("libascendalog", "CheckLogLevelForC");
    g_plogFuncArray[FUNC_DLOG_INNER_FOR_C] = Mspti::Common::RegisterFunction("libascendalog", "DlogInnerForC");
}
}

int CheckLogLevelForC(int moduleId, int level)
{
    pthread_once(&g_once, LoadPlogFunction);
    void* voidFunc = g_plogFuncArray[FUNC_CHECK_LOG_LEVEL_FOR_C];
    using checkLogLevelForCFunc = std::function<decltype(CheckLogLevelForC)>;
    checkLogLevelForCFunc func = Mspti::Common::ReinterpretConvert<decltype(&CheckLogLevelForC)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libascendalog", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libascendalog.so");
    return func(moduleId, level);
}
