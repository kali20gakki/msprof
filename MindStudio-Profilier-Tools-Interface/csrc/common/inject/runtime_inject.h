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

#include "csrc/common/inject/inject_base.h"
#include "csrc/include/mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Inject
MSPTI_API RtErrorT rtSetDevice(int32_t device);
MSPTI_API RtErrorT rtDeviceReset(int32_t device);
MSPTI_API RtErrorT rtSetDeviceEx(int32_t device);
MSPTI_API RtErrorT rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device);
MSPTI_API RtErrorT rtCtxCreate(void **ctx, uint32_t flags, int32_t device);
MSPTI_API RtErrorT rtCtxDestroy(void **ctx);
MSPTI_API RtErrorT rtStreamCreate(RtStreamT *stream, int32_t priority);
MSPTI_API RtErrorT rtStreamDestroy(RtStreamT stream);
MSPTI_API RtErrorT rtStreamSynchronize(RtStreamT stream);
MSPTI_API RtErrorT rtKernelLaunch(const void *stubFunc, uint32_t blockDim, void *args,
                                  uint32_t argsSize, RtSmDescT *smDesc, RtStreamT stream);
MSPTI_API RtErrorT rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim,
                                            RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stream, uint32_t flags,
                                            const RtTaskCfgInfoT *cfgInfo);
MSPTI_API RtErrorT rtKernelLaunchWithHandleV2(void *handle, const uint64_t tilingkey, uint32_t blockDim,
                                              RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stream,
                                              const RtTaskCfgInfoT *cfgInfo);
MSPTI_API RtErrorT rtAicpuKernelLaunchExWithArgs(const uint32_t kernelType, const char* const opName,
                                                 const uint32_t blockDim, const RtAicpuArgsExT *argsInfo,
                                                 RtSmDescT * const smDesc, const RtStreamT stm, const uint32_t flags);
MSPTI_API RtErrorT rtLaunchKernelByFuncHandle(rtFuncHandle funcHandle, uint32_t blockDim,
                                              rtLaunchArgsHandle argsHandle, RtStreamT stm);
MSPTI_API RtErrorT rtLaunchKernelByFuncHandleV2(rtFuncHandle funcHandle, uint32_t blockDim,
                                                rtLaunchArgsHandle argsHandle, RtStreamT stm,
                                                const RtTaskCfgInfoT *cfgInfo);
MSPTI_API RtErrorT rtVectorCoreKernelLaunch(const VOID_PTR stubFunc, uint32_t blockDim, RtArgsExT *argsInfo,
                                            RtSmDescT *smDesc, RtStreamT stm, uint32_t flags,
                                            const RtTaskCfgInfoT *cfgInfo);
MSPTI_API RtErrorT rtVectorCoreKernelLaunchWithHandle(VOID_PTR hdl, const uint64_t tilingKey, uint32_t blockDim,
                                                      RtArgsExT *argsInfo, RtSmDescT *smDesc, RtStreamT stm,
                                                      const RtTaskCfgInfoT *cfgInfo);

// Inner
RtErrorT rtGetDevice(int32_t *devId);
RtErrorT rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, RtStreamT stm);
RtErrorT rtGetStreamId(RtStreamT stm, int32_t *streamId);
RtErrorT rtBinaryGetFunction(VOID_PTR binHandle, uint64_t tilingKey, VOID_PTR_PTR funcHandle);
RtErrorT rtProfSetProSwitch(VOID_PTR data, uint32_t len);
RtErrorT rtGetVisibleDeviceIdByLogicDeviceId(const int32_t logicDeviceId, int32_t * const visibleDeviceId);

#if defined(__cplusplus)
}
#endif

#endif
