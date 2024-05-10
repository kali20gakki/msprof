/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : block_pmu_parser_item.h
 * Description        : ffts_profile类型二进制数据中block级别PMU数据解析
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_BLOCK_PMU_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_BLOCK_PMU_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_BLOCK_PMU 0b101001

#pragma pack(1)
struct BlockPmu {
    uint16_t funcType : 6;     // 第1个，16位数据的低6位
    uint16_t cnt : 4;
    uint16_t resv1 : 6;       // resv字段为解析完整大小结构占位需要，实际未使用
    uint16_t resv2;
    uint16_t streamId : 11;    // 第3个，16位数据低11位
    uint16_t resv3 : 5;
    uint16_t taskId;           // 第4个，16位数据
    uint64_t resv4;
    uint16_t subTaskType : 8;  // 第6个，16位数据低8位
    uint16_t resv5 : 8;
    uint16_t subTaskId;        // 第7个，16位数据
    uint8_t coreType : 1;      // 第8个，8位数据最低1位
    uint8_t coreId : 7;        // 第8个，8位数据高7位
    uint8_t resv6 : 5;
    uint8_t fftsType : 3;      // 第9个，8位数据高3位
    uint16_t resv7;
    uint32_t resv8;
    uint16_t subBlockId;       // 第12个，16位数据
    uint16_t blockId;          // 第13个，16位数据
    uint64_t totalCycle;       // 第14个，64位数据
    uint64_t resv9;
    uint64_t pmuList[8];       // 第16-23个，64位数据
    uint64_t timeList[2];      // 第24、25个，64位数据
};
#pragma pack()

int BlockPmuParseItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData);

}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_BLOCK_PMU_PARSER_ITEM_H
