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

typedef struct TagRtTaskCfgInfo {
    uint8_t qos;
    uint8_t partId;
    uint8_t schemMode;
    uint8_t res[1];
} RtTaskCfgInfoT;

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
    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flags, const RtTaskCfgInfoT *cfgInfo);
MSPTI_API rtError_t rtKernelLaunchWithHandleV2(
    void *handle, const uint64_t tilingkey, uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc,
    rtStream_t stream, const RtTaskCfgInfoT *cfgInfo);
// Inner
rtError_t RtGetDevice(int32_t *devId);
rtError_t RtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stm);
rtError_t RtGetStreamId(rtStream_t stm, int32_t *streamId);

#if defined(__cplusplus)
}
#endif

#endif
