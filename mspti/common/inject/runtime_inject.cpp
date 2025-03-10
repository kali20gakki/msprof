/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_inject.cpp
 * Description        : Injection of runtime.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "common/inject/runtime_inject.h"

#include <functional>

#include "activity/activity_manager.h"
#include "callback/callback_manager.h"
#include "common/context_manager.h"
#include "common/function_loader.h"
#include "rt_ffts.h"

namespace {
enum RuntimeFuncIndex {
    FUNC_RT_SET_DEVICE,
    FUNC_RT_DEVICE_RESET,
    FUNC_RT_SET_DEVICE_EX,
    FUNC_RT_GET_DEVICE,
    FUNC_RT_CTX_CREATE_EX,
    FUNC_RT_CTX_CREATE,
    FUNC_RT_CTX_DESTROY,
    FUNC_RT_STREAM_CREATE,
    FUNC_RT_STREAM_DESTROY,
    FUNC_RT_STREAM_SYNCHRONIZE,
    FUNC_RT_GET_STREAM_ID,
    FUNC_RT_PROFILER_TRACE_EX,
    FUNC_RT_KERNEL_LAUNCH,
    FUNC_RT_KERNEL_LAUNCH_WITH_FLAG_V2,
    FUNC_RT_KERNEL_LAUNCH_WITH_HANDLE_V2,
    FUNC_RT_AICPU_KERNEL_LAUNCH_EX_WITH_ARGS,
    FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE,
    FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V2,
    FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH,
    FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH_WITH_HANDLE,
    FUNC_RT_BINARY_GET_FUNCTION,
    FUNC_RT_PROF_SET_PRO_SWITCH,
    FUNC_RT_LAUNCH,
    FUNC_RT_KERNEL_LAUNCH_WITH_FLAG,
    FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V3,
    FUNC_RT_CPU_KERNEL_LAUNCH,
    FUNC_RT_CPU_KERNEL_LAUNCH_WITH_FLAG,
    FUNC_RT_AICPU_KERNEL_LAUNCH_WITH_FLAG,
    FUNC_RT_AICPU_KERNEL_LAUNCH,
    FUNC_RT_FFTS_PLUS_TASK_LAUNCH,
    FUNC_RT_FFTS_TASK_LAUNCH,
    FUNC_RT_FFTS_TASK_LAUNCH_WITH_FLAG,
    FUNC_RT_GET_VISIBLE_DEVICE_ID_BY_LOGIC_DEVICEID,
    FUNC_RUNTIME_COUNT
};

pthread_once_t g_once;
void* g_runtimeFuncArray[FUNC_RUNTIME_COUNT];

void LoadRuntimeFunction()
{
    g_runtimeFuncArray[FUNC_RT_SET_DEVICE] = Mspti::Common::RegisterFunction("libruntime", "rtSetDevice");
    g_runtimeFuncArray[FUNC_RT_DEVICE_RESET] = Mspti::Common::RegisterFunction("libruntime", "rtDeviceReset");
    g_runtimeFuncArray[FUNC_RT_SET_DEVICE_EX] = Mspti::Common::RegisterFunction("libruntime", "rtSetDeviceEx");
    g_runtimeFuncArray[FUNC_RT_GET_DEVICE] = Mspti::Common::RegisterFunction("libruntime", "rtGetDevice");
    g_runtimeFuncArray[FUNC_RT_CTX_CREATE_EX] = Mspti::Common::RegisterFunction("libruntime", "rtCtxCreateEx");
    g_runtimeFuncArray[FUNC_RT_CTX_CREATE] = Mspti::Common::RegisterFunction("libruntime", "rtCtxCreate");
    g_runtimeFuncArray[FUNC_RT_CTX_DESTROY] = Mspti::Common::RegisterFunction("libruntime", "rtCtxDestroy");
    g_runtimeFuncArray[FUNC_RT_STREAM_CREATE] = Mspti::Common::RegisterFunction("libruntime", "rtStreamCreate");
    g_runtimeFuncArray[FUNC_RT_STREAM_DESTROY] = Mspti::Common::RegisterFunction("libruntime", "rtStreamDestroy");
    g_runtimeFuncArray[FUNC_RT_STREAM_SYNCHRONIZE] =
        Mspti::Common::RegisterFunction("libruntime", "rtStreamSynchronize");
    g_runtimeFuncArray[FUNC_RT_GET_STREAM_ID] = Mspti::Common::RegisterFunction("libruntime", "rtGetStreamId");
    g_runtimeFuncArray[FUNC_RT_PROFILER_TRACE_EX] = Mspti::Common::RegisterFunction("libruntime", "rtProfilerTraceEx");
    g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH] = Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunch");
    g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_FLAG_V2] =
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithFlagV2");
    g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_HANDLE_V2] =
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithHandleV2");
    g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH_EX_WITH_ARGS] =
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunchExWithArgs");
    g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE] =
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandle");
    g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V2] =
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandleV2");
    g_runtimeFuncArray[FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH] =
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunch");
    g_runtimeFuncArray[FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH_WITH_HANDLE] =
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunchWithHandle");
    g_runtimeFuncArray[FUNC_RT_BINARY_GET_FUNCTION] =
        Mspti::Common::RegisterFunction("libruntime", "rtBinaryGetFunction");
    g_runtimeFuncArray[FUNC_RT_PROF_SET_PRO_SWITCH] =
        Mspti::Common::RegisterFunction("libruntime", "rtProfSetProSwitch");
    g_runtimeFuncArray[FUNC_RT_LAUNCH] = Mspti::Common::RegisterFunction("libruntime", "rtLaunch");
    g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_FLAG] =
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithFlag");
    g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V3] =
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandleV3");
    g_runtimeFuncArray[FUNC_RT_CPU_KERNEL_LAUNCH] = Mspti::Common::RegisterFunction("libruntime", "rtCpuKernelLaunch");
    g_runtimeFuncArray[FUNC_RT_CPU_KERNEL_LAUNCH_WITH_FLAG] =
        Mspti::Common::RegisterFunction("libruntime", "rtCpuKernelLaunchWithFlag");
    g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH_WITH_FLAG] =
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunchWithFlag");
    g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH] =
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunch");
    g_runtimeFuncArray[FUNC_RT_FFTS_PLUS_TASK_LAUNCH] =
        Mspti::Common::RegisterFunction("libruntime", "rtFftsPlusTaskLaunch");
    g_runtimeFuncArray[FUNC_RT_FFTS_TASK_LAUNCH] = Mspti::Common::RegisterFunction("libruntime", "rtFftsTaskLaunch");
    g_runtimeFuncArray[FUNC_RT_FFTS_TASK_LAUNCH_WITH_FLAG] =
        Mspti::Common::RegisterFunction("libruntime", "rtFftsTaskLaunchWithFlag");
    g_runtimeFuncArray[FUNC_RT_GET_VISIBLE_DEVICE_ID_BY_LOGIC_DEVICEID] =
        Mspti::Common::RegisterFunction("libruntime", "rtGetVisibleDeviceIdByLogicDeviceId");
}
}

RtErrorT rtSetDevice(int32_t device)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_SET_DEVICE];
    using rtSetDeviceFunc = std::function<RtErrorT(int32_t)>;
    rtSetDeviceFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(int32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_SET, __FUNCTION__);
    auto ret = func(device);
    if (ret == MSPTI_SUCCESS) {
        int32_t visibleDevice = 0;
        uint32_t realDevice = rtGetVisibleDeviceIdByLogicDeviceId(device, &visibleDevice) == MSPTI_SUCCESS ?
            static_cast<uint32_t>(visibleDevice) : static_cast<uint32_t>(device);
        Mspti::Activity::ActivityManager::GetInstance()->SetDevice(realDevice);
    }
    return ret;
}

RtErrorT rtDeviceReset(int32_t device)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_DEVICE_RESET];
    using rtDeviceResetFunc = std::function<RtErrorT(int32_t)>;
    rtDeviceResetFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(int32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    int32_t visibleDevice = 0;
    uint32_t realDevice = rtGetVisibleDeviceIdByLogicDeviceId(device, &visibleDevice) == MSPTI_SUCCESS ?
        static_cast<uint32_t>(visibleDevice) : static_cast<uint32_t>(device);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_RESET, __FUNCTION__);
    return func(device);
}

RtErrorT rtSetDeviceEx(int32_t device)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_SET_DEVICE_EX];
    using rtSetDeviceExFunc = std::function<RtErrorT(int32_t)>;
    rtSetDeviceExFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(int32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_SET_EX, __FUNCTION__);
    auto ret = func(device);
    if (ret == MSPTI_SUCCESS) {
        int32_t visibleDevice = 0;
        uint32_t realDevice = rtGetVisibleDeviceIdByLogicDeviceId(device, &visibleDevice) == MSPTI_SUCCESS ?
            static_cast<uint32_t>(visibleDevice) : static_cast<uint32_t>(device);
        Mspti::Activity::ActivityManager::GetInstance()->SetDevice(realDevice);
    }
    return ret;
}

RtErrorT rtGetDevice(int32_t* devId)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_GET_DEVICE];
    using rtGetDeviceFunc = std::function<RtErrorT(int32_t*)>;
    rtGetDeviceFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(int32_t*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, int32_t*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(devId);
}

RtErrorT rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_CTX_CREATE_EX];
    using rtCtxCreateExFunc = std::function<RtErrorT(void **, uint32_t, int32_t)>;
    rtCtxCreateExFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **, uint32_t, int32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, uint32_t, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, __FUNCTION__);
    return func(ctx, flags, device);
}

RtErrorT rtCtxCreate(void **ctx, uint32_t flags, int32_t device)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_CTX_CREATE];
    using rtCtxCreateFunc = std::function<RtErrorT(void **, uint32_t, int32_t)>;
    rtCtxCreateFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **ctx, uint32_t flags,
        int32_t device)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **, uint32_t, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED, __FUNCTION__);
    return func(ctx, flags, device);
}

RtErrorT rtCtxDestroy(void **ctx)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_CTX_DESTROY];
    using rtCtxDestroyFunc = std::function<RtErrorT(void **)>;
    rtCtxDestroyFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void **ctx)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void **>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY, __FUNCTION__);
    return func(ctx);
}

RtErrorT rtStreamCreate(RtStreamT* stream, int32_t priority)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_STREAM_CREATE];
    using rtStreamCreateFunc = std::function<RtErrorT(RtStreamT*, int32_t)>;
    rtStreamCreateFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtStreamT*, int32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtStreamT*, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_CREATED, __FUNCTION__);
    return func(stream, priority);
}

RtErrorT rtStreamDestroy(RtStreamT stream)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_STREAM_DESTROY];
    using rtStreamDestroyFunc = std::function<RtErrorT(RtStreamT)>;
    rtStreamDestroyFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_DESTROY, __FUNCTION__);
    return func(stream);
}

RtErrorT rtStreamSynchronize(RtStreamT stream)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_STREAM_SYNCHRONIZE];
    using rtStreamSynchronizeFunc = std::function<RtErrorT(RtStreamT)>;
    rtStreamSynchronizeFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED,
        __FUNCTION__);
    return func(stream);
}

RtErrorT rtGetStreamId(RtStreamT stm, int32_t *streamId)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_GET_STREAM_ID];
    using rtGetStreamIdFunc = std::function<RtErrorT(RtStreamT, int32_t*)>;
    rtGetStreamIdFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtStreamT, int32_t*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtStreamT, int32_t*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(stm, streamId);
}


RtErrorT rtProfilerTraceEx(uint64_t id, uint64_t eventType, uint16_t tagId, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_PROFILER_TRACE_EX];
    using rtProfilerTraceExFunc = std::function<RtErrorT(uint64_t, uint64_t, uint16_t, RtStreamT)>;
    rtProfilerTraceExFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(uint64_t, uint64_t, uint16_t,
        RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, uint64_t, uint64_t, uint16_t, RtStreamT>("libruntime",
                                                                                      __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(id, eventType, tagId, stm);
}

RtErrorT rtKernelLaunch(const void* stubFunc, uint32_t blockDim, void* args,
                        uint32_t argsSize, RtSmDescT* smDesc, RtStreamT stream)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH];
    using rtKernelLaunchFunc = std::function<RtErrorT(const void*, uint32_t, void*, uint32_t,
                                                      RtSmDescT*, RtStreamT)>;
    rtKernelLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void*, uint32_t, void*, uint32_t,
                                                            RtSmDescT*, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void*, uint32_t, void*, uint32_t,
            RtSmDescT*, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, args, argsSize, smDesc, stream);
}

RtErrorT rtKernelLaunchWithFlagV2(const void* stubFunc, uint32_t blockDim, RtArgsExT* argsInfo, RtSmDescT* smDesc,
                                  RtStreamT stream, uint32_t flags, const RtTaskCfgInfoT* cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_FLAG_V2];
    using rtKernelLaunchWithFlagV2Func = std::function<RtErrorT(const void*, uint32_t, RtArgsExT*,
                                                                RtSmDescT*, RtStreamT, uint32_t,
                                                                const RtTaskCfgInfoT*)>;
    rtKernelLaunchWithFlagV2Func func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void*, uint32_t,
        RtArgsExT*, RtSmDescT*, RtStreamT, uint32_t, const RtTaskCfgInfoT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void*, uint32_t, RtArgsExT*, RtSmDescT*, RtStreamT,
            uint32_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, argsInfo, smDesc, stream, flags, cfgInfo);
}

RtErrorT rtKernelLaunchWithHandleV2(void *handle, const uint64_t tilingkey, uint32_t blockDim, RtArgsExT *argsInfo,
                                    RtSmDescT *smDesc, RtStreamT stream, const RtTaskCfgInfoT *cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_HANDLE_V2];
    using rtKernelLaunchWithHandleV2Func = std::function<RtErrorT(void*, uint64_t, uint32_t, RtArgsExT*,
                                                                  RtSmDescT*, RtStreamT, const RtTaskCfgInfoT*)>;
    rtKernelLaunchWithHandleV2Func func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(void*, uint64_t, uint32_t,
        RtArgsExT*, RtSmDescT*, RtStreamT, const RtTaskCfgInfoT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, void*, uint64_t, uint32_t, RtArgsExT*, RtSmDescT*,
            RtStreamT, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(handle, tilingkey, blockDim, argsInfo, smDesc, stream, cfgInfo);
}

RtErrorT rtAicpuKernelLaunchExWithArgs(const uint32_t kernelType, const char* const opName,
                                       const uint32_t blockDim, const RtAicpuArgsExT *argsInfo,
                                       RtSmDescT * const smDesc, const RtStreamT stm, const uint32_t flags)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH_EX_WITH_ARGS];
    using rtAicpuKernelLaunchExWithArgsFunc = std::function<RtErrorT(uint32_t, const char*, uint32_t,
                                                                     const RtAicpuArgsExT*, RtSmDescT*,
                                                                     const RtStreamT, uint32_t)>;
    rtAicpuKernelLaunchExWithArgsFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(uint32_t, const char*,
        uint32_t, const RtAicpuArgsExT*, RtSmDescT*, const RtStreamT, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, uint32_t, const char*, uint32_t, const RtAicpuArgsExT*,
            RtSmDescT*, RtStreamT, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    return func(kernelType, opName, blockDim, argsInfo, smDesc, stm, flags);
}

RtErrorT rtLaunchKernelByFuncHandle(rtFuncHandle funcHandle, uint32_t blockDim, rtLaunchArgsHandle argsHandle,
                                    RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE];
    using rtLaunchKernelByFuncHandleFunc = std::function<RtErrorT(rtFuncHandle, uint32_t, rtLaunchArgsHandle,
                                                                  RtStreamT)>;
    rtLaunchKernelByFuncHandleFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(rtFuncHandle, uint32_t,
        rtLaunchArgsHandle, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, rtFuncHandle, uint32_t, rtLaunchArgsHandle, RtStreamT>("libruntime",
                                                                                                    __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsHandle, stm);
}

RtErrorT rtLaunchKernelByFuncHandleV2(rtFuncHandle funcHandle, uint32_t blockDim, rtLaunchArgsHandle argsHandle,
                                      RtStreamT stm, const RtTaskCfgInfoT *cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V2];
    using rtLaunchKernelByFuncHandleV2Func = std::function<RtErrorT(rtFuncHandle, uint32_t, rtLaunchArgsHandle,
                                                                    RtStreamT, const RtTaskCfgInfoT*)>;
    rtLaunchKernelByFuncHandleV2Func func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(rtFuncHandle, uint32_t,
                                                                          rtLaunchArgsHandle, RtStreamT,
                                                                          const RtTaskCfgInfoT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, rtFuncHandle, uint32_t, rtLaunchArgsHandle, RtStreamT,
            const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    return func(funcHandle, blockDim, argsHandle, stm, cfgInfo);
}

RtErrorT rtVectorCoreKernelLaunch(const VOID_PTR stubFunc, uint32_t blockDim, RtArgsExT *argsInfo,
                                  RtSmDescT *smDesc, RtStreamT stm, uint32_t flags, const RtTaskCfgInfoT *cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH];
    using rtVectorCoreKernelLaunchFunc = std::function<RtErrorT(VOID_PTR, uint32_t, RtArgsExT*,
                                                                RtSmDescT*, RtStreamT, uint32_t,
                                                                const RtTaskCfgInfoT*)>;
    rtVectorCoreKernelLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(VOID_PTR, uint32_t, RtArgsExT*,
                                                                      RtSmDescT*, RtStreamT, uint32_t,
                                                                      const RtTaskCfgInfoT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, VOID_PTR, uint32_t, RtArgsExT*, RtSmDescT*, RtStreamT,
            uint32_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AIV_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    return func(stubFunc, blockDim, argsInfo, smDesc, stm, flags, cfgInfo);
}

RtErrorT rtVectorCoreKernelLaunchWithHandle(VOID_PTR hdl, const uint64_t tilingKey, uint32_t blockDim,
                                            RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stm,
                                            const RtTaskCfgInfoT *cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_VECTOR_CORE_KERNEL_LAUNCH_WITH_HANDLE];
    using rtVectorCoreKernelLaunchWithHandleFunc = std::function<RtErrorT(VOID_PTR, uint64_t, uint32_t,
                                                                          RtArgsExT*, RtSmDescT*, RtStreamT,
                                                                          const RtTaskCfgInfoT*)>;
    rtVectorCoreKernelLaunchWithHandleFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(VOID_PTR, uint64_t,
        uint32_t, RtArgsExT*, RtSmDescT*, RtStreamT, const RtTaskCfgInfoT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, VOID_PTR, uint64_t, uint32_t, RtArgsExT*, RtSmDescT*,
            RtStreamT, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AIV_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    return func(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, cfgInfo);
}

RtErrorT rtBinaryGetFunction(VOID_PTR binHandle, uint64_t tilingKey, VOID_PTR_PTR funcHandle)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_BINARY_GET_FUNCTION];
    using rtBinaryGetFunctionFunc = std::function<RtErrorT(VOID_PTR, uint64_t, VOID_PTR_PTR)>;
    rtBinaryGetFunctionFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(VOID_PTR, uint64_t,
        VOID_PTR_PTR)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, VOID_PTR, uint64_t, VOID_PTR_PTR>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(binHandle, tilingKey, funcHandle);
}

RtErrorT rtProfSetProSwitch(VOID_PTR data, uint32_t len)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_PROF_SET_PRO_SWITCH];
    using rtProfSetProSwitchFunc = std::function<RtErrorT(VOID_PTR, uint32_t)>;
    rtProfSetProSwitchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(VOID_PTR, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, VOID_PTR, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(data, len);
}

RtErrorT rtGetVisibleDeviceIdByLogicDeviceId(const int32_t logicDeviceId, int32_t * const visibleDeviceId)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_GET_VISIBLE_DEVICE_ID_BY_LOGIC_DEVICEID];
    using rtGetVisibleDeviceIdByLogicDeviceIdFunc = std::function<RtErrorT(int32_t, int32_t *)>;
    rtGetVisibleDeviceIdByLogicDeviceIdFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(int32_t,
        int32_t *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, int32_t, int32_t *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(logicDeviceId, visibleDeviceId);
}

RtErrorT rtLaunch(const void *stubFunc)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_LAUNCH];
    using rtLaunchFunc = std::function<RtErrorT(const void *)>;
    rtLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc);
}

RtErrorT rtKernelLaunchWithFlag(const void *stubFunc, uint32_t blockDim, RtArgsExT *argsInfo, RtSmDescT *smDesc,
                                RtStreamT stm, uint32_t flags)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_KERNEL_LAUNCH_WITH_FLAG];
    using rtKernelLaunchWithFlagFunc = std::function<RtErrorT(const void *, uint32_t, RtArgsExT *, RtSmDescT *,
                                                              RtStreamT, uint32_t)>;
    rtKernelLaunchWithFlagFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void *, uint32_t,
        RtArgsExT *, RtSmDescT *, RtStreamT, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void *, uint32_t, RtArgsExT *, RtSmDescT *, RtStreamT,
            uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, argsInfo, smDesc, stm, flags);
}

RtErrorT rtLaunchKernelByFuncHandleV3(rtFuncHandle funcHandle, uint32_t blockDim, const RtArgsExT * const argsInfo,
                                      RtStreamT stm, const RtTaskCfgInfoT * const cfgInfo)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_LAUNCH_KERNEL_BY_FUNC_HANDLE_V3];
    using rtLaunchKernelByFuncHandleV3Func = std::function<RtErrorT(rtFuncHandle, uint32_t, const RtArgsExT *,
                                                                    RtStreamT, const RtTaskCfgInfoT *)>;
    rtLaunchKernelByFuncHandleV3Func func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(rtFuncHandle, uint32_t,
        const RtArgsExT *, RtStreamT, const RtTaskCfgInfoT *)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, rtFuncHandle, uint32_t, const RtArgsExT *, RtStreamT,
            const RtTaskCfgInfoT *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsInfo, stm, cfgInfo);
}
    
RtErrorT rtCpuKernelLaunch(const void *soName, const void *kernelName, uint32_t blockDim, const void *args,
                           uint32_t argsSize, RtSmDescT *smDesc, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_CPU_KERNEL_LAUNCH];
    using rtCpuKernelLaunchFunc = std::function<RtErrorT(const void *, const void *, uint32_t, const void *,
                                                         uint32_t, RtSmDescT *, RtStreamT)>;
    rtCpuKernelLaunchFunc func =  Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void *, const void *, uint32_t,
        const void *, uint32_t, RtSmDescT *, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void *, const void *, uint32_t, const void *, uint32_t,
            RtSmDescT *, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CPU_LAUNCH, __FUNCTION__);
    return func(soName, kernelName, blockDim, args, argsSize, smDesc, stm);
}

RtErrorT rtCpuKernelLaunchWithFlag(const void *soName, const void *kernelName, uint32_t blockDim,
                                   const RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stm, uint32_t flags)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_CPU_KERNEL_LAUNCH_WITH_FLAG];
    using rtCpuKernelLaunchWithFlagFunc =
            std::function<RtErrorT(const void *, const void *, uint32_t, const RtArgsExT *, RtSmDescT *,
                                   RtStreamT, uint32_t)>;
    rtCpuKernelLaunchWithFlagFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const void *, const void *,
        uint32_t, const RtArgsExT *, RtSmDescT *, RtStreamT, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const void *, const void *, uint32_t, const RtArgsExT *, RtSmDescT *,
            RtStreamT, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CPU_LAUNCH, __FUNCTION__);
    return func(soName, kernelName, blockDim, argsInfo, smDesc, stm, flags);
}

RtErrorT rtAicpuKernelLaunchWithFlag(const RtKernelLaunchNamesT *launchNames, uint32_t blockDim,
                                     const RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stm, uint32_t flags)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH_WITH_FLAG];
    using rtAicpuKernelLaunchWithFlagFunc =
            std::function<RtErrorT(const RtKernelLaunchNamesT *, uint32_t, const RtArgsExT *, RtSmDescT *,
                                   RtStreamT, uint32_t)>;
    rtAicpuKernelLaunchWithFlagFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const RtKernelLaunchNamesT *,
        uint32_t, const RtArgsExT *, RtSmDescT *, RtStreamT, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const RtKernelLaunchNamesT *, uint32_t, const RtArgsExT *, RtSmDescT *,
            RtStreamT, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    return func(launchNames, blockDim, argsInfo, smDesc, stm, flags);
}

RtErrorT rtAicpuKernelLaunch(const RtKernelLaunchNamesT *launchNames, uint32_t blockDim, const void *args,
                             uint32_t argsSize, RtSmDescT *smDesc, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_AICPU_KERNEL_LAUNCH];
    using rtAicpuKernelLaunchFunc = std::function<RtErrorT(const RtKernelLaunchNamesT *, uint32_t, const void *,
                                                           uint32_t, RtSmDescT *, RtStreamT)>;
    rtAicpuKernelLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(const RtKernelLaunchNamesT *,
        uint32_t, const void *, uint32_t, RtSmDescT *, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, const RtKernelLaunchNamesT *, uint32_t, const void *, uint32_t,
            RtSmDescT *, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    return func(launchNames, blockDim, args, argsSize, smDesc, stm);
}

RtErrorT rtFftsPlusTaskLaunch(RtFftsPlusTaskInfoT *fftsPlusTaskInfo, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_FFTS_PLUS_TASK_LAUNCH];
    using rtFftsPlusTaskLaunchFunc = std::function<RtErrorT(RtFftsPlusTaskInfoT *, RtStreamT)>;
    rtFftsPlusTaskLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtFftsPlusTaskInfoT *,
        RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtFftsPlusTaskInfoT *, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsPlusTaskInfo, stm);
}

RtErrorT rtFftsTaskLaunch(RtFftsTaskInfoT *fftsTaskInfo, RtStreamT stm)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_FFTS_TASK_LAUNCH];
    using rtFftsTaskLaunchFunc = std::function<RtErrorT(RtFftsTaskInfoT *, RtStreamT)>;
    rtFftsTaskLaunchFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtFftsTaskInfoT *, RtStreamT)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtFftsTaskInfoT *, RtStreamT>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsTaskInfo, stm);
}

RtErrorT rtFftsTaskLaunchWithFlag(RtFftsTaskInfoT *fftsTaskInfo, RtStreamT stm, uint32_t flag)
{
    pthread_once(&g_once, LoadRuntimeFunction);
    void* voidFunc = g_runtimeFuncArray[FUNC_RT_FFTS_TASK_LAUNCH_WITH_FLAG];
    using rtFftsTaskLaunchWithFlagFunc = std::function<RtErrorT(RtFftsTaskInfoT *, RtStreamT, uint32_t)>;
    rtFftsTaskLaunchWithFlagFunc func = Mspti::Common::ReinterpretConvert<RtErrorT (*)(RtFftsTaskInfoT *, RtStreamT,
        uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<RtErrorT, RtFftsTaskInfoT *, RtStreamT,
            uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsTaskInfo, stm, flag);
}
