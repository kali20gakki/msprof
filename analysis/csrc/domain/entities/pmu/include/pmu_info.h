/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_info.h
 * Description        : 填充deviceTask的抽象PMU结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_PMU_INFO_H
#define MSPROF_ANALYSIS_PMU_INFO_H

#include <cstdint>
#include <vector>
#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {

struct PmuBaseInfo {
    AcceleratorType type;

    virtual ~PmuBaseInfo() = default;
};

struct PmuInfoNone : public PmuBaseInfo {
    AcceleratorType type;
};

struct PmuInfoAic : public PmuBaseInfo {
    uint64_t aicore_time = 0;
    uint64_t aic_total_cycles = 0;
    uint64_t pmu_list[8] = {0};
};

struct PmuInfoAiV : public PmuBaseInfo {
    uint64_t aiv_time = 0;
    uint64_t aiv_total_cycles = 0;
    uint64_t pmu_list[8] = {0};
};

struct PmuInfoMix : public PmuBaseInfo {
    uint32_t total_block_count = 0;
    uint64_t aicore_time = 0;
    uint64_t aic_total_cycles = 0;
    uint64_t aiv_time = 0;
    uint64_t aiv_total_cycles = 0;
    uint64_t aic_pmu_list[8] = {0};
    uint64_t aiv_pmu_list[8] = {0};
};
}
}
#endif // MSPROF_ANALYSIS_PMU_INFO_H
