/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_freq.h
 * Description        : freq_lpm抽象结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_HAL_FREQ_H
#define MSPROF_ANALYSIS_HAL_FREQ_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {
struct HalFreqLpmData {
    uint64_t sysCnt = 0;
    uint32_t freq = 0;
};

// 用于接收二进制转换后数据, 需要在parser中过滤有效数据
struct HalFreqData {
    uint64_t count;
    HalFreqLpmData freqLpmDataS[55];
};
}
}

#endif // MSPROF_ANALYSIS_HAL_FREQ_H
