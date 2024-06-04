/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_parser_item.h
 * Description        : 解析acc pmu数据
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

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
