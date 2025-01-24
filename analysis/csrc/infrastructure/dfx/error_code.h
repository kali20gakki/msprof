/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : error_code.h
 * Description        : 错误码
 * Author             : msprof team
 * Creation Date      : 2023/11/8
 * *****************************************************************************
 */

#ifndef ANALYSIS_DFX_ERROR_CODE_H
#define ANALYSIS_DFX_ERROR_CODE_H

#include <cstdint>

namespace Analysis {

const int ANALYSIS_OK = 0;
const int ANALYSIS_ERROR = 1;

const int SERVICE_OFFSET = 24;
const int SEQUENCE_OFFSET = 16;
// define Service id
constexpr uint32_t SERVICE_ID_CONTEXT  = 0x30;   // Context Service
constexpr uint32_t SERVICE_ID_PARSER  = 0x31;   // Parser Service
constexpr uint32_t SERVICE_ID_MODELING  = 0x32;   // Modeling Service
constexpr uint32_t SERVICE_ID_ASSOCIATION  = 0x33;   // association Service
constexpr uint32_t SERVICE_ID_PERSISTENCE  = 0x34;   // persistence Service

// 第1个8bit为服务/组件ID,第2个8bit为服务/组件内部细分序号，后16个bit为错误码
constexpr uint32_t ERROR_NO(const uint32_t serviceId, const uint32_t seq, const uint32_t error_code)
{
    return ((serviceId << SERVICE_OFFSET) | (seq << SEQUENCE_OFFSET) | (error_code));
}

}  // namespace Analysis
#endif // ANALYSIS_DFX_ERROR_CODE_H
