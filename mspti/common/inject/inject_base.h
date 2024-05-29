/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : inject_base.h
 * Description        : Common definition of Injection.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_INJECT_BASE_H
#define MSPTI_COMMON_INJECT_INJECT_BASE_H

#include <stdint.h>

using rtError_t = uint32_t;
using rtStream_t = void *;
using rtSmDesc_t = void;
using rtArgsEx_t = void;

enum DrvError {
    DRV_ERROR_NONE = 0,
    DRV_ERROR_NO_DEVICE = 1,
    DRV_ERROR_NOT_SUPPORT = 0xfffe,
};

#define PROF_CHANNEL_NAME_LEN 32
#define PROF_CHANNEL_NUM_MAX 160

struct ChannelInfo {
    char channelName[PROF_CHANNEL_NAME_LEN];
    unsigned int channelType;
    unsigned int channelId;
};

typedef struct ChannelList {
    unsigned int chipType;
    unsigned int channelNum;
    struct ChannelInfo channel[PROF_CHANNEL_NUM_MAX];
} ChannelListT;

enum AI_DRV_CHANNEL {
    PROF_CHANNEL_UNKNOWN         = 0,
    PROF_CHANNEL_TS_FW           = 44,
    PROF_CHANNEL_MAX             = 160,
};

typedef enum ProfChannelType {
    PROF_CHANNEL_TYPE_TS,
    PROF_CHANNEL_TYPE_PERIPHERAL,
    PROF_CHANNEL_TYPE_MAX,
} PROF_CHANNEL_TYPE;

typedef struct ProfStartPara {
    PROF_CHANNEL_TYPE channelType;
    unsigned int samplePeriod;
    unsigned int realTime;
    void *userData;
    unsigned int userDataSize;
} ProfStartParaT;

typedef struct ProfPollInfo {
    unsigned int deviceId;
    unsigned int channelId;
} ProfPollInfoT;

typedef struct TagTsTsFwProfileConfig {
    uint32_t period;
    uint32_t tsTaskTrack;     // 1-enable,2-disable
    uint32_t tsCpuUsage;      // 1-enable,2-disable
    uint32_t aiCoreStatus;    // 1-enable,2-disable
    uint32_t tsTimeline;       // 1-enable,2-disable
    uint32_t aiVectorStatus;  // 1-enable,2-disable
    uint32_t tsKeypoint;       // 1-enable,2-disable
    uint32_t tsMemcpy;         // 1-enable,2-disable
} TsTsFwProfileConfigT;

enum PROFILE_MODE {
    PROFILE_REAL_TIME = 1,
};

#endif
