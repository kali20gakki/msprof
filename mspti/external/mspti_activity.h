/**
* @file mspti_activity.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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

#include "mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__GNUC__) && defined(MSPTI_LIB)
#pragma GCC visibility push(default)
#endif

/**
 * @brief The kinds of activity records.
 *
 * Each kind is associated with a
 * activity record structure that holds the information associated
 * with the kind.
 */
typedef enum {
    /**
    * The activity record is invalid.
    */
    MSPTI_ACTIVITY_KIND_INVALID = 0,
    /**
    * Extended, optional, data about a marker. The corresponding
    * activity record structure is msptiActivityMark
    */
    MSPTI_ACTIVITY_KIND_MARKER = 1,
    MSPTI_ACTIVITY_KIND_KERNEL = 2,
    MSPTI_ACTIVITY_KIND_API = 3,
    MSPTI_ACTIVITY_KIND_COUNT,
    MSPTI_ACTIVITY_KIND_FORCE_INT = 0x7fffffff
} msptiActivityKind;

/**
 * @brief The source kinds of mark data.
 *
 * Each mark activity record kind represents information about host or device
 */
typedef enum {
    MSPTI_ACTIVITY_SOURCE_KIND_HOST = 0,
    MSPTI_ACTIVITY_SOURCE_KIND_DEVICE = 1
} msptiActivitySourceKind;

/**
 * @brief Flags associated with activity records.
 *
 * Activity record flags. Flags can be combined by bitwise OR to
 * associated multiple flags with an activity record. Each flag is
 * specific to a certain activity kind, as noted below.
 */
typedef enum {
    /**
    * Indicates the activity record has no flags.
    */
    MSPTI_ACTIVITY_FLAG_NONE = 0,
    /**
    * Indicates the activity represents an instantaneous marker. Valid for
    * MSPTI_ACTIVITY_KIND_MARKER.
    */
    MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS = 1 << 0,
    /**
    * Indicates the activity represents a region start marker. Valid for
    * MSPTI_ACTIVITY_KIND_MARKER.
    */
    MSPTI_ACTIVITY_FLAG_MARKER_START = 1 << 1,
    /**
    * Indicates the activity represents a region end marker. Valid for
    * MSPTI_ACTIVITY_KIND_MARKER.
    */
    MSPTI_ACTIVITY_FLAG_MARKER_END = 1 << 2
} msptiActivityFlag;

START_PACKED_ALIGNMENT

typedef struct PACKED_ALIGNMENT {
    msptiActivityKind kind;
} msptiActivity;

typedef union PACKED_ALIGNMENT {
    /**
    * A process object requires that we identify the process ID. A
    * thread object requires that we identify both the process and
    * thread ID.
    */
    struct {
        uint32_t processId;
        uint32_t threadId;
    } pt;
    /**
    * A device object requires that we identify the device ID. A
    * context object requires that we identify both the device and
    * A stream object requires that we identify device and stream ID.
    */
    struct {
        uint32_t deviceId;
        uint32_t streamId;
    } ds;
} msptiObjectId;

/**
 * @brief The activity record providing a marker which is an
 * instantaneous point in time.
 *
 * The marker is specified with a descriptive name and unique id
 * (MSPTI_ACTIVITY_KIND_MARKER).
 */
typedef struct PACKED_ALIGNMENT {
    /**
    * The activity record kind, must be MSPTI_ACTIVITY_KIND_MARKER.
    */
    msptiActivityKind kind;

    /**
    * The flags associated with the marker.
    * @see msptiActivityFlag
    */
    msptiActivityFlag flag;

    /**
    * The source kinds of mark data.
    * @see msptiActivitySourceKind
    */
    msptiActivitySourceKind sourceKind;

    /**
    * The timestamp for the marker, in ns. A value of 0 indicates that
    * timestamp information could not be collected for the marker.
    */
    uint64_t timestamp;

    /**
    * The marker ID.
    */
    uint64_t id;

    /**
    * The identifier for the activity object associated with this
    * marker. 'objectKind' indicates which ID is valid for this record.
    */
    msptiObjectId objectId;

    /**
    * The marker name for an instantaneous or start marker.
    * This will be NULL for an end marker.
    */
    const char *name;

    /**
    * The name of the domain to which this marker belongs to.
    * This will be NULL for default domain.
    */
    const char *domain;
} msptiActivityMark;

END_PACKED_ALIGNMENT

/**
 * @brief Function type for callback used by MSPTI to request an empty
 * buffer for storing activity records.
 *
 * This callback function signals the MSPTI client that an activity
 * buffer is needed by MSPTI. The activity buffer is used by MSPTI to
 * store activity records. The callback function can decline the
 * request by setting **buffer to NULL. In this case MSPTI may drop
 * activity records.
 *
 * @param buffer Returns the new buffer. If set to NULL then no buffer
 * is returned.
 * @param size Returns the size of the returned buffer.
 * @param maxNumRecords Returns the maximum number of records that
 * should be placed in the buffer. If 0 then the buffer is filled with
 * as many records as possible. If > 0 the buffer is filled with at
 * most that many records before it is returned.
 */
typedef void(*msptiBuffersCallbackRequestFunc)(uint8_t **buffer, size_t *size, size_t *maxNumRecords);

/**
 * @brief Function type for callback used by MSPTI to return a buffer
 * of activity records.
 *
 * This callback function returns to the MSPTI client a buffer
 * containing activity records.  The buffer contains \p validSize
 * bytes of activity records which should be read using
 * msptiActivityGetNextRecord. After this call MSPTI
 * relinquished ownership of the buffer and will not use it
 * anymore. The client may return the buffer to MSPTI using the
 * mspti_BuffersCallbackRequestFunc callback.
 *
 * @param buffer The activity record buffer.
 * @param size The total size of the buffer in bytes as set in
 * MSPTI_BuffersCallbackRequestFunc.
 * @param validSize The number of valid bytes in the buffer.
 */
typedef void(*msptiBuffersCallbackCompleteFunc)(uint8_t *buffer, size_t size, size_t validSize);

/**
 * @brief Registers callback functions with MSPTI for activity buffer
 * handling.
 *
 * This function registers two callback functions to be used in asynchronous
 * buffer handling. If registered, activity record buffers are handled using
 * asynchronous requested/completed callbacks from MSPTI.
 *
 * Registering these callbacks prevents the client from using MSPTI's
 * blocking enqueue/dequeue functions.
 *
 * @param funcBufferRequested callback which is invoked when an empty
 * buffer is requested by MSPTI
 * @param funcBufferCompleted callback which is invoked when a buffer
 * containing activity records is available from MSPTI
 *
 * @retval MSPTI_SUCCESS
 * @retval MSPTI_ERROR_INVALID_PARAMETER if either
 * funcBufferRequested or funcBufferCompleted is NULL
 */
msptiResult msptiActivityRegisterCallbacks(
    msptiBuffersCallbackRequestFunc funcBufferRequested, msptiBuffersCallbackCompleteFunc funcBufferCompleted);

/**
 * @brief Enable collection of a specific kind of activity record.
 *
 * Enable collection of a specific kind of activity record. Multiple
 * kinds can be enabled by calling this function multiple times. By
 * default all activity kinds are disabled for collection.
 *
 * @param kind The kind of activity record to collect
 *
 * @retval MSPTI_SUCCESS
 */
msptiResult msptiActivityEnable(msptiActivityKind kind);

/**
 * @brief Disable collection of a specific kind of activity record.
 *
 * Disable collection of a specific kind of activity record. Multiple
 * kinds can be disabled by calling this function multiple times. By
 * default all activity kinds are disabled for collection.
 *
 * @param kind The kind of activity record to stop collecting
 *
 * @retval MSPTI_SUCCESS
 */
msptiResult msptiActivityDisable(msptiActivityKind kind);

/**
 * @brief Iterate over the activity records in a buffer.
 *
 * This is a function to iterate over the activity records in buffer.
 *
 * @param buffer The buffer containing activity records
 * @param validBufferSizeBytes The number of valid bytes in the buffer.
 * @param record Inputs the previous record returned by
 * msptiActivityGetNextRecord and returns the next activity record
 * from the buffer. If input value is NULL, returns the first activity
 * record in the buffer.
 *
 * @retval MSPTI_SUCCESS
 * @retval MSPTI_ERROR_MAX_LIMIT_REACHED if no more records in the buffer
 * @retval MSPTI_ERROR_INVALID_PARAMETER if buffer is NULL.
 */
msptiResult msptiActivityGetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record);

/**
 * @brief Request to deliver activity records via the buffer completion callback.
 *
 * This function returns the activity records associated with all contexts/streams
 * (and the global buffers not associated with any stream) to the MSPTI client
 * using the callback registered in msptiActivityRegisterCallbacks. it returns all the
 * activity buffers which have all the activity records completed, buffers need not
 * to be full though
 *
 * Before calling this function, the buffer handling callback api must be activated
 * by calling msptiActivityRegisterCallbacks.
 *
 * @param flag Reserved for internal use.
 *
 * @retval MSPTI_SUCCESS
 */
msptiResult msptiActivityFlushAll(uint32_t flag);

#if defined(__GNUC__) && defined(MSPTI_LIB)
#pragma GCC visibility pop
#endif

#if defined(__cplusplus)
}
#endif

#endif
