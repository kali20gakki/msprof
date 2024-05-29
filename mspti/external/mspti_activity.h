/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_activity.h
 * Description        : Activity API.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_H
#define MSPTI_ACTIVITY_H

#define ACTIVITY_STRUCT_ALIGNMENT 8
#if defined(_WIN32)
#define START_PACKED_ALIGNMENT __pragma(pack(push, 1))
#define PACKED_ALIGNMENT __declspec(align(ACTIVITY_STRUCT_ALIGNMENT))
#define END_PACKED_ALIGNMENT __pragma(pack(pop))
#elif defined(__GNUC__)
#define START_PACKED_ALIGNMENT
#define PACKED_ALIGNMENT __attribute__((__packed__)) __attribute__((aligned(ACTIVITY_STRUCT_ALIGNMENT)))
#define END_PACKED_ALIGNMENT
#else
#define START_PACKED_ALIGNMENT
#define PACKED_ALIGNMENT
#define END_PACKED_ALIGNMENT
#endif

#include "mspti_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    MSPTI_ACTIVITY_KIND_INVALID = 0,
    MSPTI_ACTIVITY_KIND_MARK = 1,
    MSPTI_ACTIVITY_KIND_COUNT,
    MSPTI_ACTIVITY_KIND_FORCE_INT = 0x7fffffff
} msptiActivityKind;

START_PACKED_ALIGNMENT

typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;
} msptiActivity;


typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;
    uint8_t mode;
    uint64_t timestamp;
    uint64_t markId;
    uint32_t streamId;
    uint32_t deviceId;
    const char *name;
} msptiActivityMark;

END_PACKED_ALIGNMENT

typedef void(*msptiBuffersCallbackRequestFunc)(uint8_t **buffer, size_t *size, size_t *maxNumRecords);

typedef void(*msptiBuffersCallbackCompleteFunc)(uint8_t *buffer, size_t size, size_t validSize);

MSPTI_API msptiResult msptiActivityRegisterCallbacks(
    msptiBuffersCallbackRequestFunc funcBufferRequested, msptiBuffersCallbackCompleteFunc funcBufferCompleted);

MSPTI_API msptiResult msptiActivityEnable(msptiActivityKind kind);

MSPTI_API msptiResult msptiActivityDisable(msptiActivityKind kind);

MSPTI_API msptiResult msptiActivityGetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record);

MSPTI_API msptiResult msptiActivityFlushAll(uint32_t flag);


#if defined(__cplusplus)
}
#endif

#endif
