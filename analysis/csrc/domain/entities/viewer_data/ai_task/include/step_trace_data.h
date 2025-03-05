/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_data.h
 * Description        : 处理step_trace表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ENTITIES_STEP_TRACE_DATA_H
#define ANALYSIS_DOMAIN_ENTITIES_STEP_TRACE_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct TrainTraceData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint64_t fpStart = UINT64_MAX;
    uint64_t bpEnd = UINT64_MAX;
    uint64_t iterEnd = UINT64_MAX;
    uint64_t iterTime = UINT64_MAX;
    uint64_t fpBpTime = UINT64_MAX;
    uint64_t gradRefreshBound = UINT64_MAX;
    uint64_t dataAugBound = UINT64_MAX;
};

struct GetNextData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint64_t end = UINT64_MAX;
};

struct AllReduceData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint64_t iterEnd = UINT64_MAX;
    uint64_t end = UINT64_MAX;
};
}
}

#endif // ANALYSIS_DOMAIN_ENTITIES_STEP_TRACE_DATA_H
