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

class PlogInject {
public:
    PlogInject() noexcept
    {
        Mspti::Common::RegisterFunction("libascendalog", "CheckLogLevelForC");
        Mspti::Common::RegisterFunction("libascendalog", "DlogInnerForC");
    }
    ~PlogInject() = default;
};

PlogInject g_plogInject;

int CheckLogLevelForC(int moduleId, int level)
{
    using checkLogLevelForCFunc = std::function<int(int, int)>;
    static checkLogLevelForCFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, int, int>("libascendalog", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libascendalog.so");
    return func(moduleId, level);
}
