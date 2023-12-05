/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: handle profiling data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023/11/6
 */

#ifndef ANALYSIS_ENTITIES_ASCEND_OBJ_H
#define ANALYSIS_ENTITIES_ASCEND_OBJ_H

#include <cstdint>
#include <memory>
#include "prof_common.h"

namespace Analysis {
namespace Entities {

#define DEFAULT_CONTEXT_ID 0xffffffff

enum class OpType {
    OPTYPE_HCCL_BIG = 0,
    OPTYPE_HCCL_SMALL,
    OPTYPE_KERNEL,
    OPTYPE_KERNEKL_HCCL,
    OPTYPE_RESERVED,
    OPTYPE_INVALID
};

struct TimeRange {
    uint64_t start;
    uint64_t end;
    TimeRange(uint64_t start_, uint64_t end_):start(start_), end(end_) {}
};

struct OpDesc {
    std::shared_ptr<MsprofCompactInfo> nodeDesc;
    std::shared_ptr<ConcatTensorInfo> tensorDesc;
    std::shared_ptr<MsprofAdditionalInfo> ctxId;
};

struct HcclMiniOpDesc {
    std::shared_ptr<MsprofAdditionalInfo> hcclInfo;
    uint32_t ctxId = DEFAULT_CONTEXT_ID;
};

struct HcclBigOpDesc {
    std::shared_ptr<TimeRange> hostTime;
    std::shared_ptr<MsprofCompactInfo> nodeDesc;
};

struct Operator {
    uint64_t name = 0;
    OpType type = OpType::OPTYPE_INVALID;
    std::shared_ptr<char> desc;

    Operator(uint64_t name, OpType type) : name(name), type(type)
    {}
};

struct HostTask {
    uint32_t modelId = 0;
    uint32_t requestId = 0;
    uint32_t streamId = 0;
    uint32_t taskId = 0;
    uint32_t contextId = 0;
    uint16_t batchId = 0;
    uint16_t deviceId = 0;
    // node层API对应的trace编号(id<<32+contextId)
    uint64_t nodeTraceId = 0;
    uint64_t taskType = 0;
    uint64_t timeStamp = 0;
    // nullptr when no op
    std::shared_ptr<Operator> op;
};

} // namespace Entities
} // namespace Analysis
#endif // ANALYSIS_ENTITIES_ASCEND_OBJ_H
