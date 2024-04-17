/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_gc.h
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_BASE_GC_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_BASE_GC_H
#include <cstdint>

namespace Analysis {

namespace Ps {

class BaseGc {
public:
    BaseGc() = default;
    virtual ~BaseGc() = default;
protected:
    void CommonFun();
};

uint32_t GetBaseGcCount();
void ResetBaseGcCount();
}

}

#endif