/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_flip_parser_item.h
 * Description        : taskFlip数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/23
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_FLIP_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_FLIP_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
#define PARSER_ITEM_TASK_FLIP 0x0e

#pragma pack(1)
struct TaskFlip {
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t streamId : 11;
    uint16_t resv4 : 5;
    uint16_t flipNum;
    uint16_t resv5;
    uint16_t taskId;
    uint8_t resv6[16];
};
#pragma pack()

void TaskFlipParseItem(void *binaryData, uint32_t binaryDataSize,
                       void *halUniData, uint32_t halUniDataSize);
}
}
#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_FLIP_PARSER_ITEM_H