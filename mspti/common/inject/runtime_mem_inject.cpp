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

pthread_once_t g_once;
void* g_runtimeMemFuncArray[FUNC_COUNT];

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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC];
    using rtMallocFunc = std::function<RtErrorT(void **, uint64_t, RtMemTypeT, uint16_t)>;
    rtMallocFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **, uint64_t, RtMemTypeT,
        uint16_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, uint64_t, RtMemTypeT, uint16_t>("libruntime", __FUNCTION__,
                                                                                      func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(
        devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, type, moduleId);
}

RtErrorT rtFree(void *devPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE];
    using rtFreeFunc = std::function<RtErrorT(void *)>;
    rtFreeFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(
        &devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(devPtr);
}

RtErrorT rtMallocHost(void **hostPtr, uint64_t size, const uint16_t moduleId)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_HOST];
    using rtMallocHostFunc = std::function<RtErrorT(void **, uint64_t, uint16_t)>;
    rtMallocHostFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **, uint64_t, uint16_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, uint64_t, uint16_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_HOST, __FUNCTION__);
    return func(hostPtr, size, moduleId);
}

RtErrorT rtFreeHost(void *hostPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE_HOST];
    using rtFreeHostFunc = std::function<RtErrorT(void *)>;
    rtFreeHostFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FREE_HOST, __FUNCTION__);
    return func(hostPtr);
}

RtErrorT rtMallocCached(void **devPtr, uint64_t size, RtMemTypeT type, const uint16_t moduleId)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_CACHED];
    using rtMallocCachedFunc = std::function<RtErrorT(void **, uint64_t, RtMemTypeT, uint16_t)>;
    rtMallocCachedFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **, uint64_t, RtMemTypeT,
        uint16_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, uint64_t, RtMemTypeT, uint16_t>("libruntime", __FUNCTION__,
                                                                                      func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_CACHED, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(
        devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, type, moduleId);
}

RtErrorT rtFlushCache(void *base, size_t len)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_FLUSH_CACHE];
    using rtFlushCacheFunc = std::function<RtErrorT(void *, size_t)>;
    rtFlushCacheFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, size_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, size_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FLUSH_CACHE, __FUNCTION__);
    return func(base, len);
}

RtErrorT rtInvalidCache(void *base, size_t len)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_INVALID_CACHE];
    using rtInvalidCacheFunc = std::function<RtErrorT(void *, size_t)>;
    rtInvalidCacheFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, size_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, size_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_INVALID_CACHE, __FUNCTION__);
    return func(base, len);
}

RtErrorT rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t cnt, RtMemcpyKindT kind)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY];
    using rtMemcpyFunc = std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, RtMemcpyKindT)>;
    rtMemcpyFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
        RtMemcpyKindT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t,
        RtMemcpyKindT>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_EX];
    using rtMemcpyExFunc = std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, RtMemcpyKindT)>;
    rtMemcpyExFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
        RtMemcpyKindT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t,
        RtMemcpyKindT>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_HOST_TASK];
    using rtMemcpyHostTaskFunc =
        std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, RtMemcpyKindT, RtStreamT)>;
    rtMemcpyHostTaskFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
                                                              RtMemcpyKindT, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t, RtMemcpyKindT, RtStreamT>(
            "libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_ASYNC];
    using rtMemcpyAsyncFunc =
        std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, RtMemcpyKindT, RtStreamT)>;
    rtMemcpyAsyncFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
        RtMemcpyKindT, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t, RtMemcpyKindT, RtStreamT>(
            "libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D];
    using rtMemcpy2dFunc =
        std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, uint64_t, uint64_t, RtMemcpyKindT)>;
    rtMemcpy2dFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
        uint64_t, uint64_t, RtMemcpyKindT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t, uint64_t, uint64_t,
            RtMemcpyKindT>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMCPY_2D_ASYNC];
    using rtMemcpy2dAsyncFunc = std::function<RtErrorT(void *, uint64_t, const void *, uint64_t, uint64_t, uint64_t,
                                                       RtMemcpyKindT, RtStreamT)>;
    rtMemcpy2dAsyncFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, const void *, uint64_t,
        uint64_t, uint64_t, RtMemcpyKindT, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, const void *, uint64_t, uint64_t, uint64_t,
            RtMemcpyKindT, RtStreamT>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMSET];
    using rtMemsetFunc = std::function<RtErrorT(void *, uint64_t, uint32_t, uint64_t)>;
    rtMemsetFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, uint32_t, uint64_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, uint32_t, uint64_t>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEMSET_ASYNC];
    using rtMemsetAsyncFunc = std::function<RtErrorT(void *, uint64_t, uint32_t, uint64_t, RtStreamT)>;
    rtMemsetAsyncFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *, uint64_t, uint32_t, uint64_t,
        RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *, uint64_t, uint32_t, uint64_t, RtStreamT>("libruntime",
                                                                                              __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO];
    using rtMemGetInfoFunc = std::function<RtErrorT(size_t *, size_t *)>;
    rtMemGetInfoFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(size_t *, size_t *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, size_t *, size_t *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_GET_INFO, __FUNCTION__);
    return func(freeSize, totalSize);
}

RtErrorT rtMemGetInfoEx(RtMemInfoTypeT memInfoType, size_t *freeSize, size_t *totalSize)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_GET_INFO_EX];
    using rtMemGetInfoExFunc = std::function<RtErrorT(RtMemInfoTypeT, size_t *, size_t *)>;
    rtMemGetInfoExFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtMemInfoTypeT, size_t *,
        size_t *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtMemInfoTypeT, size_t *, size_t *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_GET_INFO, __FUNCTION__);
    return func(memInfoType, freeSize, totalSize);
}

RtErrorT rtReserveMemAddress(void **devPtr, size_t size, size_t alignment, void *devAddr, uint64_t flags)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_RESERVE_MEM_ADDRESS];
    using rtReserveMemAddressFunc = std::function<RtErrorT(void **, size_t, size_t, void *, uint64_t)>;
    rtReserveMemAddressFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **, size_t, size_t, void *,
        uint64_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, size_t, size_t, void *, uint64_t>("libruntime", __FUNCTION__,
                                                                                        func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RESERVE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(
        devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
    return func(devPtr, size, alignment, devAddr, flags);
}

RtErrorT rtReleaseMemAddress(void *devPtr)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_RELEASE_MEM_ADDRESS];
    using rtReleaseMemAddressFunc = std::function<RtErrorT(void *)>;
    rtReleaseMemAddressFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_RELEASE_MEM_ADDRESS, __FUNCTION__);
    auto record = Mspti::Reporter::MemoryRecord(
        &devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
    return func(devPtr);
}

RtErrorT rtMallocPhysical(RtDrvMemHandleT **handle, size_t size, RtDrvMemPropT *prop, uint64_t flags)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MALLOC_PHYSICAL];
    using rtMallocPhysicalFunc = std::function<RtErrorT(RtDrvMemHandleT **, size_t, RtDrvMemPropT *, uint64_t)>;
    rtMallocPhysicalFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtDrvMemHandleT **, size_t,
        RtDrvMemPropT *, uint64_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtDrvMemHandleT **, size_t, RtDrvMemPropT *,
        uint64_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MALLOC_PHYSICAL, __FUNCTION__);
    return func(handle, size, prop, flags);
}

RtErrorT rtFreePhysical(RtDrvMemHandleT *handle)
{
    pthread_once(&g_once, LoadRuntimeMemFunction);
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_FREE_PHYSICAL];
    using rtFreePhysicalFunc = std::function<RtErrorT(RtDrvMemHandleT *)>;
    rtFreePhysicalFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtDrvMemHandleT *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtDrvMemHandleT *>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_EXPORT_TO_SHAREABLE_HANDLE];
    using rtMemExportToShareableHandleFunc =
        std::function<RtErrorT(RtDrvMemHandleT *, RtDrvMemHandleType, uint64_t, uint64_t *)>;
    rtMemExportToShareableHandleFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtDrvMemHandleT *handle,
        RtDrvMemHandleType handleType, uint64_t flags, uint64_t *shareableHandle)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtDrvMemHandleT *, RtDrvMemHandleType, uint64_t, uint64_t *>(
            "libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_IMPORT_FROM_SHAREABLE_HANDLE];
    using rtMemImportFromShareableHandleFunc = std::function<RtErrorT(uint64_t, int32_t, RtDrvMemHandleT **)>;
    rtMemImportFromShareableHandleFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(uint64_t, int32_t,
        RtDrvMemHandleT **)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, uint64_t, int32_t, RtDrvMemHandleT **>("libruntime", __FUNCTION__, func);
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
    void* voidFunc = g_runtimeMemFuncArray[FUNC_RT_MEM_SET_PID_TO_SHAREABLE_HANDLE];
    using rtMemSetPidToShareableHandleFunc = std::function<RtErrorT(uint64_t, int *, uint32_t)>;
    rtMemSetPidToShareableHandleFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(uint64_t, int *,
        uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, uint64_t, int *, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_MEM_SET_PID_TO_SHAREABLE_HANDLE,
                                         __FUNCTION__);
    return func(shareableHandle, pid, pidNum);
}