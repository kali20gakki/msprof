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

#include "activity/ascend/ascend_manager.h"
#include "callback/callback_manager.h"
#include "common/function_loader.h"

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
    }
    ~RuntimeInject() = default;
};

RuntimeInject g_rtInject;

rtError_t rtSetDevice(int32_t device)
{
    using rtSetDeviceFunc = std::function<rtError_t(int32_t)>;
    static rtSetDeviceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", "rtSetDevice", func);
    }
    Mspti::Ascend::AscendProfEnable(true, device);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_SET, __FUNCTION__);
    return func(device);
}

rtError_t rtDeviceReset(int32_t device)
{
    using rtDeviceResetFunc = std::function<rtError_t(int32_t)>;
    static rtDeviceResetFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", "rtDeviceReset", func);
    }
    Mspti::Ascend::AscendProfEnable(false, device);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_RESET, __FUNCTION__);
    return func(device);
}

rtError_t rtSetDeviceEx(int32_t device)
{
    using rtSetDeviceExFunc = std::function<rtError_t(int32_t)>;
    static rtSetDeviceExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t>("libruntime", "rtSetDeviceEx", func);
    }
    Mspti::Ascend::AscendProfEnable(true, device);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_SET_EX, __FUNCTION__);
    return func(device);
}

rtError_t RtGetDevice(int32_t* devId)
{
    using rtGetDeviceFunc = std::function<rtError_t(int32_t*)>;
    static rtGetDeviceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, int32_t*>("libruntime", "rtGetDevice", func);
    }
    return func(devId);
}

rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device)
{
    using rtCtxCreateExFunc = std::function<rtError_t(void **, uint32_t, int32_t)>;
    static rtCtxCreateExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **, uint32_t, int32_t>("libruntime", "rtCtxCreateEx", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, __FUNCTION__);
    return func(ctx, flags, device);
}

rtError_t rtCtxCreate(void **ctx, int32_t device)
{
    using rtCtxCreateFunc = std::function<rtError_t(void **, int32_t)>;
    static rtCtxCreateFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **, int32_t>("libruntime", "rtCtxCreate", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED, __FUNCTION__);
    return func(ctx, device);
}

rtError_t rtCtxDestroy(void **ctx)
{
    using rtCtxDestroyFunc = std::function<rtError_t(void **)>;
    static rtCtxDestroyFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void **>("libruntime", "rtCtxDestroy", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY, __FUNCTION__);
    return func(ctx);
}

rtError_t rtStreamCreate(rtStream_t* stream, int32_t priority)
{
    using rtStreamCreateFunc = std::function<rtError_t(rtStream_t*, int32_t)>;
    static rtStreamCreateFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t*, int32_t>("libruntime", "rtStreamCreate", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_CREATED, __FUNCTION__);
    return func(stream, priority);
}

rtError_t rtStreamDestroy(rtStream_t stream)
{
    using rtStreamDestroyFunc = std::function<rtError_t(rtStream_t)>;
    static rtStreamDestroyFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t>("libruntime", "rtStreamDestroy", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_DESTROY, __FUNCTION__);
    return func(stream);
}

rtError_t rtStreamSynchronize(rtStream_t stream)
{
    using rtStreamSynchronizeFunc = std::function<rtError_t(rtStream_t)>;
    static rtStreamSynchronizeFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t>("libruntime", "rtStreamSynchronize", func);
    }
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED,
        __FUNCTION__);
    return func(stream);
}

rtError_t RtGetStreamId(rtStream_t stm, int32_t *streamId)
{
    using rtGetStreamIdFunc = std::function<rtError_t(rtStream_t, int32_t*)>;
    static rtGetStreamIdFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, rtStream_t, int32_t*>("libruntime", "rtGetStreamId", func);
    }
    return func(stm, streamId);
}


rtError_t RtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stm)
{
    using rtProfilerTraceExFunc = std::function<rtError_t(uint64_t, uint64_t, uint16_t, rtStream_t)>;
    static rtProfilerTraceExFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, uint64_t, uint64_t, uint16_t, rtStream_t>("libruntime",
            "rtProfilerTraceEx", func);
    }
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
            rtSmDesc_t*, rtStream_t>("libruntime", "rtKernelLaunch", func);
    }
    return func(stubFunc, blockDim, args, argsSize, smDesc, stream);
}

rtError_t rtKernelLaunchWithFlagV2(const void* stubFunc, uint32_t blockDim,
    rtArgsEx_t* argsInfo, rtSmDesc_t* smDesc, rtStream_t stream, uint32_t flags, const RtTaskCfgInfoT* cfgInfo)
{
    using rtKernelLaunchWithFlagV2Func = std::function<rtError_t(const void*, uint32_t, rtArgsEx_t*,
        rtSmDesc_t*, rtStream_t, uint32_t, const RtTaskCfgInfoT*)>;
    static rtKernelLaunchWithFlagV2Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, const void*, uint32_t, rtArgsEx_t*, rtSmDesc_t*, rtStream_t,
            uint32_t, const RtTaskCfgInfoT*>("libruntime", "rtKernelLaunchWithFlagV2", func);
    }
    return func(stubFunc, blockDim, argsInfo, smDesc, stream, flags, cfgInfo);
}

rtError_t rtKernelLaunchWithHandleV2(
    void *handle, const uint64_t tilingkey, uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc,
    rtStream_t stream, const RtTaskCfgInfoT *cfgInfo)
{
    using rtKernelLaunchWithHandleV2Func = std::function<rtError_t(void*, uint64_t, uint32_t, rtArgsEx_t*,
        rtSmDesc_t*, rtStream_t, const RtTaskCfgInfoT*)>;
    static rtKernelLaunchWithHandleV2Func func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<rtError_t, void*, uint64_t, uint32_t, rtArgsEx_t*, rtSmDesc_t*,
            rtStream_t, const RtTaskCfgInfoT*>("libruntime", "rtKernelLaunchWithHandleV2", func);
    }
    return func(handle, tilingkey, blockDim, argsInfo, smDesc, stream, cfgInfo);
}
