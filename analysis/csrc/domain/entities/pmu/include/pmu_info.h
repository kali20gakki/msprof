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
#include "analysis/csrc/domain/entities/metric/include/metric.h"

namespace Analysis {
namespace Domain {

struct PmuBaseInfo {
    AcceleratorType type = INVALID;
    virtual ~PmuBaseInfo() = default;
};

struct PmuInfoSingleAccelerator : public PmuBaseInfo {
    double totalTime = 0;
    uint64_t totalCycles = 0;
    std::vector<double> pmuResult;
};

struct PmuInfoMixAccelerator : public PmuBaseInfo {
    uint32_t totalBlockCount = 0;
    uint64_t mainTimestamp = 0;
    double aiCoreTime = 0;
    uint64_t aicTotalCycles = 0;
    double aivTime = 0;
    uint64_t aivTotalCycles = 0;
    std::vector<double> aicPmuResult;
    std::vector<double> aivPmuResult;
};
}
}
#endif // MSPROF_ANALYSIS_PMU_INFO_H
