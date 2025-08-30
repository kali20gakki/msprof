/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acl_inject.h
 * Description        : Injection of acl.
 * Author             : msprof team
 * Creation Date      : 2025/08/27
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_ACL_INJECT_H
#define MSPTI_COMMON_INJECT_ACL_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Inject
MSPTI_API AclError aclrtSetDevice(int32_t deviceId);
MSPTI_API AclError aclrtResetDevice(int32_t deviceId);
MSPTI_API AclError aclrtCreateContext(AclrtContext *context, int32_t deviceId);
MSPTI_API AclError aclrtDestroyContext(AclrtContext context);
MSPTI_API AclError aclrtCreateStream(AclrtStream *stream);
MSPTI_API AclError aclrtDestroyStream(AclrtStream stream);
MSPTI_API AclError aclrtSynchronizeStream(AclrtStream stream);
MSPTI_API AclError aclrtLaunchKernel(AclrtFuncHandle funcHandle, uint32_t blockDim,
                                     const void *argsData, size_t argsSize, AclrtStream stream);
MSPTI_API AclError aclrtLaunchKernelV2(AclrtFuncHandle funcHandle, uint32_t blockDim, const void *argsData,
                                       size_t argsSize, AclrtLaunchKernelCfg *cfg, AclrtStream stream);
MSPTI_API AclError aclrtLaunchKernelWithConfig(AclrtFuncHandle funcHandle, uint32_t blockDim, AclrtStream stream,
                                               AclrtLaunchKernelCfg *cfg, AclrtArgsHandle argsHandle, void *reserve);
MSPTI_API AclError aclrtLaunchKernelWithHostArgs(AclrtFuncHandle funcHandle, uint32_t blockDim, AclrtStream stream,
                                                 AclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
                                                 AclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum);

// Inner
AclError aclrtGetDevice(int32_t *deviceId);
AclError aclrtStreamGetId(AclrtStream stream, int32_t *streamId);
AclError aclrtProfTrace(void *userdata, int32_t length, AclrtStream stream);
AclError aclrtBinaryGetFunctionByEntry(AclrtBinHandle binHandle, uint64_t funcEntry, AclrtFuncHandle *funcHandle);
AclError aclrtGetLogicDevIdByUserDevId(const int32_t userDevid, int32_t *const logicDevId);

AclError profTraceInner(uint64_t indexId, uint64_t modelId, uint16_t tagId, AclrtStream stream);

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_COMMON_INJECT_ACL_INJECT_H
