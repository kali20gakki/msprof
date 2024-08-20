/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_mem_inject.h
 * Description        : inject runtime mem api
 * Author             : msprof team
 * Creation Date      : 2024/08/20
 * *****************************************************************************
*/
#ifndef MSPTI_PROJECT_RUNTIME_MEM_INJECT_H
#define MSPTI_PROJECT_RUNTIME_MEM_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
* @brief memory copy type
*/
typedef enum TagRtMemcpyKind {
    RT_MEMCPY_HOST_TO_HOST = 0,
    RT_MEMCPY_HOST_TO_DEVICE,
    RT_MEMCPY_DEVICE_TO_HOST,
    RT_MEMCPY_DEVICE_TO_DEVICE,  // device to device, 1P && P2P
    RT_MEMCPY_MANAGED,
    RT_MEMCPY_ADDR_DEVICE_TO_DEVICE,
    RT_MEMCPY_HOST_TO_DEVICE_EX, // host  to device ex (only used for 8 bytes)
    RT_MEMCPY_DEVICE_TO_HOST_EX,
    RT_MEMCPY_DEFAULT,
    RT_MEMCPY_RESERVED,
} RtMemcpyKindT;

/**
 * @ingroup dvrt_mem
 * @brief memory type
 */
typedef enum TagRtMemoryType {
    RT_MEMORY_TYPE_HOST = 1,    // host
    RT_MEMORY_TYPE_DEVICE = 2,  // device
    RT_MEMORY_TYPE_SVM = 3,     // SVM
    RT_MEMORY_TYPE_DVPP = 4     // DVPP
} RtMemoryTypeT;

typedef struct DrvMemHandle {
    int32_t id;
    uint32_t side;
    uint32_t devid;
    uint64_t pg_num;
} RtDrvMemHandleT;

typedef enum DrvMemHandleType {
    RT_MEM_HANDLE_TYPE_NONE = 0x0,
} RtDrvMemHandleType;

typedef struct DrvMemProp {
    uint32_t side;
    uint32_t devid;
    uint32_t module_id;

    uint32_t pg_type;
    uint32_t mem_type;
    uint64_t reserve;
} RtDrvMemPropT;

/**
 * @ingroup dvrt_mem
 * @brief memory attribute
 */
typedef struct TagRtPointerAttributes {
    RtMemoryTypeT memoryType;  // host or device
    RtMemoryTypeT locationType;
    uint32_t deviceID;
    uint32_t pageSize;
} RtPointerAttributesT;

typedef enum TagRtMemInfoType {
    RT_MEMORYINFO_DDR,
    RT_MEMORYINFO_HBM,
    RT_MEMORYINFO_DDR_HUGE,
    RT_MEMORYINFO_DDR_NORMAL,
    RT_MEMORYINFO_HBM_HUGE,
    RT_MEMORYINFO_HBM_NORMAL,
    RT_MEMORYINFO_DDR_P2P_HUGE,
    RT_MEMORYINFO_DDR_P2P_NORMAL,
    RT_MEMORYINFO_HBM_P2P_HUGE,
    RT_MEMORYINFO_HBM_P2P_NORMAL,
} RtMemInfoTypeT;

/**
 * @ingroup rt_kernel
 * @brief ai core memory size
 */
typedef struct RtAiCoreMemorySize {
    uint32_t l0ASize;                   // l0
    uint32_t l0BSize;
    uint32_t l0CSize;
    uint32_t l1Size;
    uint32_t ubSize;
    uint32_t l2Size;                    // l2
    uint32_t l2PageNum;
    uint32_t blockSize;
    uint64_t bankSize;                  // bank
    uint64_t bankNum;
    uint64_t burstInOneBlock;
    uint64_t bankGroupNum;
} RtAiCoreMemorySizeT;

typedef enum TagRtRecudeKind {
    RT_MEMCPY_SDMA_AUTOMATIC_ADD = 10,  // D2D, SDMA inline reduce, include 1P, and P2P
    RT_MEMCPY_SDMA_AUTOMATIC_MAX = 11,
    RT_MEMCPY_SDMA_AUTOMATIC_MIN = 12,
    RT_MEMCPY_SDMA_AUTOMATIC_EQUAL = 13,
    RT_RECUDE_KIND_END = 14,
} RtRecudeKindT;

typedef enum TagRtDataType {
    RT_DATA_TYPE_FP32 = 0,
    RT_DATA_TYPE_FP16 = 1,
    RT_DATA_TYPE_INT16 = 2,
    RT_DATA_TYPE_INT4 = 3,
    RT_DATA_TYPE_INT8 = 4,
    RT_DATA_TYPE_INT32 = 5,
    RT_DATA_TYPE_BFP16 = 6,
    RT_DATA_TYPE_BFP32 = 7,
    RT_DATA_TYPE_UINT8 = 8,
    RT_DATA_TYPE_UINT16 = 9,
    RT_DATA_TYPE_UINT32 = 10,
    RT_DATA_TYPE_END = 11,
} RtDataTypeT;

typedef enum TagRtCmoType {
    RT_CMO_PREFETCH = 6, // Preload
    RT_CMO_WRITEBACK, // Prewriteback
    RT_CMO_INVALID, // invalid
    RT_CMO_FLUSH, // flush
    RT_CMO_RESERVED,
} RtCmoOpCodeT;

typedef struct {
    const char *name;
    const uint64_t size;
    uint32_t flag;
} RtMallocHostSharedMemoryIn;

typedef struct {
    int32_t fd;
    void *ptr;
    void *devPtr;
} RtMallocHostSharedMemoryOut;

typedef struct {
    const char *name;
    const uint64_t size;
    int32_t fd;
    void *ptr;
    void *devPtr;
} RtFreeHostSharedMemoryIn;

MSPTI_API RtErrorT rtMalloc(void **devPtr, uint64_t size, RtMemTypeT type, const uint16_t moduleId);
MSPTI_API RtErrorT rtFree(void *devPtr);
MSPTI_API RtErrorT rtMallocHost(void **hostPtr, uint64_t size, const uint16_t moduleId);
MSPTI_API RtErrorT rtFreeHost(void *hostPtr);
MSPTI_API RtErrorT rtMallocCached(void **devPtr, uint64_t size, RtMemTypeT type, const uint16_t moduleId);
MSPTI_API RtErrorT rtFlushCache(void *base, size_t len);
MSPTI_API RtErrorT rtInvalidCache(void *base, size_t len);
MSPTI_API RtErrorT rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind);
MSPTI_API RtErrorT rtMemcpyEx(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind);
MSPTI_API RtErrorT rtMemcpyHostTask(void * const dst, const uint64_t destMax, const void * const src,
                                    const uint64_t cnt, RtMemcpyKindT kind, RtStreamT stm);
MSPTI_API RtErrorT rtMemcpyAsync(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind,
                                 RtStreamT stm);
MSPTI_API RtErrorT rtMemcpy2d(void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width,
                              uint64_t height, RtMemcpyKindT kind);
MSPTI_API RtErrorT rtMemcpy2dAsync(void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width,
                                   uint64_t height, RtMemcpyKindT kind, RtStreamT stm);
MSPTI_API RtErrorT rtMemset(void *devPtr, uint64_t destMax, uint32_t val, uint64_t cnt);
MSPTI_API RtErrorT rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t val, uint64_t cnt, RtStreamT stm);
MSPTI_API RtErrorT rtMemGetInfo(size_t *freeSize, size_t *totalSize);
MSPTI_API RtErrorT rtMemGetInfoEx(RtMemInfoTypeT memInfoType, size_t *freeSize, size_t *totalSize);
MSPTI_API RtErrorT rtReserveMemAddress(void** devPtr, size_t size, size_t alignment, void *devAddr, uint64_t flags);
MSPTI_API RtErrorT rtReleaseMemAddress(void* devPtr);
MSPTI_API RtErrorT rtMallocPhysical(RtDrvMemHandleT** handle, size_t size, RtDrvMemPropT* prop, uint64_t flags);
MSPTI_API RtErrorT rtFreePhysical(RtDrvMemHandleT* handle);
MSPTI_API RtErrorT rtMemExportToShareableHandle(RtDrvMemHandleT* handle, RtDrvMemHandleType handleType,
                                                uint64_t flags, uint64_t *shareableHandle);
MSPTI_API RtErrorT rtMemImportFromShareableHandle(uint64_t shareableHandle, int32_t devId, RtDrvMemHandleT** handle);
MSPTI_API RtErrorT rtMemSetPidToShareableHandle(uint64_t shareableHandle, int pid[], uint32_t pidNum);

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_PROJECT_RUNTIME_MEM_INJECT_H