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

#ifndef ANALYSIS_DOMAIN_ENTITIES_STEP_TRACE_DATA_H
#define ANALYSIS_DOMAIN_ENTITIES_STEP_TRACE_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct TrainTraceData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    // indexId means iterationId
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
