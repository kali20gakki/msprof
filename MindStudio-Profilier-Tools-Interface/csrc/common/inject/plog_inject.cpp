/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : plog_inject.cpp
 * Description        : Injection of plog.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
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
