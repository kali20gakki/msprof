/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_op_desc.h
 * Description        : 通信算子描述vo
 * Author             : msprof team
 * Creation Date      : 2025/05/07
 * *****************************************************************************
 */

#ifndef MSPTI_PARSER_COMMUNICATION_OP_DESC_H
#define MSPTI_PARSER_COMMUNICATION_OP_DESC_H

#include <cstdint>
#include <vector>
#include "csrc/common/inject/hccl_inject.h"
#include "csrc/common/config.h"

#include "csrc/common/inject/profapi_inject.h"

namespace Mspti {
namespace Parser {
struct CommunicationTask {
    uint64_t start;
    uint64_t end;
    uint64_t hostStartTime;
    uint64_t hostEndTime;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t deviceId;
    bool agingFlag = true;
    CommunicationTask(uint64_t start, uint64_t end, uint64_t hostStartTime, uint64_t hostEndTime, uint16_t streamId,
        uint16_t taskId, uint16_t deviceId, bool agingFlag = true)
        : start(start),
          end(end),
          hostStartTime(hostStartTime),
          hostEndTime(hostEndTime),
          streamId(streamId),
          taskId(taskId),
          deviceId(deviceId),
          agingFlag(agingFlag)
    {}
};

struct CommunicationOpDesc {
    bool agingFlag = true;
    uint64_t threadId;
    uint64_t hostStartTime;
    uint64_t hostEndTime;
    uint64_t startTime;
    uint64_t endTime;
    uint32_t deviceId;
    uint32_t streamId;
    uint64_t algTypeHash;
    uint64_t opNameHash;
    uint8_t dataType;
    uint64_t groupNameHash;
    uint64_t count;
    uint64_t correlationId;
    std::vector<std::unique_ptr<CommunicationTask>> tasks;
};
}
}


#endif // MSPTI_PARSER_COMMUNICATION_OP_DESC_H
