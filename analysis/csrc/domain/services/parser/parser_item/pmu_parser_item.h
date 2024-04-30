/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_parser_item.h
 * Description        : ffts_profile类型二进制数据中context级别PMU数据解析
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {

#define PARSER_ITEM_CONTEXT_PMU 0b101000

#pragma pack(1)
struct ContextPmu {
    uint16_t funcType : 6;     // 第1个，16位数据的低6位
    uint16_t resv1 : 10;       // resv字段为解析完整大小结构占位需要，实际未使用
    uint16_t resv2;
    uint16_t streamId : 11;    // 第3个，16位数据低11位
    uint16_t resv3 : 5;
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

void PmuParseItem(void *binaryData, uint32_t binaryDataSize, void *halUniData, uint32_t halUniDataSize);

}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_PARSER_ITEM_H
