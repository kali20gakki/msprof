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

#ifndef MSPTI_COMMON_INJECT_INJECT_BASE_H
#define MSPTI_COMMON_INJECT_INJECT_BASE_H

#include <cstdint>
#include <stdexcept>
#include <string>

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSPTI_API __declspec(dllexport)
#else
#define MSPTI_API __attribute__((visibility("default")))
#endif

using RtErrorT = uint32_t;
using RtStreamT = void *;
using RtSmDescT = void;
using VOID_PTR = void*;
using CONST_VOID_PTR = const void *;
using VOID_PTR_PTR = void**;
using RtMemTypeT = uint32_t;

enum DrvError {
    DRV_ERROR_NONE = 0,
    DRV_ERROR_NO_DEVICE = 1,
    DRV_ERROR_NOT_SUPPORT = 0xfffe,
};

#define PROF_CHANNEL_NAME_LEN 32
#define PROF_CHANNEL_NUM_MAX 160

template<typename Type>
inline void THROW_FUNC_NOTFOUND(Type ptr, const std::string &funName, const std::string &soName)
{
    if (ptr == nullptr) {
        throw std::runtime_error("Failed to get function: \""+ funName + "\" from \"" + soName + "\"");
    }
}

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
    PROF_CHANNEL_STARS_SOC_LOG   = 50,
    PROF_CHANNEL_MAX             = 160,
};

typedef enum ProfChannelType {
    PROF_CHANNEL_TYPE_TS,
    PROF_CHANNEL_TYPE_PERIPHERAL,
    PROF_CHANNEL_TYPE_MAX,
} PROF_CHANNEL_TYPE;

typedef enum TAG_TS_PROFILE_COMMAND_TYPE {
    TS_PROFILE_COMMAND_TYPE_ACK = 0,
    TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE = 1,
    TS_PROFILE_COMMAND_TYPE_PROFILING_DISABLE = 2,
    TS_PROFILE_COMMAND_TYPE_BUFFERFULL = 3,
    TS_PROFILE_COMMAND_TASK_BASE_ENABLE = 4,     // task base profiling enable
    TS_PROFILE_COMMAND_TASK_BASE_DISENABLE = 5,  // task base profiling disenable
    TS_PROFILE_COMMAND_TS_FW_ENABLE = 6,         // TS fw data enable
    TS_PROFILE_COMMAND_TS_FW_DISENABLE = 7,      // TS fw data disenable
} TS_PROFILE_COMMAND_TYPE_T;

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

typedef struct TagStarsSocLogConfig {
    uint32_t acsq_task;         // 1-enable,2-disable
    uint32_t acc_pmu;           // 1-enable,2-disable
    uint32_t cdqm_reg;          // 1-enable,2-disable
    uint32_t dvpp_vpc_block;    // 1-enable,2-disable
    uint32_t dvpp_jpegd_block;  // 1-enable,2-disable
    uint32_t dvpp_jpede_block;  // 1-enable,2-disable
    uint32_t ffts_thread_task;  // 1-enable,2-disable
    uint32_t ffts_block;        // 1-enable,2-disable
    uint32_t sdma_dmu;          // 1-enable,2-disable
} StarsSocLogConfigT;

enum PROFILE_MODE {
    PROFILE_REAL_TIME = 1,
};

typedef struct rtHostInputInfo {
    uint32_t addrOffset;
    uint32_t dataOffset;
} RtHostInputInfoT;

typedef struct tagRtArgsEx {
    void *args;                     // args host mem addr
    RtHostInputInfoT *hostInputInfoPtr;     // nullptr means no host mem input
    uint32_t argsSize;              // input + output + tiling addr size + tiling data size + host mem
    uint32_t tilingAddrOffset;      // tiling addr offset
    uint32_t tilingDataOffset;      // tiling data offset
    uint16_t hostInputInfoNum;      // hostInputInfo num
    uint8_t hasTiling;              // if has tiling: 0 means no tiling
    uint8_t isNoNeedH2DCopy;        // is no need host to device copy: 0 means need H2D copy,
                                    // others means doesn't need H2D copy.
    uint8_t reserved[4];
} RtArgsExT;

typedef struct tagRtAicpuArgsEx {
    void *args; // args host mem addr
    RtHostInputInfoT *hostInputInfoPtr; // nullptr means no host mem input
    RtHostInputInfoT *kernelOffsetInfoPtr; // KernelOffsetInfo, it is different for CCE Kernel and fwk kernel
    uint32_t argsSize;
    uint16_t hostInputInfoNum; // hostInputInfo num
    uint16_t kernelOffsetInfoNum; // KernelOffsetInfo num
    uint32_t soNameAddrOffset; // just for CCE Kernel, default value is 0xffff for FWK kernel
    uint32_t kernelNameAddrOffset; // just for CCE Kernel, default value is 0xffff for FWK kernel
    bool isNoNeedH2DCopy; // is no need host to device copy: 0 means need H2D copy,
                               // other means doesn't need H2D copy.
    uint8_t reserved[3];
} RtAicpuArgsExT;

typedef void *rtFuncHandle;
typedef void *rtLaunchArgsHandle;

typedef struct tagRtTaskCfgInfo {
    uint8_t qos;
    uint8_t partId;
    uint8_t schemMode; // rtschemModeType_t 0:normal;1:batch;2:sync
    bool d2dCrossFlag; // d2dCrossFlag true:D2D_CROSS flase:D2D_INNER
    uint32_t blockDimOffset;
    uint8_t dumpflag; // dumpflag 0:fault 2:RT_KERNEL_DUMPFLAG 4:RT_FUSION_KERNEL_DUMPFLAG
} RtTaskCfgInfoT;

#define MSPROF_ENGINE_MAX_TAG_LEN (63)

struct ReporterData {
    char tag[MSPROF_ENGINE_MAX_TAG_LEN + 1];  // the sub-type of the module, data with different tag will be writen
    int deviceId;                             // the index of device
    size_t dataLen;                           // the length of send data
    unsigned char *data;                      // the data content
};

struct MsprofStampInfo {
    uint16_t magicNumber;
    uint16_t dataTag;
    uint32_t processId;
    uint32_t threadId;
    uint32_t category;      // marker category
    uint32_t eventType;
    int32_t payloadType;
    union PayloadValue {    // payload info for marker
        uint64_t ullValue;
        int64_t llValue;
        double dValue;
        uint32_t uiValue[2];
        int32_t iValue[2];
        float fValue[2];
    } payload;
    uint64_t startTime;
    uint64_t endTime;
    int32_t messageType;
    char message[128];
};

struct MsprofStampInstance {
    ReporterData report;
    MsprofStampInfo stampInfo;
    int id;
    struct MsprofStampInstance* next;
    struct MsprofStampInstance* prev;
};
#endif
