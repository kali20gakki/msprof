/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef __INC_LLT_RUNTIME_STUB_H
#define __INC_LLT_RUNTIME_STUB_H

#include <vector>
#include "runtime/rt.h"
#include <memory>

namespace ge {
class RuntimeStub {
 public:
  virtual ~RuntimeStub() = default;

  static RuntimeStub* GetInstance();

  static void SetInstance(const std::shared_ptr<RuntimeStub> &instance) {
    instance_ = instance;
  }

  static void Reset() {
    instance_.reset();
  }

  virtual rtError_t rtKernelLaunchEx(void *args, uint32_t args_size, uint32_t flags, rtStream_t stream) {
    return RT_ERROR_NONE;
  }

  virtual rtError_t rtKernelLaunch(const void *stub_func,
                                   uint32_t block_dim,
                                   void *args,
                                   uint32_t args_size,
                                   rtSmDesc_t *sm_desc,
                                   rtStream_t stream) {
    return RT_ERROR_NONE;
  }
  virtual rtError_t rtKernelLaunchWithFlag(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                                   rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flag) {
    return RT_ERROR_NONE;
  }
  virtual rtError_t rtCpuKernelLaunchWithFlag(const void *soName, const void *kernelName, uint32_t blockDim,
                                                const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream,
                                                uint32_t flags) {
    return RT_ERROR_NONE;
  }
  virtual rtError_t rtAicpuKernelLaunchWithFlag(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
                                                  const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream,
                                                  uint32_t flags) {
    return RT_ERROR_NONE;
  }
  virtual rtError_t rtGetIsHeterogenous(int32_t *heterogeneous) {
    return RT_ERROR_NONE;
  }

  virtual rtError_t rtGetDeviceCount(int32_t *count) {
    *count = 1;
    return RT_ERROR_NONE;
  }

  virtual rtError_t rtMemcpy(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind);

  virtual rtError_t rtMalloc(void **dev_ptr, uint64_t size, rtMemType_t type);

  virtual rtError_t rtFree(void *dev_ptr);

  virtual rtError_t rtEschedWaitEvent(int32_t device_id,
                                      uint32_t group_id,
                                      uint32_t thread_id,
                                      int32_t timeout,
                                      rtEschedEventSummary_t *event);


 private:
  static std::shared_ptr<RuntimeStub> instance_;
};
}  // namespace ge

#ifdef __cplusplus
extern "C" {
#endif
void rtStubTearDown();

#define RTS_STUB_SETUP()    \
do {                        \
  rtStubTearDown();         \
} while (0)

#define RTS_STUB_TEARDOWN() \
do {                        \
  rtStubTearDown();         \
} while (0)

#define RTS_STUB_RETURN_VALUE(FUNC, TYPE, VALUE)                          \
do {                                                                      \
  g_Stub_##FUNC##_RETURN.emplace(g_Stub_##FUNC##_RETURN.begin(), VALUE);  \
} while (0)

#define RTS_STUB_OUTBOUND_VALUE(FUNC, TYPE, NAME, VALUE)                          \
do {                                                                              \
  g_Stub_##FUNC##_OUT_##NAME.emplace(g_Stub_##FUNC##_OUT_##NAME.begin(), VALUE);  \
} while (0)

extern std::string g_runtime_stub_mock;

#define RTS_STUB_RETURN_EXTERN(FUNC, TYPE) extern std::vector<TYPE> g_Stub_##FUNC##_RETURN;
#define RTS_STUB_OUTBOUND_EXTERN(FUNC, TYPE, NAME) extern std::vector<TYPE> g_Stub_##FUNC##_OUT_##NAME;

RTS_STUB_RETURN_EXTERN(rtGetDevice, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtGetDevice, int32_t, device)

RTS_STUB_RETURN_EXTERN(rtGetDeviceCapability, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtGetDeviceCapability, int32_t, value);

RTS_STUB_RETURN_EXTERN(rtGetRtCapability, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtGetRtCapability, int32_t, value);

RTS_STUB_RETURN_EXTERN(rtGetTsMemType, uint32_t);

RTS_STUB_RETURN_EXTERN(rtStreamWaitEvent, rtError_t);

RTS_STUB_RETURN_EXTERN(rtEventReset, rtError_t);

RTS_STUB_RETURN_EXTERN(rtEventCreate, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtEventCreate, rtEvent_t, event);

RTS_STUB_RETURN_EXTERN(rtGetEventID, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtEventCreate, uint32_t, event_id);

RTS_STUB_RETURN_EXTERN(rtQueryFunctionRegistered, rtError_t);

RTS_STUB_RETURN_EXTERN(rtGetAicpuDeploy, rtError_t);
RTS_STUB_OUTBOUND_EXTERN(rtGetAicpuDeploy, rtAicpuDeployType_t, value);

RTS_STUB_RETURN_EXTERN(rtProfilerTraceEx, rtError_t);

#ifdef __cplusplus
}
#endif
#endif // __INC_LLT_RUNTIME_STUB_H
