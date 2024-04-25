/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_pmu.h
 * Description        : pmu抽象数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_HAL_PMU_H
#define MSPROF_ANALYSIS_HAL_PMU_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {

enum HalPmuType {
    PMU = 0,
    BLOCK_PMU,
    INVALID_PMU = 2
};

struct HalPmu {         // 主核context, 从核block
    AcceleratorType acceleratorType;  // 加速器类型，即AIC、AIV、MIX_AIC、MIX_AIV
    uint64_t totalCycle = 0;
    uint64_t pmuList[8] = {0};
    uint64_t timeList[2] = {0};   // startTimestamp 和 endTimestamp

    uint8_t ovFlag = 0;
    uint16_t subBlockId = 0;
    uint16_t blockId = 0;
    uint8_t coreType = 0;       // mix时判断是aic还是aiv上报
    uint8_t coreId = 0;
};

struct HalPmuData {
    HalUniData hd;
    HalPmuType type;
    HalPmu pmu;
};
}
}

#endif // MSPROF_ANALYSIS_HAL_PMU_H
