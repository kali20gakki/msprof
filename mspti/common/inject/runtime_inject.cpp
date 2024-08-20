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

class RuntimeInject {
public:
    RuntimeInject() noexcept
    {
        Mspti::Common::RegisterFunction("libruntime", "rtSetDevice");
        Mspti::Common::RegisterFunction("libruntime", "rtDeviceReset");
        Mspti::Common::RegisterFunction("libruntime", "rtSetDeviceEx");
        Mspti::Common::RegisterFunction("libruntime", "rtGetDevice");
        Mspti::Common::RegisterFunction("libruntime", "rtCtxCreateEx");
        Mspti::Common::RegisterFunction("libruntime", "rtCtxCreate");
        Mspti::Common::RegisterFunction("libruntime", "rtCtxDestroy");
        Mspti::Common::RegisterFunction("libruntime", "rtStreamCreate");
        Mspti::Common::RegisterFunction("libruntime", "rtStreamDestroy");
        Mspti::Common::RegisterFunction("libruntime", "rtStreamSynchronize");
        Mspti::Common::RegisterFunction("libruntime", "rtGetStreamId");
        Mspti::Common::RegisterFunction("libruntime", "rtProfilerTraceEx");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithFlagV2");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithHandleV2");
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunchExWithArgs");
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandle");
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandleV2");
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunchWithHandle");
        Mspti::Common::RegisterFunction("libruntime", "rtBinaryGetFunction");
        Mspti::Common::RegisterFunction("libruntime", "rtProfSetProSwitch");
        Mspti::Common::RegisterFunction("libruntime", "rtLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithHandle");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithHandleV2");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithFlag");
        Mspti::Common::RegisterFunction("libruntime", "rtKernelLaunchWithFlagV2");
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandleV2");
        Mspti::Common::RegisterFunction("libruntime", "rtLaunchKernelByFuncHandleV3");
        Mspti::Common::RegisterFunction("libruntime", "rtCpuKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtCpuKernelLaunchWithFlag");
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunchWithFlag");
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtAicpuKernelLaunchExWithArgs");
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtVectorCoreKernelLaunchWithHandle");
        Mspti::Common::RegisterFunction("libruntime", "rtFftsPlusTaskLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtFftsTaskLaunch");
        Mspti::Common::RegisterFunction("libruntime", "rtFftsTaskLaunchWithFlag");
        Mspti::Common::RegisterFunction("libruntime", "rtGetVisibleDeviceIdByLogicDeviceId");
    }
    ~RuntimeInject() = default;
};

RuntimeInject g_rtInject;

rtError_t rtSetDevice(int32_t device)
{
    using rtSetDeviceFunc = std::function<rtError_t(int32_t)>;
    static rtSetDeviceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", __FUNCTION__, func);
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

rtError_t rtDeviceReset(int32_t device)
{
    using rtDeviceResetFunc = std::function<rtError_t(int32_t)>;
    static rtDeviceResetFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    int32_t visibleDevice = 0;
    uint32_t realDevice = rtGetVisibleDeviceIdByLogicDeviceId(device, &visibleDevice) == MSPTI_SUCCESS ?
        static_cast<uint32_t>(visibleDevice) : static_cast<uint32_t>(device);
    Mspti::Activity::ActivityManager::GetInstance()->DeviceReset(realDevice);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_RESET, __FUNCTION__);
    return func(device);
}

rtError_t rtSetDeviceEx(int32_t device)
{
    using rtSetDeviceExFunc = std::function<rtError_t(int32_t)>;
    static rtSetDeviceExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", __FUNCTION__, func);
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

rtError_t rtGetDevice(int32_t* devId)
{
    using rtGetDeviceFunc = std::function<rtError_t(int32_t*)>;
    static rtGetDeviceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(devId);
}

rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device)
{
    using rtCtxCreateExFunc = std::function<rtError_t(void **, uint32_t, int32_t)>;
    static rtCtxCreateExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **, uint32_t, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, __FUNCTION__);
    return func(ctx, flags, device);
}

rtError_t rtCtxCreate(void **ctx, int32_t device)
{
    using rtCtxCreateFunc = std::function<rtError_t(void **, int32_t)>;
    static rtCtxCreateFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED, __FUNCTION__);
    return func(ctx, device);
}

rtError_t rtCtxDestroy(void **ctx)
{
    using rtCtxDestroyFunc = std::function<rtError_t(void **)>;
    static rtCtxDestroyFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY, __FUNCTION__);
    return func(ctx);
}

rtError_t rtStreamCreate(rtStream_t* stream, int32_t priority)
{
    using rtStreamCreateFunc = std::function<rtError_t(rtStream_t*, int32_t)>;
    static rtStreamCreateFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t*, int32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_CREATED, __FUNCTION__);
    return func(stream, priority);
}

rtError_t rtStreamDestroy(rtStream_t stream)
{
    using rtStreamDestroyFunc = std::function<rtError_t(rtStream_t)>;
    static rtStreamDestroyFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_DESTROY, __FUNCTION__);
    return func(stream);
}

rtError_t rtStreamSynchronize(rtStream_t stream)
{
    using rtStreamSynchronizeFunc = std::function<rtError_t(rtStream_t)>;
    static rtStreamSynchronizeFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED,
        __FUNCTION__);
    return func(stream);
}

rtError_t rtGetStreamId(rtStream_t stm, int32_t *streamId)
{
    using rtGetStreamIdFunc = std::function<rtError_t(rtStream_t, int32_t*)>;
    static rtGetStreamIdFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t, int32_t*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(stm, streamId);
}


rtError_t rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stm)
{
    using rtProfilerTraceExFunc = std::function<rtError_t(uint64_t, uint64_t, uint16_t, rtStream_t)>;
    static rtProfilerTraceExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, uint64_t, uint64_t, uint16_t, rtStream_t>("libruntime",
            __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(id, modelId, tagId, stm);
}

rtError_t rtKernelLaunch(const void* stubFunc, uint32_t blockDim, void* args,
    uint32_t argsSize, rtSmDesc_t* smDesc, rtStream_t stream)
{
    using rtKernelLaunchFunc = std::function<rtError_t(const void*, uint32_t, void*, uint32_t,
        rtSmDesc_t*, rtStream_t)>;
    static rtKernelLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void*, uint32_t, void*, uint32_t,
            rtSmDesc_t*, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, args, argsSize, smDesc, stream);
}

rtError_t rtKernelLaunchWithFlagV2(const void* stubFunc, uint32_t blockDim,
    RtArgsExT* argsInfo, rtSmDesc_t* smDesc, rtStream_t stream, uint32_t flags, const RtTaskCfgInfoT* cfgInfo)
{
    using rtKernelLaunchWithFlagV2Func = std::function<rtError_t(const void*, uint32_t, RtArgsExT*,
        rtSmDesc_t*, rtStream_t, uint32_t, const RtTaskCfgInfoT*)>;
    static rtKernelLaunchWithFlagV2Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void*, uint32_t, RtArgsExT*, rtSmDesc_t*, rtStream_t,
            uint32_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, argsInfo, smDesc, stream, flags, cfgInfo);
}

rtError_t rtKernelLaunchWithHandleV2(
    void *handle, const uint64_t tilingkey, uint32_t blockDim, RtArgsExT *argsInfo, rtSmDesc_t *smDesc,
    rtStream_t stream, const RtTaskCfgInfoT *cfgInfo)
{
    using rtKernelLaunchWithHandleV2Func = std::function<rtError_t(void*, uint64_t, uint32_t, RtArgsExT*,
        rtSmDesc_t*, rtStream_t, const RtTaskCfgInfoT*)>;
    static rtKernelLaunchWithHandleV2Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void*, uint64_t, uint32_t, RtArgsExT*, rtSmDesc_t*,
            rtStream_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(handle, tilingkey, blockDim, argsInfo, smDesc, stream, cfgInfo);
}

rtError_t rtAicpuKernelLaunchExWithArgs(const uint32_t kernelType, const char* const opName,
    const uint32_t blockDim, const RtAicpuArgsExT *argsInfo, rtSmDesc_t * const smDesc,
    const rtStream_t stm, const uint32_t flags)
{
    using rtAicpuKernelLaunchExWithArgsFunc = std::function<rtError_t(uint32_t, const char*, uint32_t,
        const RtAicpuArgsExT*, rtSmDesc_t*, const rtStream_t, uint32_t)>;
    static rtAicpuKernelLaunchExWithArgsFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, uint32_t, const char*, uint32_t, const RtAicpuArgsExT*,
            rtSmDesc_t*, rtStream_t, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    return func(kernelType, opName, blockDim, argsInfo, smDesc, stm, flags);
}

rtError_t rtLaunchKernelByFuncHandle(rtFuncHandle funcHandle, uint32_t blockDim, rtLaunchArgsHandle argsHandle,
    rtStream_t stm)
{
    using rtLaunchKernelByFuncHandleFunc = std::function<rtError_t(rtFuncHandle, uint32_t, rtLaunchArgsHandle,
        rtStream_t)>;
    static rtLaunchKernelByFuncHandleFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtFuncHandle, uint32_t, rtLaunchArgsHandle, rtStream_t>("libruntime",
            __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsHandle, stm);
}

rtError_t rtLaunchKernelByFuncHandleV2(rtFuncHandle funcHandle, uint32_t blockDim, rtLaunchArgsHandle argsHandle,
    rtStream_t stm, const RtTaskCfgInfoT *cfgInfo)
{
    using rtLaunchKernelByFuncHandleV2Func = std::function<rtError_t(rtFuncHandle, uint32_t, rtLaunchArgsHandle,
        rtStream_t, const RtTaskCfgInfoT*)>;
    static rtLaunchKernelByFuncHandleV2Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtFuncHandle, uint32_t, rtLaunchArgsHandle, rtStream_t,
            const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    return func(funcHandle, blockDim, argsHandle, stm, cfgInfo);
}

rtError_t rtVectorCoreKernelLaunch(const VOID_PTR stubFunc, uint32_t blockDim, RtArgsExT *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const RtTaskCfgInfoT *cfgInfo)
{
    using rtVectorCoreKernelLaunchFunc = std::function<rtError_t(VOID_PTR, uint32_t, RtArgsExT*,
        rtSmDesc_t*, rtStream_t, uint32_t, const RtTaskCfgInfoT*)>;
    static rtVectorCoreKernelLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, VOID_PTR, uint32_t, RtArgsExT*, rtSmDesc_t*, rtStream_t,
            uint32_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AIV_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    return func(stubFunc, blockDim, argsInfo, smDesc, stm, flags, cfgInfo);
}

rtError_t rtVectorCoreKernelLaunchWithHandle(VOID_PTR hdl, const uint64_t tilingKey, uint32_t blockDim,
    RtArgsExT *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const RtTaskCfgInfoT *cfgInfo)
{
    using rtVectorCoreKernelLaunchWithHandleFunc = std::function<rtError_t(VOID_PTR, uint64_t, uint32_t,
        RtArgsExT*, rtSmDesc_t*, rtStream_t, const RtTaskCfgInfoT*)>;
    static rtVectorCoreKernelLaunchWithHandleFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, VOID_PTR, uint64_t, uint32_t, RtArgsExT*, rtSmDesc_t*,
            rtStream_t, const RtTaskCfgInfoT*>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AIV_LAUNCH, __FUNCTION__);
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    return func(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, cfgInfo);
}

rtError_t rtBinaryGetFunction(VOID_PTR binHandle, uint64_t tilingKey, VOID_PTR_PTR funcHandle)
{
    using rtBinaryGetFunctionFunc = std::function<rtError_t(VOID_PTR, uint64_t, VOID_PTR_PTR)>;
    static rtBinaryGetFunctionFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, VOID_PTR, uint64_t, VOID_PTR_PTR>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(binHandle, tilingKey, funcHandle);
}

rtError_t rtProfSetProSwitch(VOID_PTR data, uint32_t len)
{
    using rtProfSetProSwitchFunc = std::function<rtError_t(VOID_PTR, uint32_t)>;
    static rtProfSetProSwitchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, VOID_PTR, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(data, len);
}

rtError_t rtGetVisibleDeviceIdByLogicDeviceId(const int32_t logicDeviceId, int32_t * const visibleDeviceId)
{
    using rtGetVisibleDeviceIdByLogicDeviceIdFunc = std::function<rtError_t(int32_t, int32_t *)>;
    static rtGetVisibleDeviceIdByLogicDeviceIdFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t, int32_t *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    return func(logicDeviceId, visibleDeviceId);
}

rtError_t rtLaunch(const void *stubFunc)
{
    using rtLaunchFunc = std::function<rtError_t(const void *)>;
    static rtLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc);
}

rtError_t rtKernelLaunchWithFlag(const void *stubFunc, uint32_t blockDim, RtArgsExT *argsInfo, rtSmDesc_t *smDesc,
                                 rtStream_t stm, uint32_t flags)
{
    using rtKernelLaunchWithFlagFunc = std::function<rtError_t(const void *, uint32_t, RtArgsExT *, rtSmDesc_t *,
                                                               rtStream_t, uint32_t)>;
    static rtKernelLaunchWithFlagFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void *, uint32_t, RtArgsExT *, rtSmDesc_t *, rtStream_t,
            uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(stubFunc, blockDim, argsInfo, smDesc, stm, flags);
}

rtError_t rtLaunchKernelByFuncHandleV3(rtFuncHandle funcHandle, uint32_t blockDim, const RtArgsExT * const argsInfo,
                                       rtStream_t stm, const RtTaskCfgInfoT * const cfgInfo)
{
    using rtLaunchKernelByFuncHandleV3Func = std::function<rtError_t(rtFuncHandle, uint32_t, const RtArgsExT *,
                                                                     rtStream_t, const RtTaskCfgInfoT *)>;
    static rtLaunchKernelByFuncHandleV3Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtFuncHandle, uint32_t, const RtArgsExT *, rtStream_t,
            const RtTaskCfgInfoT *>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsInfo, stm, cfgInfo);
}

rtError_t rtCpuKernelLaunch(const void *soName, const void *kernelName, uint32_t blockDim, const void *args,
                            uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)
{
    using rtCpuKernelLaunchFunc = std::function<rtError_t(const void *, const void *, uint32_t, const void *,
                                                          uint32_t, rtSmDesc_t *, rtStream_t)>;
    static rtCpuKernelLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void *, const void *, uint32_t, const void *, uint32_t,
            rtSmDesc_t *, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CPU_LAUNCH, __FUNCTION__);
    return func(soName, kernelName, blockDim, args, argsSize, smDesc, stm);
}

rtError_t rtCpuKernelLaunchWithFlag(const void *soName, const void *kernelName, uint32_t blockDim,
                                    const RtArgsExT *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    using rtCpuKernelLaunchWithFlagFunc =
            std::function<rtError_t(const void *, const void *, uint32_t, const RtArgsExT *, rtSmDesc_t *,
                                    rtStream_t, uint32_t)>;
    static rtCpuKernelLaunchWithFlagFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void *, const void *, uint32_t, const RtArgsExT *, rtSmDesc_t *,
            rtStream_t, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CPU_LAUNCH, __FUNCTION__);
    return func(soName, kernelName, blockDim, argsInfo, smDesc, stm, flags);
}

rtError_t rtAicpuKernelLaunchWithFlag(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
                                      const RtArgsExT *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    using rtAicpuKernelLaunchWithFlagFunc =
            std::function<rtError_t(const rtKernelLaunchNames_t *, uint32_t, const RtArgsExT *, rtSmDesc_t *,
                                    rtStream_t, uint32_t)>;
    static rtAicpuKernelLaunchWithFlagFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const rtKernelLaunchNames_t *, uint32_t, const RtArgsExT *, rtSmDesc_t *,
            rtStream_t, uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    return func(launchNames, blockDim, argsInfo, smDesc, stm, flags);
}

rtError_t rtAicpuKernelLaunch(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim, const void *args,
                              uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)
{
    using rtAicpuKernelLaunchFunc = std::function<rtError_t(const rtKernelLaunchNames_t *, uint32_t, const void *,
                                                            uint32_t, rtSmDesc_t *, rtStream_t)>;
    static rtAicpuKernelLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const rtKernelLaunchNames_t *, uint32_t, const void *, uint32_t,
            rtSmDesc_t *, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, __FUNCTION__);
    return func(launchNames, blockDim, args, argsSize, smDesc, stm);
}

rtError_t rtFftsPlusTaskLaunch(RtFftsPlusTaskInfoT *fftsPlusTaskInfo, rtStream_t stm)
{
    using rtFftsPlusTaskLaunchFunc = std::function<rtError_t(RtFftsPlusTaskInfoT *, rtStream_t)>;
    static rtFftsPlusTaskLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, RtFftsPlusTaskInfoT *, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsPlusTaskInfo, stm);
}

rtError_t rtFftsTaskLaunch(rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stm)
{
    using rtFftsTaskLaunchFunc = std::function<rtError_t(rtFftsTaskInfo_t *, rtStream_t)>;
    static rtFftsTaskLaunchFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtFftsTaskInfo_t *, rtStream_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsTaskInfo, stm);
}

rtError_t rtFftsTaskLaunchWithFlag(rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stm, uint32_t flag)
{
    using rtFftsTaskLaunchWithFlagFunc = std::function<rtError_t(rtFftsTaskInfo_t *, rtStream_t, uint32_t)>;
    static rtFftsTaskLaunchWithFlagFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtFftsTaskInfo_t *, rtStream_t,
            uint32_t>("libruntime", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libruntime.so");
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_FFTS_LAUNCH, __FUNCTION__);
    return func(fftsTaskInfo, stm, flag);
}
