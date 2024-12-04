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
#include "common/inject/plog_inject.h"
#include "common/utils.h"
enum PlogFunctionIndex {
    FUNC_CHECK_LOG_LEVEL_FOR_C,
    FUNC_DLOG_INNER_FOR_C,
    FUNC_PLOG_COUNT
};

void* g_plogFuncArray[FUNC_PLOG_COUNT] = {
    Mspti::Common::RegisterFunction("libascendalog", "CheckLogLevelForC"),
    Mspti::Common::RegisterFunction("libascendalog", "DlogInnerForC")
};

int CheckLogLevelForC(int moduleId, int level)
{
    void* voidFunc = g_plogFuncArray[FUNC_CHECK_LOG_LEVEL_FOR_C];
    using checkLogLevelForCFunc = std::function<int(int, int)>;
    checkLogLevelForCFunc func = Mspti::Common::ReinterpretConvert<int (*)(int, int)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, int, int>("libascendalog", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libascendalog.so");
    return func(moduleId, level);
}
