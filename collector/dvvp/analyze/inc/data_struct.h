/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: define data struct
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_DATA_STRUCT_H
#define ANALYSIS_DVVP_ANALYZE_DATA_STRUCT_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Dvvp {
namespace Analyze {
// common
static const std::string KEY_SEPARATOR = "-";
static const uint32_t PROFILE_MODE_INVALID = 0;
static const uint32_t PROFILE_MODE_SINGLE_OP = 1;
static const uint32_t PROFILE_MODE_STEP_TRACE = 2;

struct KeypointOp {
    uint16_t streamId;
    uint16_t taskId;
    uint64_t indexId;
    uint64_t modelId;
    uint64_t startTime;
    uint64_t endTime;
    bool uploaded; // false: send to pipe; true: not send to pipe
    uint64_t findSuccTimes; // find op success times by this KeypointOp
};

struct OpTime {
    uint64_t indexId;
    uint64_t start;
    uint64_t startAicore;
    uint64_t endAicore;
    uint64_t end;
    uint32_t threadId;
    uint32_t flag;  // enum aclprofSubscribeOpFlag
};

// ge
static const uint32_t GE_TASK_DESC_MODEL_NAME_INDEX = 0;
static const uint32_t GE_TASK_DESC_OP_NAME_INDEX = 1;
static const uint32_t GE_TASK_DESC_BLOCK_DIM_INDEX = 2;
static const uint32_t GE_TASK_DESC_TASK_ID_INDEX = 3;
static const uint32_t GE_TASK_DESC_STREAM_ID_INDEX = 4;
static const uint32_t GE_TASK_DESC_MODEL_ID_INDEX = 5;
static const uint32_t GE_TASK_DESC_SIZE = 6;


// ts
static const uint8_t TS_TIMELINE_RPT_TYPE = 3;
static const uint8_t TS_KEYPOINT_RPT_TYPE = 10;
static const uint8_t TS_INVALID_TYPE = 0xff;
static const uint16_t TS_TIMELINE_START_TASK_STATE = 2;
static const uint16_t TS_TIMELINE_END_TASK_STATE = 3;
static const uint16_t TS_TIMELINE_AICORE_START_TASK_STATE = 7;
static const uint16_t TS_TIMELINE_AICORE_END_TASK_STATE = 8;
static const uint16_t TS_KEYPOINT_START_TASK_STATE = 0;
static const uint16_t TS_KEYPOINT_END_TASK_STATE = 1;

struct TsProfileDataHead {
    uint8_t mode;   // 0-host,1-device
    uint8_t rptType;
    uint16_t bufSize;
    uint8_t reserved[4];    // reserved 4 bytes
};

struct TsProfileTimeline {
    TsProfileDataHead head;
    uint16_t taskType;
    uint16_t taskState;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint32_t thread;
    uint32_t deviceId;
};

struct TsProfileKeypoint {
    TsProfileDataHead head;
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t tagId;
    uint16_t resv;
};

// hwts
static const uint8_t HWTS_TASK_START_TYPE = 0;
static const uint8_t HWTS_TASK_END_TYPE = 1;
static const uint8_t HWTS_INVALID_TYPE = 0xff;
static const uint32_t HWTS_DATA_SIZE = 64;     // 64bytes

struct HwtsProfileType01 {
    uint8_t cntRes0Type;    // bit0-2:Type, bit3:Res0, bit4-7:Cnt
    uint8_t reserved;
    uint16_t hex6bd3;       // 0x6bd3
    uint8_t reserved1[2];   // reserved 2 bytes
    uint16_t taskId;
    uint64_t syscnt;
    uint32_t streamId;
    uint8_t reserved2[44];  // reserved 44 bytes, total size: 64 bytes
};

struct HwtsProfileType2 {
    uint8_t cntRes0Type;    // bit0-2:Type, bit3:Res0, bit4-7:Cnt
    uint8_t coreId;
    uint16_t hex6bd3;       // 0x6bd3
    uint16_t blockId;
    uint16_t taskId;
    uint64_t syscnt;
    uint32_t streamId;
    uint8_t reserved[44];   // reserved 44 bytes, total size: 64 bytes
};

struct HwtsProfileType3 {
    uint8_t cntWarnType;    // bit0-2:Type, bit3:Warn, bit4-7:Cnt
    uint8_t coreId;
    uint16_t hex6bd3;       // 0x6bd3
    uint16_t blockId;
    uint16_t taskId;
    uint64_t syscnt;
    uint32_t streamId;
    uint8_t reserved[4];    // reserved 4 bytes
    uint64_t warnStatus;
    uint8_t reserved2[32];  // reserved 32 bytes, total size: 64 bytes
};

// ffts
#define FFTS_DATA_SIZE 64   // 64bytes
#define ACSQ_TASK_START_FUNC_TYPE 0  // ACSQ task start log
#define ACSQ_TASK_END_FUNC_TYPE   1  // ACSQ task end log
#define FFTS_SUBTASK_THREAD_START_FUNC_TYPE 34  // ffts thread subtask start log
#define FFTS_SUBTASK_THREAD_END_FUNC_TYPE   35  // ffts thread subtask end log
struct FftsLogHead {
    uint16_t logType : 6;
    uint16_t cnt : 4;
    uint16_t sqeType : 6;
    uint16_t hex6bd3;
};

struct FftsAcsqLog {
    FftsLogHead head;
    uint16_t streamId;
    uint16_t taskId;
    uint32_t sysCountLow;
    uint32_t sysCountHigh;
    uint32_t reserved[12];
};

struct FftsCxtLog {
    FftsLogHead head;
    uint16_t streamId;
    uint16_t taskId;
    uint32_t sysCountLow;
    uint32_t sysCountHigh;
    uint16_t rsv1;
    uint16_t cxtId;
    uint16_t rsv2;
    uint16_t threadId;
    uint32_t rsv[10];
};

// profiling
struct ProfOpDesc {
    uint32_t signature;
    uint32_t modelId;
    uint32_t flag;
    uint32_t threadId;
    uint64_t opIndex;
    uint64_t duration;  // unit: us, schedule time + execution time;
    uint64_t start;
    uint64_t end;
    uint64_t executionTime; // AI Core execution time;
    uint64_t cubeFops;
    uint64_t vectorFops;    // total size: 64 bytes
};

struct RtOpInfo {
    uint64_t tsTrackTimeStamp;
    uint64_t start;
    uint64_t end;
    uint32_t threadId;
    bool ageFlag;
    uint64_t startAicore;
    uint64_t endAicore;
    uint32_t flag;
    uint16_t contextId;
};
 
// ge op info
struct GeOpFlagInfo {
    uint64_t opNameHash;
    uint64_t opTypeHash;
    uint64_t modelId;
    uint64_t start;
    uint64_t end;
    bool modelFlag;
    bool nodeFlag;
    bool ageFlag;
    uint16_t contextId;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
