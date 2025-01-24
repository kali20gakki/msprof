
/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_error_code.h
 * Description        : 解析二进制数据错误码
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_ERROR_CODE_H
#define ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_ERROR_CODE_H

#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
constexpr uint32_t DEVICE_CONTEXT_OPEN_DIR_ERROR = ERROR_NO(SERVICE_ID_CONTEXT, 0, 0x1);
}
#endif // ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_ERROR_CODE_H
