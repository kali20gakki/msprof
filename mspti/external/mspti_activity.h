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
    MSPTI_ACTIVITY_KIND_MARKER = 1,
    MSPTI_ACTIVITY_KIND_KERNEL = 2,
    MSPTI_ACTIVITY_KIND_COUNT,
    MSPTI_ACTIVITY_KIND_FORCE_INT = 0x7fffffff
} msptiActivityKind;

typedef enum {
    MSPTI_ACTIVITY_MARKER_MODE_HOST = 0,
    MSPTI_ACTIVITY_MARKER_MODE_DEVICE = 1
} msptiActivityMarkerMode;

typedef enum {
    MSPTI_ACTIVITY_FLAG_NONE = 0,
    MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS = 1 << 0, // 瞬时mark
    MSPTI_ACTIVITY_FLAG_MARKER_START = 1 << 1, // range start
    MSPTI_ACTIVITY_FLAG_MARKER_END = 1 << 2 // range stop
} msptiActivityFlag;

START_PACKED_ALIGNMENT

typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;
} msptiActivity;


typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;
    msptiActivityFlag flag;
    msptiActivityMarkerMode mode;
    uint64_t timestamp;
    uint64_t id;
    union {
        struct {
            uint32_t processId;
            uint32_t threadId;
        } pt;
        struct {
            uint32_t deviceId;
            uint32_t streamId;
        } ds;
    } msptiObjectId;
    const char *name;
    const char *domain;
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
