/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.h
 * Description        : Injection of mstx.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H
#define MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

#define RT_STREAM void*

#if defined(__cplusplus)
extern "C" {
#endif

MSPTI_API void aclprofMarkEx(const char* message, size_t len, RT_STREAM stream);

MSPTI_API void aclprofMark(void *stamp);

#if defined(__cplusplus)
}
#endif

#endif
