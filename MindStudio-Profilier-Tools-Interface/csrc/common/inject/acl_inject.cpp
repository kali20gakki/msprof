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
#include "common/inject/acl_inject.h"

#include <functional>

#include "activity/activity_manager.h"
#include "callback/callback_manager.h"

namespace {
const std::string SO_NAME = "libascendcl";
const std::string SO_FILE_NAME = SO_NAME + ".so";

enum AclRtFuncIndex {
    FUNC_ACL_RT_SET_DEVICE,
    FUNC_ACL_RT_RESET_DEVICE,
    FUNC_ACL_RT_CREATE_CTX,
    FUNC_ACL_RT_DESTROY_CTX,
    FUNC_ACL_RT_CREATE_STREAM,
    FUNC_ACL_RT_DESTROY_STREAM,
    FUNC_ACL_RT_SYNCHRONIZE_STREAM,
    FUNC_ACL_RT_LAUNCH_KERNEL,
    FUNC_ACL_RT_LAUNCH_KERNEL_V2,
    FUNC_ACL_RT_LAUNCH_KERNEL_WITH_CFG,
    FUNC_ACL_RT_LAUNCH_KERNEL_WITH_HOST_ARGS,

    FUNC_ACL_RT_GET_DEVICE,
    FUNC_ACL_RT_STREAM_GET_ID,
    FUNC_ACL_RT_PROF_TRACE,
    FUNC_ACL_RT_BINARY_GET_FUNC_BY_ENTRY,
    FUNC_ACL_RT_GET_LOGIC_DEV_ID_BY_USER_DEV_ID,
    FUNC_ACL_RT_COUNT
};

pthread_once_t g_once;
void* g_aclrtFuncArray[FUNC_ACL_RT_COUNT];

void LoadAclFunction()
{
    g_aclrtFuncArray[FUNC_ACL_RT_SET_DEVICE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtSetDevice");
    g_aclrtFuncArray[FUNC_ACL_RT_RESET_DEVICE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtResetDevice");
    g_aclrtFuncArray[FUNC_ACL_RT_CREATE_CTX] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtCreateContext");
    g_aclrtFuncArray[FUNC_ACL_RT_DESTROY_CTX] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtDestroyContext");
    g_aclrtFuncArray[FUNC_ACL_RT_CREATE_STREAM] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtCreateStream");
    g_aclrtFuncArray[FUNC_ACL_RT_DESTROY_STREAM] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtDestroyStream");
    g_aclrtFuncArray[FUNC_ACL_RT_SYNCHRONIZE_STREAM] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtSynchronizeStream");
    g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtLaunchKernel");
    g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_V2] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtLaunchKernelV2");
    g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_WITH_CFG] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtLaunchKernelWithConfig");
    g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_WITH_HOST_ARGS] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtLaunchKernelWithHostArgs");

    g_aclrtFuncArray[FUNC_ACL_RT_GET_DEVICE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtGetDevice");
    g_aclrtFuncArray[FUNC_ACL_RT_STREAM_GET_ID] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtStreamGetId");
    g_aclrtFuncArray[FUNC_ACL_RT_PROF_TRACE] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtProfTrace");
    g_aclrtFuncArray[FUNC_ACL_RT_BINARY_GET_FUNC_BY_ENTRY] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtBinaryGetFunctionByEntry");
    g_aclrtFuncArray[FUNC_ACL_RT_GET_LOGIC_DEV_ID_BY_USER_DEV_ID] =
        Mspti::Common::RegisterFunction(SO_NAME, "aclrtGetLogicDevIdByUserDevId");
}
}

AclError aclrtSetDevice(int32_t deviceId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_SET_DEVICE];
    using aclrtSetDeviceFunc = std::function<decltype(aclrtSetDevice)>;
    aclrtSetDeviceFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtSetDevice)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_SET, __FUNCTION__);
    auto ret = func(deviceId);
    if (ret == MSPTI_SUCCESS) {
        int32_t logicDeviceId = 0;
        uint32_t realDeviceId = aclrtGetLogicDevIdByUserDevId(deviceId, &logicDeviceId) == MSPTI_SUCCESS ?
            static_cast<uint32_t>(logicDeviceId) : static_cast<uint32_t>(deviceId);
        Mspti::Activity::ActivityManager::GetInstance()->SetDevice(realDeviceId);
    }
    return ret;
}

AclError aclrtResetDevice(int32_t deviceId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_RESET_DEVICE];
    using aclrtResetDeviceFunc = std::function<decltype(aclrtResetDevice)>;
    aclrtResetDeviceFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtResetDevice)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_DEVICE_RESET, __FUNCTION__);
    return func(deviceId);
}

AclError aclrtCreateContext(void **context, int32_t deviceId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_CREATE_CTX];
    using aclrtCreateContextFunc = std::function<decltype(aclrtCreateContext)>;
    aclrtCreateContextFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtCreateContext)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_CREATED, __FUNCTION__);
    return func(context, deviceId);
}

AclError aclrtDestroyContext(void *context)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_DESTROY_CTX];
    using aclrtDestroyContextFunc = std::function<decltype(aclrtDestroyContext)>;
    aclrtDestroyContextFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtDestroyContext)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY, __FUNCTION__);
    return func(context);
}

AclError aclrtCreateStream(AclrtStream *stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_CREATE_STREAM];
    using aclrtCreateStreamFunc = std::function<decltype(aclrtCreateStream)>;
    aclrtCreateStreamFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtCreateStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_CREATED, __FUNCTION__);
    return func(stream);
}

AclError aclrtDestroyStream(AclrtStream stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_DESTROY_STREAM];
    using aclrtDestroyStreamFunc = std::function<decltype(aclrtDestroyStream)>;
    aclrtDestroyStreamFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtDestroyStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_DESTROY, __FUNCTION__);
    return func(stream);
}

AclError aclrtSynchronizeStream(AclrtStream stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_SYNCHRONIZE_STREAM];
    using aclrtSynchronizeStreamFunc = std::function<decltype(aclrtSynchronizeStream)>;
    aclrtSynchronizeStreamFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtSynchronizeStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED,
                                         __FUNCTION__);
    return func(stream);
}

AclError aclrtLaunchKernel(AclrtFuncHandle funcHandle, uint32_t blockDim,
                           const void *argsData, size_t argsSize, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL];
    using aclrtLaunchKernelFunc = std::function<decltype(aclrtLaunchKernel)>;
    aclrtLaunchKernelFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtLaunchKernel)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsData, argsSize, stream);
}

AclError aclrtLaunchKernelV2(AclrtFuncHandle funcHandle, uint32_t blockDim, const void *argsData,
                             size_t argsSize, AclrtLaunchKernelCfg *cfg, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_V2];
    using aclrtLaunchKernelV2Func = std::function<decltype(aclrtLaunchKernelV2)>;
    aclrtLaunchKernelV2Func func = Mspti::Common::ReinterpretConvert<decltype(&aclrtLaunchKernelV2)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, argsData, argsSize, cfg, stream);
}

AclError aclrtLaunchKernelWithConfig(AclrtFuncHandle funcHandle, uint32_t blockDim, AclrtStream stream,
                                     AclrtLaunchKernelCfg *cfg, AclrtArgsHandle argsHandle, void *reserve)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_WITH_CFG];
    using aclrtLaunchKernelWithConfigFunc = std::function<decltype(aclrtLaunchKernelWithConfig)>;
    aclrtLaunchKernelWithConfigFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtLaunchKernelWithConfig)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, stream, cfg, argsHandle, reserve);
}

AclError aclrtLaunchKernelWithHostArgs(AclrtFuncHandle funcHandle, uint32_t blockDim, AclrtStream stream,
                                       AclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
                                       AclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_LAUNCH_KERNEL_WITH_HOST_ARGS];
    using aclrtLaunchKernelWithHostArgsFunc = std::function<decltype(aclrtLaunchKernelWithHostArgs)>;
    aclrtLaunchKernelWithHostArgsFunc func =
        Mspti::Common::ReinterpretConvert<decltype(&aclrtLaunchKernelWithHostArgs)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_RUNTIME, MSPTI_CBID_RUNTIME_LAUNCH, __FUNCTION__);
    return func(funcHandle, blockDim, stream, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
}

AclError aclrtGetDevice(int32_t *deviceId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_GET_DEVICE];
    using aclrtGetDeviceFunc = std::function<decltype(aclrtGetDevice)>;
    aclrtGetDeviceFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtGetDevice)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    return func(deviceId);
}

AclError aclrtStreamGetId(AclrtStream stream, int32_t *streamId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_STREAM_GET_ID];
    using aclrtStreamGetIdFunc = std::function<decltype(aclrtStreamGetId)>;
    aclrtStreamGetIdFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtStreamGetId)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    return func(stream, streamId);
}

AclError aclrtProfTrace(void *userdata, int32_t length, AclrtStream stream)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_PROF_TRACE];
    using aclrtProfTraceFunc = std::function<decltype(aclrtProfTrace)>;
    aclrtProfTraceFunc func = Mspti::Common::ReinterpretConvert<decltype(&aclrtProfTrace)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    return func(userdata, length, stream);
}

AclError aclrtBinaryGetFunctionByEntry(AclrtBinHandle binHandle, uint64_t funcEntry, AclrtFuncHandle *funcHandle)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_BINARY_GET_FUNC_BY_ENTRY];
    using aclrtBinaryGetFunctionByEntryFunc = std::function<decltype(aclrtBinaryGetFunctionByEntry)>;
    aclrtBinaryGetFunctionByEntryFunc func =
            Mspti::Common::ReinterpretConvert<decltype(&aclrtBinaryGetFunctionByEntry)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    return func(binHandle, funcEntry, funcHandle);
}

AclError aclrtGetLogicDevIdByUserDevId(const int32_t userDevid, int32_t *const logicDevId)
{
    pthread_once(&g_once, LoadAclFunction);
    void* voidFunc = g_aclrtFuncArray[FUNC_ACL_RT_GET_LOGIC_DEV_ID_BY_USER_DEV_ID];
    using aclrtGetLogicDevIdByUserDevIdFunc = std::function<decltype(aclrtGetLogicDevIdByUserDevId)>;
    aclrtGetLogicDevIdByUserDevIdFunc func =
            Mspti::Common::ReinterpretConvert<decltype(&aclrtGetLogicDevIdByUserDevId)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction(SO_NAME, __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, SO_FILE_NAME);
    return func(userDevid, logicDevId);
}

