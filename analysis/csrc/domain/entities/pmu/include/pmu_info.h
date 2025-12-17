/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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
