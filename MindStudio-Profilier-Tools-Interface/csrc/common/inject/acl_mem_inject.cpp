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
#include "acl_mem_inject.h"

#include <functional>

#include "activity/activity_manager.h"
#include "activity/ascend/reporter/memory_reporter.h"
#include "callback/callback_manager.h"
#include "common/context_manager.h"
#include "common/function_loader.h"

namespace {
const std::string SO_NAME = "libascendcl";
const std::string SO_FILE_NAME = SO_NAME + ".so";

enum AclRtMemFuncIndex {
    FUNC_ACL_RT_MALLOC,
    FUNC_ACL_RT_MALLOC_HOST,
    FUNC_ACL_RT_MALLOC_WITH_CFG,
    FUNC_ACL_RT_MALLOC_FOR_TASK_SCHEDULER,
    FUNC_ACL_RT_MALLOC_CACHED,
    FUNC_ACL_RT_FREE,
    FUNC_ACL_RT_FREE_HOST,
    FUNC_ACL_RT_MEM_FLUSH,
    FUNC_ACL_RT_MEM_INVALIDATE,
    FUNC_ACL_RT_MEMCPY,
    FUNC_ACL_RT_MEMCPY_ASYNC,
    FUNC_ACL_RT_MEMCPY_2D,
    FUNC_ACL_RT_MEMCPY_2D_ASYNC,
    FUNC_ACL_RT_MEMSET,
    FUNC_ACL_RT_MEMSET_ASYNC,
    FUNC_ACL_RT_GET_MEM_INFO,
    FUNC_ACL_RT_RESERVE_MEM_ADDRESS,
    FUNC_ACL_RT_RELEASE_MEM_ADDRESS,
    FUNC_ACL_RT_MALLOC_PHYSIAL,
    FUNC_ACL_RT_FREE_PHYSIAL,
    FUNC_ACL_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE,
    FUNC_ACL_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE,
    FUNC_ACL_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE,
    FUNC_ACL_RT_COUNT
};

pthread_once_t g_once;
void *g_aclrtMemFuncArray[FUNC_ACL_RT_COUNT];

void LoadAclMemFunction()
{
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMalloc");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_HOST] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMallocHost");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_WITH_CFG] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMallocWithCfg");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_FOR_TASK_SCHEDULER] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMallocForTaskScheduler");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_CACHED] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMallocCached");
    g_aclrtMemFuncArray[FUNC_ACL_RT_FREE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtFree");
    g_aclrtMemFuncArray[FUNC_ACL_RT_FREE_HOST] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtFreeHost");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_FLUSH] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemFlush");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_INVALIDATE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemInvalidate");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemcpy");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_ASYNC] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemcpyAsync");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_2D] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemcpy2d");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_2D_ASYNC] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemcpy2dAsync");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMSET] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemset");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEMSET_ASYNC] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemsetAsync");
    g_aclrtMemFuncArray[FUNC_ACL_RT_GET_MEM_INFO] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtGetMemInfo");
    g_aclrtMemFuncArray[FUNC_ACL_RT_RESERVE_MEM_ADDRESS] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtReserveMemAddress");
    g_aclrtMemFuncArray[FUNC_ACL_RT_RELEASE_MEM_ADDRESS] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtReleaseMemAddress");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_PHYSIAL] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMallocPhysical");
    g_aclrtMemFuncArray[FUNC_ACL_RT_FREE_PHYSIAL] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtFreePhysical");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemExportToShareableHandle");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemImportFromShareableHandle");
    g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtMemSetPidToShareableHandle");
}
}

MSPTI_API AclError aclrtMalloc(void **devPtr, size_t size, AclrtMemMallocPolicy policy)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC];
    using aclrtMallocFunc = std::function<decltype(aclrtMalloc)>;
    aclrtMallocFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMalloc)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, policy);
}

MSPTI_API AclError aclrtMallocHost(void **hostPtr, size_t size)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_HOST];
    using aclrtMallocHostFunc = std::function<decltype(aclrtMallocHost)>;
    aclrtMallocHostFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMallocHost)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_HOST, __FUNCTION__);
    return func(hostPtr, size);
}

MSPTI_API AclError aclrtMallocWithCfg(void **devPtr, size_t size, AclrtMemMallocPolicy policy, AclrtMallocConfig *cfg)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_WITH_CFG];
    using aclrtMallocWithCfgFunc = std::function<decltype(aclrtMallocWithCfg)>;
    aclrtMallocWithCfgFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMallocWithCfg)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, policy, cfg);
}

MSPTI_API AclError aclrtMallocForTaskScheduler(void **devPtr, size_t size, AclrtMemMallocPolicy policy,
                                               AclrtMallocConfig *cfg)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_FOR_TASK_SCHEDULER];
    using aclrtMallocForTaskSchedulerFunc = std::function<decltype(aclrtMallocForTaskScheduler)>;
    aclrtMallocForTaskSchedulerFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtMallocForTaskScheduler)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, policy, cfg);
}

MSPTI_API AclError aclrtMallocCached(void **devPtr, size_t size, AclrtMemMallocPolicy policy)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_CACHED];
    using aclrtMallocCachedFunc = std::function<decltype(aclrtMallocCached)>;
    aclrtMallocCachedFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMallocCached)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, policy);
}

MSPTI_API AclError aclrtFree(void *devPtr)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_FREE];
    using aclrtFreeFunc = std::function<decltype(aclrtFree)>;
    aclrtFreeFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtFree)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(&devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(devPtr);
}

MSPTI_API AclError aclrtFreeHost(void *hostPtr)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_FREE_HOST];
    using aclrtFreeHostFunc = std::function<decltype(aclrtFreeHost)>;
    aclrtFreeHostFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtFreeHost)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE_HOST, __FUNCTION__);
    return func(hostPtr);
}

MSPTI_API AclError aclrtMemFlush(void *devPtr, size_t size)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_FLUSH];
    using aclrtMemFlushFunc = std::function<decltype(aclrtMemFlush)>;
    aclrtMemFlushFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemFlush)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FLUSH_CACHE, __FUNCTION__);
    return func(devPtr, size);
}

MSPTI_API AclError aclrtMemInvalidate(void *devPtr, size_t size)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_INVALIDATE];
    using aclrtMemInvalidateFunc = std::function<decltype(aclrtMemInvalidate)>;
    aclrtMemInvalidateFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemInvalidate)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_INVALID_CACHE, __FUNCTION__);
    return func(devPtr, size);
}

MSPTI_API AclError aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, AclrtMemcpyKind kind)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY];
    using aclrtMemcpyFunc = std::function<decltype(aclrtMemcpy)>;
    aclrtMemcpyFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemcpy)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, count, nullptr, 0);
    return func(dst, destMax, src, count, kind);
}

MSPTI_API AclError aclrtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                                    AclrtMemcpyKind kind, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_ASYNC];
    using aclrtMemcpyAsyncFunc = std::function<decltype(aclrtMemcpyAsync)>;
    aclrtMemcpyAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemcpyAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, count, stream, 1);
    return func(dst, destMax, src, count, kind, stream);
}

MSPTI_API AclError aclrtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                                 size_t height, AclrtMemcpyKind kind)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_2D];
    using aclrtMemcpy2dFunc = std::function<decltype(aclrtMemcpy2d)>;
    aclrtMemcpy2dFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemcpy2d)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_CPY2D, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, width * height, nullptr, 0);
    return func(dst, dpitch, src, spitch, width, height, kind);
}

MSPTI_API AclError aclrtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                                      size_t height, AclrtMemcpyKind kind, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMCPY_2D_ASYNC];
    using aclrtMemcpy2dAsyncFunc = std::function<decltype(aclrtMemcpy2dAsync)>;
    aclrtMemcpy2dAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemcpy2dAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_CPY2D_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, width * height, stream, 1);
    return func(dst, dpitch, src, spitch, width, height, kind, stream);
}

MSPTI_API AclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMSET];
    using aclrtMemsetFunc = std::function<decltype(aclrtMemset)>;
    aclrtMemsetFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemset)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET, __FUNCTION__);
    auto record = Mspti::Reporter::MemsetRecord(value, count, nullptr, 0);
    return func(devPtr, maxCount, value, count);
}

MSPTI_API AclError aclrtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEMSET_ASYNC];
    using aclrtMemsetAsyncFunc = std::function<decltype(aclrtMemsetAsync)>;
    aclrtMemsetAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMemsetAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemsetRecord(value, count, stream, 1);
    return func(devPtr, maxCount, value, count, stream);
}

MSPTI_API AclError aclrtGetMemInfo(AclrtMemAttr attr, size_t *freePtr, size_t *totalPtr)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_GET_MEM_INFO];
    using aclrtGetMemInfoFunc = std::function<decltype(aclrtGetMemInfo)>;
    aclrtGetMemInfoFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtGetMemInfo)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_GET_INFO, __FUNCTION__);
    return func(attr, freePtr, totalPtr);
}

MSPTI_API AclError aclrtReserveMemAddress(void **virPtr, size_t size, size_t alignment, void *expectPtr,
                                          uint64_t flags)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_RESERVE_MEM_ADDRESS];
    using aclrtReserveMemAddressFunc = std::function<decltype(aclrtReserveMemAddress)>;
    aclrtReserveMemAddressFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtReserveMemAddress)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RESERVE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(virPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(virPtr, size, alignment, expectPtr, flags);
}

MSPTI_API AclError aclrtReleaseMemAddress(void *virPtr)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_RELEASE_MEM_ADDRESS];
    using aclrtReleaseMemAddressFunc = std::function<decltype(aclrtReleaseMemAddress)>;
    aclrtReleaseMemAddressFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtReleaseMemAddress)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RELEASE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(&virPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE,
                                                MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(virPtr);
}

MSPTI_API AclError aclrtMallocPhysical(AclrtDrvMemHandle *handle, size_t size, const AclrtPhysicalMemProp *prop,
                                       uint64_t flags)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MALLOC_PHYSIAL];
    using aclrtMallocPhysicalFunc = std::function<decltype(aclrtMallocPhysical)>;
    aclrtMallocPhysicalFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtMallocPhysical)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_PHYSICAL, __FUNCTION__);
    return func(handle, size, prop, flags);
}

MSPTI_API AclError aclrtFreePhysical(AclrtDrvMemHandle handle)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_FREE_PHYSIAL];
    using aclrtFreePhysicalFunc = std::function<decltype(aclrtFreePhysical)>;
    aclrtFreePhysicalFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtFreePhysical)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE_PHYSICAL, __FUNCTION__);
    return func(handle);
}

MSPTI_API AclError aclrtMemExportToShareableHandle(AclrtDrvMemHandle handle, AclrtMemHandleType handleType,
                                                   uint64_t flags, uint64_t *shareableHandle)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE];
    using aclrtMemExportToShareableHandleFunc = std::function<decltype(aclrtMemExportToShareableHandle)>;
    aclrtMemExportToShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtMemExportToShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_EXPORT_TO_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(handle, handleType, flags, shareableHandle);
}

MSPTI_API AclError aclrtMemImportFromShareableHandle(uint64_t shareableHandle, int32_t deviceId,
                                                     AclrtDrvMemHandle *handle)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE];
    using aclrtMemImportFromShareableHandleFunc = std::function<decltype(aclrtMemImportFromShareableHandle)>;
    aclrtMemImportFromShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtMemImportFromShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_IMPORT_FROM_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(shareableHandle, deviceId, handle);
}

MSPTI_API AclError aclrtMemSetPidToShareableHandle(uint64_t shareableHandle, int32_t *pid, size_t pidNum)
{
    pthread_once(&g_once, LoadAclMemFunction);
    void *voidFunc = g_aclrtMemFuncArray[FUNC_ACL_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE];
    using aclrtMemSetPidToShareableHandleFunc = std::function<decltype(aclrtMemSetPidToShareableHandle)>;
    aclrtMemSetPidToShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtMemSetPidToShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET_PID_TO_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(shareableHandle, pid, pidNum);
}
