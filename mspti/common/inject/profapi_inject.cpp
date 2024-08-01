/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : profapi_inject.cpp
 * Description        : Injection of profapi.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "common/inject/profapi_inject.h"

#include <functional>

#include "common/function_loader.h"
#include "common/plog_manager.h"

int32_t MsprofRegTypeInfo(uint16_t level, uint32_t typeId, const char *typeName)
{
    return 0;
}

int32_t MsprofReportCompactInfo(uint32_t agingFlag, const VOID_PTR data, uint32_t length)
{
    return 0;
}

int32_t MsprofReportData(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    return 0;
}
