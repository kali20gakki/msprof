/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acsq_log_parser.h
 * Description        : 解析acsq task数据
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H

#include <cstdint>
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"

namespace Analysis {
namespace Domain {

const int PARSER_ITEM_ACSQ_LOG_START = 0b000000;
const int PARSER_ITEM_ACSQ_LOG_END = 0b000001;

#pragma pack(1)
struct AcsqLog {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t resv4;
    uint16_t resv5;
    uint32_t resv6[11];
};
#pragma pack()

int AcsqLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H