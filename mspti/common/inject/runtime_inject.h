/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_inject.h
 * Description        : Injection of runtime.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_RUNTIME_INJECT_H
#define MSPTI_COMMON_INJECT_RUNTIME_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Inject
MSPTI_API rtError_t rtSetDevice(int32_t device);
MSPTI_API rtError_t rtDeviceReset(int32_t device);
MSPTI_API rtError_t rtSetDeviceEx(int32_t device);
MSPTI_API rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device);
MSPTI_API rtError_t rtCtxCreate(void **ctx, int32_t device);
MSPTI_API rtError_t rtCtxDestroy(void **ctx);
MSPTI_API rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority);
MSPTI_API rtError_t rtStreamDestroy(rtStream_t stream);
MSPTI_API rtError_t rtStreamSynchronize(rtStream_t stream);
MSPTI_API rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim, void *args,
    uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream);
MSPTI_API rtError_t rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim,
    RtArgsExT *argsInfo, rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flags, const RtTaskCfgInfoT *cfgInfo);
MSPTI_API rtError_t rtKernelLaunchWithHandleV2(
    void *handle, const uint64_t tilingkey, uint32_t blockDim, RtArgsExT *argsInfo, rtSmDesc_t *smDesc,
    rtStream_t stream, const RtTaskCfgInfoT *cfgInfo);
MSPTI_API rtError_t rtAicpuKernelLaunchExWithArgs(const uint32_t kernelType, const char* const opName,
    const uint32_t blockDim, const RtAicpuArgsExT *argsInfo, rtSmDesc_t * const smDesc,
    const rtStream_t stm, const uint32_t flags);
MSPTI_API rtError_t rtLaunchKernelByFuncHandle(rtFuncHandle funcHandle, uint32_t blockDim,
    rtLaunchArgsHandle argsHandle, rtStream_t stm);
MSPTI_API rtError_t rtLaunchKernelByFuncHandleV2(rtFuncHandle funcHandle, uint32_t blockDim,
    rtLaunchArgsHandle argsHandle, rtStream_t stm, const RtTaskCfgInfoT *cfgInfo);
MSPTI_API rtError_t rtVectorCoreKernelLaunch(const VOID_PTR stubFunc, uint32_t blockDim, RtArgsExT *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const RtTaskCfgInfoT *cfgInfo);
MSPTI_API rtError_t rtVectorCoreKernelLaunchWithHandle(VOID_PTR hdl, const uint64_t tilingKey, uint32_t blockDim,
    RtArgsExT *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const RtTaskCfgInfoT *cfgInfo);

// Inner
rtError_t rtGetDevice(int32_t *devId);
rtError_t rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stm);
rtError_t rtGetStreamId(rtStream_t stm, int32_t *streamId);
rtError_t rtBinaryGetFunction(VOID_PTR binHandle, uint64_t tilingKey, VOID_PTR_PTR funcHandle);
rtError_t rtProfSetProSwitch(VOID_PTR data, uint32_t len);

#if defined(__cplusplus)
}
#endif

#endif
