/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: handle profiling data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023/11/6
 */

#ifndef ANALYSIS_ENTITIES_ASCEND_OBJ_H
#define ANALYSIS_ENTITIES_ASCEND_OBJ_H
namespace Analysis {
namespace Entities {

#include <cstdint>
#include <memory>

enum class OpType {
    OPTYPE_HCCL_BIG = 0,
    OPTYPE_HCCL_SMALL,
    OPTYPE_KERNEL,
    OPTYPE_KERNEKL_HCCL,
    OPTYPE_RESERVED,
    OPTYPE_INVALID
};

struct Operator {
    uint64_t name = 0;
    OpType type = OpType::OPTYPE_INVALID;
    std::shared_ptr<char> desc;
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
