// /* ******************************************************************************
//             版权所有 (c) 华为技术有限公司 2024-2024
//             Copyright, 2024, Huawei Tech. Co., Ltd.
// ****************************************************************************** *
// /* ******************************************************************************
//  * File Name          : task_type_parser_item.h
//  * Description        : taskType数据类型解析头文件
//  * Author             : msprof team
//  * Creation Date      : 24-4-29 下午3:49
//  * *****************************************************************************
//  *

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_TYPE_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_TYPE_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
#define PARSER_ITEM_TS_TASK_TYPE 0x0C

#pragma pack(1)
struct TaskType {
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t streamId : 11;
    uint16_t resv4 : 5;
    uint16_t taskId;
    uint16_t taskType;
    uint8_t taskStatus;
    uint8_t resv5;
    uint64_t resv6;
    uint64_t resv7;
};
#pragma pack()

int TaskTypeParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
}


#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_TASK_TYPE_PARSER_ITEM_H
