/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_block_dim_parser_item.h
 * Description        : Task BlockDim数据结构
 * Author             : msprof team
 * Creation Date      : 2024/8/3
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_DIM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_DIM_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
#define PARSER_ITEM_BLOCK_DIM 0x0f

#pragma pack(1)
struct BlockDim {
    uint8_t resv1;
    uint8_t funcType;
    uint16_t resv2;
    uint32_t resv3;
    uint64_t timestamp;
    uint16_t streamId : 11;
    uint16_t resv4 : 5;
    uint16_t taskId;
    uint32_t blockDim;
    uint8_t resv6[16];
};
#pragma pack()

int BlockDimParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
}
#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_BLOCK_DIM_PARSER_ITEM_H