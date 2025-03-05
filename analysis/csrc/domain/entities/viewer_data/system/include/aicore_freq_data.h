/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_format_data.h.h
 * Description        : aicore_freq_processor处理FreqParse表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_AICORE_FREQ_DATA_H
#define ANALYSIS_DOMAIN_AICORE_FREQ_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct AicoreFreqData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    double freq = 0.0;
};
}
}

#endif // ANALYSIS_DOMAIN_AICORE_FREQ_DATA_H