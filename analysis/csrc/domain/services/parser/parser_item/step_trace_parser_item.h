/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_parser_item.h
 * Description        : step_trace类型二进制数据解析
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_STEP_TRACE 0x0A

#pragma pack(1)
struct StepTrace {
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t streamId : 11;
    uint16_t resv5 : 5;
    uint16_t taskId;
    uint16_t tagId;
    uint16_t resv4;
};
#pragma pack()

int StepTraceParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
}

#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STEP_TRACE_PARSER_ITEM_H
