/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : prof_mstx.cpp
 * Description        : 存储转发mstx实际init方法
 * Author             : msprof team
 * Creation Date      : 2025/08/07
 * *****************************************************************************
*/

#ifndef COMMON_MSTX_PROF_MSTX_H
#define COMMON_MSTX_PROF_MSTX_H

#include "external/prof_common.h"
#include "mstx_impl.h"

#if defined(__cplusplus)
extern "C" {
#endif

__attribute__((visibility("default"))) int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable);

#if defined(__cplusplus)
}
#endif

#endif
