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
#ifndef MSPTI_PROJECT_ACL_MEM_INJECT_H
#define MSPTI_PROJECT_ACL_MEM_INJECT_H

#include "csrc/common/inject/inject_base.h"
#include "csrc/include/mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum AclrtMemMallocPolicy {
    ACL_MEM_MALLOC_HUGE_FIRST,
    ACL_MEM_MALLOC_HUGE_ONLY,
    ACL_MEM_MALLOC_NORMAL_ONLY,
    ACL_MEM_MALLOC_HUGE_FIRST_P2P,
    ACL_MEM_MALLOC_HUGE_ONLY_P2P,
    ACL_MEM_MALLOC_NORMAL_ONLY_P2P,
    ACL_MEM_MALLOC_HUGE1G_ONLY,
    ACL_MEM_MALLOC_HUGE1G_ONLY_P2P,
    ACL_MEM_TYPE_LOW_BAND_WIDTH   = 0x0100,
    ACL_MEM_TYPE_HIGH_BAND_WIDTH  = 0x1000,
    ACL_MEM_ACCESS_USER_SPACE_READONLY = 0x100000,
} AclrtMemMallocPolicy;

typedef enum {
    ACL_RT_MEM_ATTR_RSV = 0,
    ACL_RT_MEM_ATTR_MODULE_ID,
    ACL_RT_MEM_ATTR_DEVICE_ID,
} AclrtMallocAttrType;

typedef union {
    uint16_t moduleId;
    uint32_t deviceId;
    uint8_t rsv[8];
} AclrtMallocAttrValue;

typedef struct {
    AclrtMallocAttrType attr;
    AclrtMallocAttrValue value;
} AclrtMallocAttribute;

typedef struct {
    AclrtMallocAttribute* attrs;
    size_t numAttrs;
} AclrtMallocConfig;

typedef enum AclrtMemcpyKind {
    ACL_MEMCPY_HOST_TO_HOST,
    ACL_MEMCPY_HOST_TO_DEVICE,
    ACL_MEMCPY_DEVICE_TO_HOST,
    ACL_MEMCPY_DEVICE_TO_DEVICE,
    ACL_MEMCPY_DEFAULT,
    ACL_MEMCPY_HOST_TO_BUF_TO_DEVICE,
    ACL_MEMCPY_INNER_DEVICE_TO_DEVICE, // 这俩目前没有映射关系
    ACL_MEMCPY_INTER_DEVICE_TO_DEVICE,
} AclrtMemcpyKind;

typedef enum AclrtMemAttr {
    ACL_DDR_MEM,
    ACL_HBM_MEM,
    ACL_DDR_MEM_HUGE,
    ACL_DDR_MEM_NORMAL,
    ACL_HBM_MEM_HUGE,
    ACL_HBM_MEM_NORMAL,
    ACL_DDR_MEM_P2P_HUGE,
    ACL_DDR_MEM_P2P_NORMAL,
    ACL_HBM_MEM_P2P_HUGE,
    ACL_HBM_MEM_P2P_NORMAL,
    ACL_HBM_MEM_HUGE1G,
    ACL_HBM_MEM_P2P_HUGE1G,
} AclrtMemAttr;

typedef enum AclrtMemHandleType {
    ACL_MEM_HANDLE_TYPE_NONE = 0,
} AclrtMemHandleType;

typedef enum AclrtMemAllocationType {
    ACL_MEM_ALLOCATION_TYPE_PINNED = 0,
} AclrtMemAllocationType;

typedef enum AclrtMemLocationType {
    ACL_MEM_LOCATION_TYPE_HOST = 0, /**< reserved enum, current version not support */
    ACL_MEM_LOCATION_TYPE_DEVICE,
} AclrtMemLocationType;

typedef struct AclrtMemLocation {
    uint32_t id;
    AclrtMemLocationType type;
} aclrtMemLocation;

typedef struct AclrtPhysicalMemProp {
    AclrtMemHandleType handleType;
    AclrtMemAllocationType allocationType;
    AclrtMemAttr memAttr;
    AclrtMemLocation location;
    uint64_t reserve;
} AclrtPhysicalMemProp;

typedef void* AclrtDrvMemHandle;

MSPTI_API AclError aclrtMalloc(void **devPtr, size_t size, AclrtMemMallocPolicy policy);
MSPTI_API AclError aclrtMallocHost(void **hostPtr, size_t size);
MSPTI_API AclError aclrtMallocWithCfg(void **devPtr, size_t size, AclrtMemMallocPolicy policy, AclrtMallocConfig *cfg);
MSPTI_API AclError aclrtMallocForTaskScheduler(void **devPtr, size_t size, AclrtMemMallocPolicy policy,
                                               AclrtMallocConfig *cfg);
MSPTI_API AclError aclrtMallocCached(void **devPtr, size_t size, AclrtMemMallocPolicy policy);
MSPTI_API AclError aclrtFree(void *devPtr);
MSPTI_API AclError aclrtFreeHost(void *hostPtr);
MSPTI_API AclError aclrtMemFlush(void *devPtr, size_t size);
MSPTI_API AclError aclrtMemInvalidate(void *devPtr, size_t size);
MSPTI_API AclError aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, AclrtMemcpyKind kind);
MSPTI_API AclError aclrtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                                    AclrtMemcpyKind kind, AclrtStream stream);
MSPTI_API AclError aclrtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                                 size_t height, AclrtMemcpyKind kind);
MSPTI_API AclError aclrtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                                      size_t height, AclrtMemcpyKind kind, AclrtStream stream);
MSPTI_API AclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count);
MSPTI_API AclError aclrtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, AclrtStream stream);
MSPTI_API AclError aclrtGetMemInfo(AclrtMemAttr attr, size_t *freePtr, size_t *totalPtr);
MSPTI_API AclError aclrtReserveMemAddress(void **virPtr, size_t size, size_t alignment, void *expectPtr,
                                          uint64_t flags);
MSPTI_API AclError aclrtReleaseMemAddress(void *virPtr);
MSPTI_API AclError aclrtMallocPhysical(AclrtDrvMemHandle *handle, size_t size, const AclrtPhysicalMemProp *prop,
                                       uint64_t flags);
MSPTI_API AclError aclrtFreePhysical(AclrtDrvMemHandle handle);
MSPTI_API AclError aclrtMemExportToShareableHandle(AclrtDrvMemHandle handle, AclrtMemHandleType handleType,
                                                   uint64_t flags, uint64_t *shareableHandle);
MSPTI_API AclError aclrtMemImportFromShareableHandle(uint64_t shareableHandle, int32_t deviceId,
                                                     AclrtDrvMemHandle *handle);
MSPTI_API AclError aclrtMemSetPidToShareableHandle(uint64_t shareableHandle, int32_t *pid, size_t pidNum);

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_PROJECT_ACL_MEM_INJECT_H