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
 * -------------------------------------------------------------------------
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
