/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_lpm_parser_item.h
 * Description        : 解析频率二进制数据结构体头文件
 * Author             : msprof team
 * Creation Date      : 2024/5/15
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H
#define MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H

#include <cstdint>

namespace Analysis {
namespace Domain {
#define DEFAULT_FREQ_LPM 0x00
#pragma pack(1)

struct FreqLpmData {
    uint64_t sysCnt;
    uint32_t freq;
    uint32_t resv1;
};
#pragma pack()

#pragma pack(1)
struct FreqData {
    uint32_t count;
    uint32_t resv1;
    FreqLpmData lpmDataS[55];
};
#pragma pack()

int FreqLpmParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData);
}
} // Analysis

#endif // MSPROF_ANALYSIS_FREQ_LPM_PARSER_ITEM_H
