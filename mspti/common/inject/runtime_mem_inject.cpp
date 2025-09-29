/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_mem_inject.cpp
 * Description        : inject runtime mem api
 * Author             : msprof team
 * Creation Date      : 2024/08/20
 * *****************************************************************************
 */
#include "runtime_mem_inject.h"

#include <functional>

#include "activity/activity_manager.h"
#include "activity/ascend/reporter/memory_reporter.h"
#include "callback/callback_manager.h"
#include "common/context_manager.h"
#include "common/function_loader.h"
#include "common/context_manager.h"

namespace {
enum RuntimeMemFuncIndex {
    FUNC_RT_MALLOC,
    FUNC_RT_FREE,
    FUNC_RT_MALLOC_HOST,
    FUNC_RT_FREE_HOST,
    FUNC_RT_MALLOC_CACHED,
    FUNC_RT_FLUSH_CACHE,
    FUNC_RT_INVALID_CACHE,
    FUNC_RT_MEMCPY,
    FUNC_RT_MEMCPY_EX,
    FUNC_RT_MEMCPY_HOST_TASK,
    FUNC_RT_MEMCPY_ASYNC,
    FUNC_RT_MEMCPY_2D,
    FUNC_RT_MEMCPY_2D_ASYNC,
    FUNC_RT_MEMSET,
    FUNC_RT_MEMSET_ASYNC,
    FUNC_RT_MEM_GET_INFO,
    FUNC_RT_MEM_GET_INFO_EX,
    FUNC_RT_RESERVE_MEM_ADDRESS,
    FUNC_RT_RELEASE_MEM_ADDRESS,
    FUNC_RT_MALLOC_PHYSICAL,
    FUNC_RT_FREE_PHYSICAL,
    FUNC_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE,
    FUNC_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE,
    FUNC_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE,
    FUNC_COUNT
};

pthread_once_t g_once = PTHREAD_ONCE_INIT;
void *g_runtimeMemFuncArray[FUNC_COUNT];

void LoadRuntimeMemFunction()
{
    g_runtimeMemFuncArray[FUNC_RT_MALLOC] = Mspti::Common::RegisterFunction("libruntime", "rtMalloc");
    g_runtimeMemFuncArray[FUNC_RT_FREE] = Mspti::Common::RegisterFunction("libruntime", "rtFree");
    g_runtimeMemFuncArray[FUNC_RT_MALLOC_HOST] = Mspti::Common::RegisterFunction("libruntime", "rtMallocHost");
    g_runtimeMemFuncArray[FUNC_RT_FREE_HOST] = Mspti::Common::RegisterFunction("libruntime", "rtFreeHost");
    g_runtimeMemFuncArray[FUNC_RT_MALLOC_CACHED] = Mspti::Common::RegisterFunction("libruntime", "rtMallocCached");
    g_runtimeMemFuncArray[FUNC_RT_FLUSH_CACHE] = Mspti::Common::RegisterFunction("libruntime", "rtFlushCache");
    g_runtimeMemFuncArray[FUNC_RT_INVALID_CACHE] = Mspti::Common::RegisterFunction("libruntime", "rtInvalidCache");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpy");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY_EX] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpyEx");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY_HOST_TASK] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpyHostTask");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY_ASYNC] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpyAsync");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpy2d");
    g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D_ASYNC] = Mspti::Common::RegisterFunction("libruntime", "rtMemcpy2dAsync");
    g_runtimeMemFuncArray[FUNC_RT_MEMSET] = Mspti::Common::RegisterFunction("libruntime", "rtMemset");
    g_runtimeMemFuncArray[FUNC_RT_MEMSET_ASYNC] = Mspti::Common::RegisterFunction("libruntime", "rtMemsetAsync");
    g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO] = Mspti::Common::RegisterFunction("libruntime", "rtMemGetInfo");
    g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO_EX] = Mspti::Common::RegisterFunction("libruntime", "rtMemGetInfoEx");
    g_runtimeMemFuncArray[FUNC_RT_RESERVE_MEM_ADDRESS] =
        Mspti::Common::RegisterFunction("libruntime", "rtReserveMemAddress");
    g_runtimeMemFuncArray[FUNC_RT_RELEASE_MEM_ADDRESS] =
        Mspti::Common::RegisterFunction("libruntime", "rtReleaseMemAddress");
    g_runtimeMemFuncArray[FUNC_RT_MALLOC_PHYSICAL] = Mspti::Common::RegisterFunction("libruntime", "rtMallocPhysical");
    g_runtimeMemFuncArray[FUNC_RT_FREE_PHYSICAL] = Mspti::Common::RegisterFunction("libruntime", "rtFreePhysical");
    g_runtimeMemFuncArray[FUNC_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction("libruntime", "rtMemExportToShareableHandle");
    g_runtimeMemFuncArray[FUNC_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction("libruntime", "rtMemImportFromShareableHandle");
    g_runtimeMemFuncArray[FUNC_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE] =
        Mspti::Common::RegisterFunction("libruntime", "rtMemSetPidToShareableHandle");
}
}

RtErrorT rtMalloc(void **devPtr, uint64_t size, RtMemTypeT type, const uint16_t moduleId)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC];
    using rtMallocFunc = std::function<decltype(rtMalloc)>;
    rtMallocFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMalloc)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
        MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, type, moduleId);
}

RtErrorT rtFree(void *devPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE];
    using rtFreeFunc = std::function<decltype(rtFree)>;
    rtFreeFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtFree)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(&devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE,
        MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(devPtr);
}

RtErrorT rtMallocHost(void **hostPtr, uint64_t size, const uint16_t moduleId)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_HOST];
    using rtMallocHostFunc = std::function<decltype(rtMallocHost)>;
    rtMallocHostFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMallocHost)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_HOST, __FUNCTION__);
    return func(hostPtr, size, moduleId);
}

RtErrorT rtFreeHost(void *hostPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE_HOST];
    using rtFreeHostFunc = std::function<decltype(rtFreeHost)>;
    rtFreeHostFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtFreeHost)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE_HOST, __FUNCTION__);
    return func(hostPtr);
}

RtErrorT rtMallocCached(void **devPtr, uint64_t size, RtMemTypeT type, const uint16_t moduleId)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_CACHED];
    using rtMallocCachedFunc = std::function<decltype(rtMallocCached)>;
    rtMallocCachedFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMallocCached)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_CACHED, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
        MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, type, moduleId);
}

RtErrorT rtFlushCache(void *base, size_t len)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_FLUSH_CACHE];
    using rtFlushCacheFunc = std::function<decltype(rtFlushCache)>;
    rtFlushCacheFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtFlushCache)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FLUSH_CACHE, __FUNCTION__);
    return func(base, len);
}

RtErrorT rtInvalidCache(void *base, size_t len)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_INVALID_CACHE];
    using rtInvalidCacheFunc = std::function<decltype(rtInvalidCache)>;
    rtInvalidCacheFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtInvalidCache)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_INVALID_CACHE, __FUNCTION__);
    return func(base, len);
}

RtErrorT rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY];
    using rtMemcpyFunc = std::function<decltype(rtMemcpy)>;
    rtMemcpyFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpy)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, cnt, nullptr, 0);
    return func(dst, destMax, src, cnt, kind);
}

RtErrorT rtMemcpyEx(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_EX];
    using rtMemcpyExFunc = std::function<decltype(rtMemcpyEx)>;
    rtMemcpyExFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpyEx)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, cnt, nullptr, 0);
    return func(dst, destMax, src, cnt, kind);
}

RtErrorT rtMemcpyHostTask(void * const dst, const uint64_t destMax, const void * const src, const uint64_t cnt,
    RtMemcpyKindT kind, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_HOST_TASK];
    using rtMemcpyHostTaskFunc = std::function<decltype(rtMemcpyHostTask)>;
    rtMemcpyHostTaskFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpyHostTask)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY_HOST, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, cnt, stm, 0);
    return func(dst, destMax, src, cnt, kind, stm);
}

RtErrorT rtMemcpyAsync(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_ASYNC];
    using rtMemcpyAsyncFunc = std::function<decltype(rtMemcpyAsync)>;
    rtMemcpyAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpyAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEMCPY_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, cnt, stm, 1);
    return func(dst, destMax, src, cnt, kind, stm);
}

RtErrorT rtMemcpy2d(void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width, uint64_t height,
    RtMemcpyKindT kind)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D];
    using rtMemcpy2dFunc = std::function<decltype(rtMemcpy2d)>;
    rtMemcpy2dFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpy2d)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_CPY2D, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, width * height, nullptr, 0);
    return func(dst, dstPitch, src, srcPitch, width, height, kind);
}

RtErrorT rtMemcpy2dAsync(void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width,
    uint64_t height, RtMemcpyKindT kind, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D_ASYNC];
    using rtMemcpy2dAsyncFunc = std::function<decltype(rtMemcpy2dAsync)>;
    rtMemcpy2dAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemcpy2dAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_CPY2D_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemcpyRecord(kind, width * height, stm, 1);
    return func(dst, dstPitch, src, srcPitch, width, height, kind, stm);
}

RtErrorT rtMemset(void *devPtr, uint64_t destMax, uint32_t val, uint64_t cnt)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMSET];
    using rtMemsetFunc = std::function<decltype(rtMemset)>;
    rtMemsetFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemset)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET, __FUNCTION__);
    auto record = Mspti::Reporter::MemsetRecord(val, cnt, nullptr, 0);
    return func(devPtr, destMax, val, cnt);
}

RtErrorT rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t val, uint64_t cnt, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMSET_ASYNC];
    using rtMemsetAsyncFunc = std::function<decltype(rtMemsetAsync)>;
    rtMemsetAsyncFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemsetAsync)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET_ASYNC, __FUNCTION__);
    auto record = Mspti::Reporter::MemsetRecord(val, cnt, stm, 1);
    return func(ptr, destMax, val, cnt, stm);
}

RtErrorT rtMemGetInfo(size_t *freeSize, size_t *totalSize)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO];
    using rtMemGetInfoFunc = std::function<decltype(rtMemGetInfo)>;
    rtMemGetInfoFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemGetInfo)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_GET_INFO, __FUNCTION__);
    return func(freeSize, totalSize);
}

RtErrorT rtMemGetInfoEx(RtMemInfoTypeT memInfoType, size_t *freeSize, size_t *totalSize)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO_EX];
    using rtMemGetInfoExFunc = std::function<decltype(rtMemGetInfoEx)>;
    rtMemGetInfoExFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMemGetInfoEx)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_GET_INFO, __FUNCTION__);
    return func(memInfoType, freeSize, totalSize);
}

RtErrorT rtReserveMemAddress(void **devPtr, size_t size, size_t alignment, void *devAddr, uint64_t flags)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_RESERVE_MEM_ADDRESS];
    using rtReserveMemAddressFunc = std::function<decltype(rtReserveMemAddress)>;
    rtReserveMemAddressFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtReserveMemAddress)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RESERVE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE,
        MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, alignment, devAddr, flags);
}

RtErrorT rtReleaseMemAddress(void *devPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_RELEASE_MEM_ADDRESS];
    using rtReleaseMemAddressFunc = std::function<decltype(rtReleaseMemAddress)>;
    rtReleaseMemAddressFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtReleaseMemAddress)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RELEASE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(&devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE,
        MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(devPtr);
}

RtErrorT rtMallocPhysical(RtDrvMemHandleT **handle, size_t size, RtDrvMemPropT *prop, uint64_t flags)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_PHYSICAL];
    using rtMallocPhysicalFunc = std::function<decltype(rtMallocPhysical)>;
    rtMallocPhysicalFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtMallocPhysical)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_PHYSICAL, __FUNCTION__);
    return func(handle, size, prop, flags);
}

RtErrorT rtFreePhysical(RtDrvMemHandleT *handle)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE_PHYSICAL];
    using rtFreePhysicalFunc = std::function<decltype(rtFreePhysical)>;
    rtFreePhysicalFunc func = Mspti::Common::ReinterpretConvert<decltype(&rtFreePhysical)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE_PHYSICAL, __FUNCTION__);
    return func(handle);
}

RtErrorT rtMemExportToShareableHandle(RtDrvMemHandleT *handle, RtDrvMemHandleType handleType, uint64_t flags,
                                      uint64_t *shareableHandle)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE];
    using rtMemExportToShareableHandleFunc = std::function<decltype(rtMemExportToShareableHandle)>;
    rtMemExportToShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&rtMemExportToShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_EXPORT_TO_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(handle, handleType, flags, shareableHandle);
}

RtErrorT rtMemImportFromShareableHandle(uint64_t shareableHandle, int32_t devId, RtDrvMemHandleT **handle)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE];
    using rtMemImportFromShareableHandleFunc = std::function<decltype(rtMemImportFromShareableHandle)>;
    rtMemImportFromShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&rtMemImportFromShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_IMPORT_FROM_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(shareableHandle, devId, handle);
}

RtErrorT rtMemSetPidToShareableHandle(uint64_t shareableHandle, int pid[], uint32_t pidNum)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void *voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE];
    using rtMemSetPidToShareableHandleFunc = std::function<decltype(rtMemSetPidToShareableHandle)>;
    rtMemSetPidToShareableHandleFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&rtMemSetPidToShareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET_PID_TO_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(shareableHandle, pid, pidNum);
}