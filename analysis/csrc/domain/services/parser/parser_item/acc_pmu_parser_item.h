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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACC_PMU_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACC_PMU_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
const int PARSER_ITEM_ACC_PMU = 0b011010;

#pragma pack(1)
struct AccPmu {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t resv1 : 6;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t resv4;
    uint16_t accId;
    uint32_t resv5[3];
    uint64_t bandwidth[2];
    uint64_t ost[2];
};
#pragma pack()

int AccPmuParserItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACC_PMU_PARSER_ITEM_H
