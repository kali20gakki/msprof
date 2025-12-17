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
#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H

#include <cstdint>
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"

namespace Analysis {
namespace Domain {
const int PARSER_ITEM_FFTS_PLUS_LOG_START = 0b100010;
const int PARSER_ITEM_FFTS_PLUS_LOG_END = 0b100011;

#pragma pack(1)
struct FftsPlusLog {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t subTaskType : 8;
    uint16_t resv3 : 8;
    uint16_t subTaskId;
    uint16_t resv4 : 13;
    uint16_t fftsType : 3;
    uint16_t threadId;
    uint32_t resv5[10];
};
#pragma pack()

int FftsPlusLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize,
                         uint8_t *halUniData);

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H