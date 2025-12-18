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

#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_DATA_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_DATA_H

#include <cstdint>

enum TsTrackRptType {
    RPT_TYPE_STEP_TRACE = 10,
    RPT_TYPE_TS_MEMCPY = 11,
    RPT_TYPE_TASK_TYPE = 12,
    RTP_TYPE_FLIP_INFO = 14,
};

enum StepTraceTag {
    STEP_TRACE_TAG_MARKEX = 11,
};

enum StarsFuncType {
    STARS_FUNC_TYPE_BEGIN = 0b000000,
    STARS_FUNC_TYPE_END = 0b000001,
    FFTS_PLUS_TYPE_START = 0b100010,
    FFTS_PLUS_TYPE_END = 0b100011,
};

#pragma pack(4)
// 40通道
struct TsTrackHead {
    uint8_t mode;
    uint8_t rptType;
    uint16_t bufSize;
};

struct StepTrace {
    TsTrackHead tsTraceHead;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t tagId;
    uint8_t reserved2[2];
};

struct TsMemCpy {
    TsTrackHead tsTraceHead;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t taskId;
    uint8_t taskState;
    uint8_t reserved2[19];
};

struct TaskType {
    TsTrackHead tsTraceHead;
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t taskType;
    uint8_t taskState;
    uint8_t reserved1[21];
};

struct StarsSocHeader {
    uint16_t funcType : 6;
    uint16_t resv : 4;
    uint16_t taskType : 6;
};

struct StarsSocLog {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv0;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t resv1;
    uint16_t accId : 6;
    uint16_t acsqId : 10;
    uint32_t resv2[11];
};

struct FftsPlusLog {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv2;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint16_t subTaskType : 8;
    uint16_t resv3 : 8;
    uint16_t subTaskId;
    uint16_t resv4 : 13;
    uint16_t fftsType : 3;
    uint16_t threadId;
    uint32_t resv5[10];
};

struct StarsSocLogV6 {
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t taskType : 6;
    uint16_t resv0;
    uint32_t taskId;
    uint64_t timestamp;
    uint16_t resv1;
    uint16_t accId : 6;
    uint16_t acsqId : 10;
    uint32_t resv2[3];
};
#pragma pack()

#endif
