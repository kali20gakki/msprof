/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_CONTEXT_PMU 0b101000

#pragma pack(1)
struct ContextPmu {
    uint16_t funcType : 6;     // 第1个，16位数据的低6位
    uint16_t cnt : 4;
    uint16_t resv1 : 6;       // resv字段为解析完整大小结构占位需要，实际未使用
    uint16_t resv2;
    uint16_t streamId;    // 第3个
    uint16_t taskId;           // 第4个，16位数据
    uint64_t resv4;
    uint16_t subTaskType : 8;  // 第6个，16位数据低8位
    uint16_t resv5 : 2;
    uint16_t ovFlag : 1;       // 第6个，16位数据第11位
    uint16_t resv6 : 5;
    uint16_t subTaskId;        // 第7个，16位数据
    uint16_t resv7 : 13;
    uint16_t fftsType : 3;     // 第8个，16位数据高三位
    uint16_t resv8;
    uint64_t resv9;
    uint64_t totalCycle;       // 第11个，64位数据
    uint64_t resv10;
    uint64_t pmuList[8];       // 第13-20个，64位数据
    uint64_t timeList[2];      // 第21、22个，64位数据
};
#pragma pack()

int Chip4PmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);

}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H
