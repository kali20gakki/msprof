
/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soclog.h
 * Description        : 通用抽象hallog
 * Author             : msprof team
 * Creation Date      : 2025/07/29
 * *****************************************************************************
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