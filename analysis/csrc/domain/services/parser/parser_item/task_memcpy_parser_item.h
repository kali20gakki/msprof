/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_flip_parser_item.cpp
 * Description        : taskFlip解析数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_MEMCPY_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_MEMCPY_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_TS_MEMCPY 0x0B

#pragma pack(1)
struct TaskMemcpy {
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t streamId : 11;
    uint16_t resv4 : 5;
    uint16_t taskId;
    uint8_t taskStatus;
    uint8_t resv5;
    uint16_t resv6;
    uint64_t resv7;
    uint64_t resv8;
};
#pragma pack()

int TaskMemcpyParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
}

#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_MEMCPY_PARSER_ITEM_H
