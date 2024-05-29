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

typedef struct StepTrace {
    uint8_t mode;
    uint8_t rptType;
    uint16_t bufSize;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint64_t indexId;
    uint64_t modelId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t tagId;
    uint16_t reserved2;
} StepTraceT;

struct Timeline {
    uint8_t mode;
    uint8_t rptType;
    uint16_t bufSize;
    uint8_t reserved1[4];
    uint16_t taskType;
    uint16_t taskState;
    uint16_t streamId;
    uint16_t taskId;
    uint64_t timestamp;
    uint32_t threadId;
    uint32_t deviceId;
};

struct TsMemCpy {
    uint8_t mode;
    uint8_t rptType;
    uint16_t bufSize;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t taskId;
    uint8_t taskState;
    uint8_t reserved2[19];
};

struct TaskType {
    uint8_t mode;
    uint8_t rptType;
    uint16_t bufSize;
    uint8_t reserved1[4];
    uint64_t timestamp;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t taskType;
    uint8_t taskState;
    uint8_t reserved2[17];
};

typedef enum {
    RPT_TYPE_TIMELINE = 3,
    RPT_TYPE_STEP_TRACE = 10,
    RPT_TYPE_TS_MEMCPY = 11,
    RPT_TYPE_TASK_TYPE = 12,
    RTP_TYPE_INNER = 14,
} TsTrackRptType;

#endif
