/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : profapi_inject.h
 * Description        : Injection of profapi.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_PROFAPI_INJECT_H
#define MSPTI_COMMON_INJECT_PROFAPI_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

MSPTI_API int32_t MsprofRegTypeInfo(uint16_t level, uint32_t typeId, const char *typeName);
MSPTI_API int32_t MsprofReportCompactInfo(uint32_t agingFlag, const void* data, uint32_t length);
MSPTI_API int32_t MsprofReportData(uint32_t moduleId, uint32_t type, void* data, uint32_t len);
#if defined(__cplusplus)
}
#endif
#endif
