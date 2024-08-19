/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : channel_data.h
 * Description        : data struct of channel data.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_DATA_H
#define MSPTI_ACTIVITY_ASCEND_CHANNEL_CHANNEL_DATA_H

#include <stdint.h>

enum TsTrackRptType {
    RPT_TYPE_STEP_TRACE = 10,
    RPT_TYPE_TS_MEMCPY = 11,
    RPT_TYPE_TASK_TYPE = 12,
    RTP_TYPE_FLIP_INFO = 14,
};

enum StepTraceTag {
    STEP_TRACE_TAG_MARKEX = 11,
};

#pragma pack(4)
// 40通道
#define TS_TRACK_SIZE 40
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

struct TaskFlipInfo {
    TsTrackHead tsTraceHead;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t flipNum;
    uint8_t reserved2[2];
    uint16_t taskId;
    uint8_t reserved3[16];
};

struct StarsSocLog {
    uint32_t funcType : 6;
    uint32_t cnt : 4;
    uint32_t sqeType : 6;
    uint32_t magic : 16;
    uint32_t streamId : 16;
    uint32_t taskId : 16;
    uint32_t sysCntL : 32;
    uint32_t sysCntH : 32;
    uint32_t res0 : 16;
    uint32_t accId : 6;
    uint32_t acsqId : 10;
    uint32_t res1[11];
};
#pragma pack()

#endif
