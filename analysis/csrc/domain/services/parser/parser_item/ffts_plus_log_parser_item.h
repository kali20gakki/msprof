/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ffts_plus_log_parser_item.h
 * Description        : 解析ffts plus log数据
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
#define PARSER_ITEM_FFTS_PLUS_LOG_START  0b100010
#define PARSER_ITEM_FFTS_PLUS_LOG_END    0b100011

#pragma pack(1)
struct FftsPlusLog {
    uint16_t funcType : 6;
    uint16_t resv1 : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t taskId;
    uint16_t streamId : 11;
    uint16_t resv3 : 5;
    uint64_t timestamp;
    uint16_t subTaskType;
    uint16_t subTaskId;
    uint16_t resv4 : 13;
    uint16_t fftsType : 3;
    uint16_t threadId;
    uint32_t resv5[10];
};
#pragma pack()

void FftsPlusLogParseItem(void *binaryData, uint32_t binaryDataSize,
                          void *halUniData, uint32_t halUniDataSize);

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FFTS_PLUS_LOG_PARSER_ITEM_H