/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_pmu_data.h
 * Description        : 统一db pmu类型头文件
 * Author             : msprof team
 * Creation Date      : 2025/2/15
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H
#define MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "analysis/csrc/domain/valueobject/include/task_id.h"

struct UnifiedTaskPmu {
    uint16_t deviceId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t subtaskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    std::string header;
    double value = 0.0;
    UnifiedTaskPmu() = default;
    UnifiedTaskPmu(uint16_t deviceId_, uint32_t streamId_, uint32_t taskId_,  uint32_t subtaskId_, uint32_t batchId_,
                   std::string header_, double value_) : deviceId(deviceId_), streamId(streamId_), taskId(taskId_),
                                                         subtaskId(subtaskId_), header(header_), value(value_){};
};

struct UnifiedSampleTimelinePmu {
    uint16_t deviceId = UINT16_MAX;
    uint64_t timestamp = UINT64_MAX;
    uint64_t totalCycle = UINT64_MAX;
    double usage = 0.0;
    double freq = 0.0;
    uint16_t coreId = UINT16_MAX;
    uint64_t coreType = UINT64_MAX;
    UnifiedSampleTimelinePmu() = default;
    UnifiedSampleTimelinePmu(uint16_t deviceId_, uint64_t timestamp_, uint64_t totalCycle_, double usage_,
                             double freq_, uint16_t coreId_, uint64_t coreType_)
        : deviceId(deviceId_), timestamp(timestamp_), totalCycle(totalCycle_), usage(usage_),
          freq(freq_), coreId(coreId_), coreType(coreType_){};
};

struct UnifiedSampleSummaryPmu {
    uint16_t deviceId = UINT16_MAX;
    std::string metric = "";
    double value = 0.0;
    uint16_t coreId = UINT16_MAX;
    uint64_t coreType = UINT64_MAX;
    UnifiedSampleSummaryPmu() = default;
    UnifiedSampleSummaryPmu(uint16_t deviceId_, std::string metric_, double value_, uint16_t coreId_,
                            uint64_t coreType_) : deviceId(deviceId_), metric(metric_),
                                                  value(value_), coreId(coreId_), coreType(coreType_){};
};

#endif // MSPROF_ANALYSIS_UNIFIED_PMU_DATA_H
