/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : profapi_inject.h
 * Description        : Injection of profapi.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_PROFAPI_INJECT_H
#define MSPTI_COMMON_INJECT_PROFAPI_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

enum ProfilerCallbackType {
    PROFILE_CTRL_CALLBACK = 0,
    PROFILE_DEVICE_STATE_CALLBACK,
    PROFILE_REPORT_API_CALLBACK,
    PROFILE_REPORT_EVENT_CALLBACK,
    PROFILE_REPORT_COMPACT_CALLBACK,
    PROFILE_REPORT_ADDITIONAL_CALLBACK,
    PROFILE_REPORT_REG_TYPE_INFO_CALLBACK,
    PROFILE_REPORT_GET_HASH_ID_CALLBACK,
    PROFILE_HOST_FREQ_IS_ENABLE_CALLBACK,
    PROFILE_REPORT_API_C_CALLBACK,
    PROFILE_REPORT_EVENT_C_CALLBACK,
    PROFILE_REPORT_REG_TYPE_INFO_C_CALLBACK,
    PROFILE_REPORT_GET_HASH_ID_C_CALLBACK,
};

#define MSPROF_DATA_HEAD_MAGIC_NUM  0x5a5a
#define MSPROF_EVENT_FLAG 0xFFFFFFFFFFFFFFFFULL
#define PROF_INVALID_MODE_ID 0xFFFFFFFFUL
#define MAX_DEV_NUM 64

struct MsprofApi { // for MsprofReportApi
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t reserve;
    uint64_t beginTime;
    uint64_t endTime;
    uint64_t itemId;
};

struct MsprofEvent {  // for MsprofReportEvent
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t requestId; // 0xFFFF means single event
    uint64_t timeStamp;
    uint64_t reserve = MSPROF_EVENT_FLAG;
    uint64_t itemId;
};

struct MsprofRuntimeTrack {
    uint16_t deviceId;
    uint16_t streamId;
    uint32_t taskInfo;
    uint64_t taskType;
};

struct MsprofNodeBasicInfo {
    uint64_t opName;
    uint32_t taskType;
    uint64_t opType;
    uint32_t blockDim;
    uint32_t opFlag;
    uint8_t opState;
};

struct MsprofAttrInfo {
    uint64_t opName;
    uint32_t attrType;
    uint64_t hashId;
};

#pragma pack(1)
struct MsprofHcclOPInfo {  // for MsprofReportCompactInfo buffer data
    uint8_t relay : 1;
    uint8_t retry : 1;
    uint8_t dataType;
    uint16_t algType;     // 通信算子使用的算法,每4bit表示一个阶段,最多4个阶段
    uint64_t count;
    uint64_t groupName;
};
#pragma pack()

enum TsTaskType {
    TS_TASK_TYPE_KERNEL_AICORE = 0,
    TS_TASK_TYPE_KERNEL_AICPU = 1,
    TS_TASK_TYPE_KERNEL_AIVEC = 66,
    TS_TASK_TYPE_FLIP = 98,
    TS_TASK_TYPE_RESERVED,
};

enum RtProfileDataType {
    RT_PROFILE_TYPE_TASK_BEGIN = 0,
    RT_PROFILE_TYPE_TASK_END = 800,
    RT_PROFILE_TYPE_MEMCPY_INFO = 801,
    RT_PROFILE_TYPE_TASK_TRACK = 802,
    RT_PROFILE_TYPE_API_BEGIN = 1000,
    RT_PROFILE_TYPE_API_END = 2000,
    RT_PROFILE_TYPE_MAX
};

const uint16_t MSPROF_COMPACT_INFO_DATA_LENGTH = 40;
struct MsprofCompactInfo {
    uint16_t magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t dataLen;
    uint64_t timeStamp;
    union {
        uint8_t info[MSPROF_COMPACT_INFO_DATA_LENGTH];
        MsprofRuntimeTrack runtimeTrack;
        MsprofNodeBasicInfo nodeBasicInfo;
        MsprofAttrInfo nodeAttrInfo;
        MsprofHcclOPInfo hcclopInfo;
    } data;
};

#define PATH_LEN_MAX 1023
#define PARAM_LEN_MAX 4095
struct CommandHandleParams {
    uint32_t pathLen;
    uint32_t storageLimit;  // MB
    uint32_t profDataLen;
    char path[PATH_LEN_MAX + 1];
    char profData[PARAM_LEN_MAX + 1];
};

struct CommandHandle {
    uint64_t profSwitch;
    uint64_t profSwitchHi;
    uint32_t devNums;
    uint32_t devIdList[MAX_DEV_NUM];
    uint32_t modelId;
    uint32_t type;
    uint32_t cacheFlag;
    struct CommandHandleParams params;
};

enum CommandHandleType {
    PROF_COMMANDHANDLE_TYPE_INIT = 0,
    PROF_COMMANDHANDLE_TYPE_START,
    PROF_COMMANDHANDLE_TYPE_STOP,
    PROF_COMMANDHANDLE_TYPE_FINALIZE,
};

#if defined(__cplusplus)
extern "C" {
#endif

MSPTI_API int32_t MsprofRegTypeInfo(uint16_t level, uint32_t typeId, const char *typeName);
MSPTI_API int32_t MsprofReportCompactInfo(uint32_t agingFlag, const void* data, uint32_t length);
MSPTI_API int32_t MsprofReportData(uint32_t moduleId, uint32_t type, void* data, uint32_t len);
#if defined(__cplusplus)
}
#endif
#endif
