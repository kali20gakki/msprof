/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cce/dnn.h>
#include <securec.h>
#include "external/runtime/rt_error_codes.h"

#define EVENT_LENTH 10

rtError_t rtCtxSetCurrent(rtContext_t ctx)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtGetStreamId(rtStream_t stream, int32_t *streamId)
{
    *streamId = 0;
    return ACL_RT_SUCCESS;
}

rtError_t rtCtxGetCurrent(rtContext_t *ctx)
{
    int x = 1;
    *ctx = (void *)x;
    return ACL_RT_SUCCESS;
}

rtError_t rtCtxSetDryRun (rtContext_t ctx, rtDryRunFlag_t enable, uint32_t flag)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtEventGetTimeStamp(uint64_t *time, rtEvent_t event)
{
    *time = 12345;
    return ACL_RT_SUCCESS;
}

rtError_t rtEventCreate(rtEvent_t *event)
{
    *event = new int[EVENT_LENTH];
    return ACL_RT_SUCCESS;
}
rtError_t rtEventRecord(rtEvent_t event, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtEventSynchronize(rtEvent_t event)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtEventDestroy(rtEvent_t event)
{
    delete [] (int*)event;
    return ACL_RT_SUCCESS;
}

rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = new uint8_t[size];
    return ACL_RT_SUCCESS;
}

rtError_t rtMemset(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtFree(void *devPtr)
{
    delete[] (uint8_t *)devPtr;
    return ACL_RT_SUCCESS;
}

rtError_t rtMallocHost(void **hostPtr,  uint64_t  size)
{
    *hostPtr = new uint8_t[size];
    return ACL_RT_SUCCESS;
}

rtError_t rtFreeHost(void *hostPtr)
{
    delete[] (uint8_t*)hostPtr;
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority)
{
    *stream = new uint32_t;
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamDestroy(rtStream_t stream)
{
    delete (uint32_t*)stream;
    return ACL_RT_SUCCESS;
}

rtError_t rtSetDevice(int32_t device)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamSynchronize(rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind)
{
    #ifdef OTQT_UT
    if (destMax == 12 && count == 12) { // UTEST_kernelinfo_manager.all_success特殊处理
        memcpy_s(dst, destMax, src, count);
    }
    #endif
    return ACL_RT_SUCCESS;
}
rtError_t rtMemcpyAsync(void *dst, uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamWaitEvent(rtStream_t stream, rtEvent_t event)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtGetDeviceCount(int32_t* count)
{
    *count = 1;
    return ACL_RT_SUCCESS;
}

rtError_t rtDeviceReset(int32_t device)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end)
{
    *time = 10.0f;
    return ACL_RT_SUCCESS;
}
rtError_t rtFunctionRegister(void *binHandle,
    const void *stubFunc,
    const char *stubName,
    const void *devFunc)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtFunctionRegister(void *binHandle,
     const void *stubFunc,
     const char *stubName,
     const void *devFunc,
     uint32_t funcMode)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtKernelConfigTransArg(const void *ptr,
    uint64_t size,
    uint32_t flag,
    void** arg)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtKernelLaunch(const void *stubFunc,
    uint32_t blockDim,
    void *args,
    uint32_t argsSize,
    rtSmDesc_t *smDesc,
    rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtKernelLaunchWithHandle(void *handle,
    const void *devFunc,
    uint32_t blockDim,
    void *args,
    uint32_t argsSize,
    rtSmDesc_t *smDesc,
    rtStream_t stream,
    const void *ptr)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtSetupArgument(const void *arg, uint32_t size, uint32_t offset)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtLaunch(const void *stubFunc)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtDevBinaryUnRegister(void *handle)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtConfigureCall(uint32_t numBlocks, rtSmDesc_t *smDesc, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtSetProfDir(char *profdir)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtSetProfDirEx(char *profdir, char *address, char *job_ctx)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtAiCoreMemorySizes(rtAiCoreMemorySize_t *aiCoreMemorySize)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtSetKernelReportCallback(rtKernelReportCallback callBack)
{
    rtKernelInfo rt_kernel_info = { 0 };
    rt_kernel_info.arg_size = 12;
    rt_kernel_info.task_offset = 100;
    rt_kernel_info.arg = (void *)100;
    rt_kernel_info.module_addr = (void *)100;
    rt_kernel_info.module_size = 100;

    rtStream_t stream;
    callBack(stream, &rt_kernel_info);
    return ACL_RT_SUCCESS;
}

rtError_t rtMemAdvise(void *ptr, uint64_t size, uint32_t advise)
{
    return ACL_RT_SUCCESS;
}

/**
 * @ingroup rt_kernel
 * @brief start fusion kernels.
 * @param [in] stream   stream for fusion kernels
 * @return ACL_RT_SUCCESS for ok, errno for failed
 */
rtError_t rtKernelFusionStart(rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

/**
 * @ingroup rt_kernel
 * @brief end fusion kernels.
 * @param [in] stream   stream for fusion kernels
 * @return ACL_RT_SUCCESS for ok, errno for failed
 */
rtError_t rtKernelFusionEnd(rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtMemGetInfo(size_t* free, size_t* total)
{
    *free = 512UL * 1024UL * 1024UL;
    *total = 1024UL * 1024UL * 1024UL;
    return ACL_RT_SUCCESS;
}

rtError_t rtMemAllocManaged(void **ptr, uint64_t size, uint32_t flag)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtMemFreeManaged(void *ptr)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtMetadataRegister(void *handle, const char *metadata)
{
	return ACL_RT_SUCCESS;
}
rtError_t rtSetTaskGenCallback(rtTaskGenCallback callback)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtModelCreate(rtModel_t *model, uint32_t flag)
{
    *model = new uint32_t;
    return ACL_RT_SUCCESS;
}

rtError_t rtModelDestroy(rtModel_t model)
{
    delete model;
    return ACL_RT_SUCCESS;
}

rtError_t rtModelBindStream(rtModel_t model, rtStream_t stream, uint32_t flag)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtModelUnbindStream(rtModel_t model, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}
rtError_t rtModelExecute(rtModel_t model, rtStream_t stream, uint32_t flag)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtGetFunctionByName(const char *stubName, void **stubFunc)
{
    *(char**)stubFunc =  "func";
    return ACL_RT_SUCCESS;
}

rtError_t rtQueryFunctionRegistered(const char *stubName)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtCtxCreate(rtContext_t *ctx, uint32_t flags, int32_t device)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtKernelLaunchEx(void *args, uint32_t argsSize, uint32_t flags, rtStream_t stream_)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtModelGetTaskId(void* handle, uint32_t *taskid)
{
    *taskid = 0;
    return ACL_RT_SUCCESS;
}
rtError_t rtProfilerStop(void)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtSetDvfsProfile(DvfsProfileMode mode)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtUnsetDvfsProfile()
{
    return ACL_RT_SUCCESS;
}

rtError_t rtGetDvfsProfile(DvfsProfileMode *pmode)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtCtxDestroy(rtContext_t ctx)
{
	return ACL_RT_SUCCESS;
}

rtError_t rtProfilerInit(const char *profdir, const char *address, const char *job_ctx)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtProfilerStart(void)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtLabelCreate(rtLabel_t *label)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtLabelDestroy(rtLabel_t label)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtLabelSet(rtLabel_t label, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtLabelSwitch(void *ptr, rtCondition_t condition, uint32_t value, rtLabel_t trueLabel, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtLabelGoto(rtLabel_t label, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtInvalidCache(uint64_t base, uint32_t len)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtModelLoadComplete(rtModel_t model)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamCreateWithFlags(rtStream_t *stream, int32_t priority, uint32_t flags)
{
    *stream = new uint32_t;
    return ACL_RT_SUCCESS;
}

rtError_t rtFlushCache(uint64_t base, uint32_t len)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtProfilerTrace(uint64_t id, bool notify, uint32_t flags, rtStream_t stream_)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtMemSetRC(const void *devPtr, uint64_t size, uint32_t readCount)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamSwitch(void *ptr,  rtCondition_t condition, int64_t value, rtStream_t true_stream, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamSwitchEx(void *ptr,  rtCondition_t condition, void *value_ptr, rtStream_t true_stream, rtStream_t stream, rtSwitchDataType_t dataType)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtStreamActive(rtStream_t active_stream, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtEventReset(rtEvent_t event, rtStream_t stream)
{
    return ACL_RT_SUCCESS;
}

rtError_t rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
  *value = 0;
  return ACL_RT_SUCCESS;
}