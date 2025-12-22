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

#ifndef MSPTI_PROJECT_SOCLOG_H
#define MSPTI_PROJECT_SOCLOG_H

#include <cstdint>

namespace Mspti {
struct SocLog {
    uint16_t funcType;
    uint16_t taskType;
    uint16_t streamId;
    uint32_t taskId;
    uint64_t timestamp;
    SocLog() = default;
    SocLog(uint16_t funcType_, uint16_t taskType_, uint16_t streamId_, uint32_t taskId_, uint64_t timestamp_)
        : funcType(funcType_), taskType(taskType_), streamId(streamId_), taskId(taskId_), timestamp(timestamp_){};
};

struct FftsLog {
    uint16_t funcType;
    uint16_t taskType;
    uint16_t streamId;
    uint32_t taskId;
    uint64_t timestamp;
    uint16_t subTaskId;
    FftsLog() = default;
    FftsLog(uint16_t funcType_, uint16_t taskType_, uint16_t streamId_, uint32_t taskId_, uint64_t timestamp_,
        uint16_t subTaskId_)
        : funcType(funcType_),
          taskType(taskType_),
          streamId(streamId_),
          taskId(taskId_),
          timestamp(timestamp_),
          subTaskId(subTaskId_){};
};

enum HalLogType {
    ACSQ_LOG = 0,
    FFTS_LOG,
    ACC_PMU,
    INVALID_LOG = 3
};

struct HalLogData {
    HalLogType type{INVALID_LOG};

    union {
        SocLog acsq{};
        FftsLog ffts;
    };
    HalLogData() {}
};
}

#endif // MSPTI_PROJECT_SOCLOG_H