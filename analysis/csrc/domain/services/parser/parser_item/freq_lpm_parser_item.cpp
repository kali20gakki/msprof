/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_lpm_parser_item.cpp
 * Description        : 解析频率二进制数据结构体
 * Author             : msprof team
 * Creation Date      : 2024/5/15
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "securec.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

int FreqLpmParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(FreqData)) {
        ERROR("The TrunkSize of Freq is not equal with the FreqData struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *binData = ReinterpretConvert<FreqData *>(binaryData);
    auto *targetData = ReinterpretConvert<HalFreqData *>(halUniData);
    int count = binData->count;

    targetData->count = binData->count;
    for (int i = 0; i < count; ++i) {
        targetData->freqLpmDataS[i].sysCnt = binData->lpmDataS[i].sysCnt;
        targetData->freqLpmDataS[i].freq = binData->lpmDataS[i].freq;
    }
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(FREQ_PARSER, DEFAULT_FREQ_LPM, FreqLpmParseItem);
}
} // Analysis