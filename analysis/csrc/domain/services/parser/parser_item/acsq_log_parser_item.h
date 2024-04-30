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

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_ACSQ_LOG_START 0b000000
#define PARSER_ITEM_ACSQ_LOG_END   0b000001

#pragma pack(1)
struct AcsqLog {
    uint16_t funcType : 6;
    uint16_t resv1 : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t streamId : 11;
    uint16_t resv3 : 5;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t resv4;
    uint16_t resv5;
    uint32_t resv6[11];
};
#pragma pack()

void AcsqLogParseItem(void *binaryData, uint32_t binaryDataSize, void *halUniData, uint32_t halUniDataSize);

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_ACSQ_LOG_PARSER_ITEM_H