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

#include <string>
#include <map>
#include <queue>

#include "securec.h"
#include "cce/dnn.h"
#include "aicpu/aicpu_schedule/aicpu_op_type_list.h"
#include "mmpa/mmpa_api.h"
#include "runtime_stub.h"
#include "runtime/rt.h"

extern std::string g_runtime_stub_mock;
std::string g_runtime_stub_mock = "";

#define ADD_STUB_RETURN_VALUE(FUNC, TYPE) std::vector<TYPE> g_Stub_##FUNC##_RETURN

#define GET_STUB_RETURN_VALUE(FUNC, TYPE, DEFAULT) ({   \
  TYPE result = DEFAULT;                                \
  if (!g_Stub_##FUNC##_RETURN.empty()) {                \
    result = g_Stub_##FUNC##_RETURN.back();             \
    g_Stub_##FUNC##_RETURN.pop_back();                  \
  }                                                     \
  result;                                               \
})

#define DEL_STUB_RETURN_VALUE(FUNC, TYPE)           \
do {                                                \
  extern std::vector<TYPE> g_Stub_##FUNC##_RETURN;  \
  g_Stub_##FUNC##_RETURN.clear();                   \
} while (0)


#define ADD_STUB_OUTBOUND_VALUE(FUNC, TYPE, NAME) std::vector<TYPE> g_Stub_##FUNC##_OUT_##NAME

#define GET_STUB_OUTBOUND_VALUE(FUNC, TYPE, NAME, DEFAULT) ({ \
  TYPE value;                                                 \
  if (!g_Stub_##FUNC##_OUT_##NAME.empty()) {                  \
    value = g_Stub_##FUNC##_OUT_##NAME.back();                \
    g_Stub_##FUNC##_OUT_##NAME.pop_back();                    \
  } else {                                                    \
    value = DEFAULT;                                          \
  }                                                           \
  value;                                                      \
})

#define DEL_STUB_OUTBOUND_VALUE(FUNC, TYPE, NAME)       \
do {                                                    \
  extern std::vector<TYPE> g_Stub_##FUNC##_OUT_##NAME;  \
  g_Stub_##FUNC##_OUT_##NAME.clear();                   \
} while (0)


namespace ge {
namespace {
struct MbufStub {
  explicit MbufStub(uint64_t size) {
    length = size;
    if (size > 0) {
      buffer = new uint8_t[size];
    }
  }
  ~MbufStub() {
    delete []buffer;
  }
  uint8_t *buffer = nullptr;
  uint64_t length = 0;
};

std::map<int32_t, std::map<uint32_t, std::queue<void *>>> mem_queues_;
}  // namespace

std::shared_ptr<RuntimeStub> RuntimeStub::instance_;
RuntimeStub *RuntimeStub::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = std::make_shared<RuntimeStub>();
  }
  return instance_.get();
}

rtError_t RuntimeStub::rtMemcpy(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  if (dst != nullptr && src != nullptr) {
    memcpy_s(dst, dest_max, src, count);
  }
  return RT_ERROR_NONE;
}

rtError_t RuntimeStub::rtMalloc(void **dev_ptr, uint64_t size, rtMemType_t type) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_2";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  if (std::string(__FUNCTION__) == g_runtime_stub_mock) {
    return -1;
  }

  if (size == 123) {
    return -1;
  }
  if (size > INT32_MAX) {
    *dev_ptr = nullptr;
    return RT_ERROR_NONE;
  }
  *dev_ptr = new uint8_t[size];
  memset_s(*dev_ptr, size, 0, size);

  return RT_ERROR_NONE;
}

rtError_t RuntimeStub::rtFree(void *dev_ptr) {
  delete[](uint8_t *) dev_ptr;
  return RT_ERROR_NONE;
}

rtError_t RuntimeStub::rtEschedWaitEvent(int32_t device_id,
                                         uint32_t group_id,
                                         uint32_t thread_id,
                                         int32_t timeout,
                                         rtEschedEventSummary_t *event) {
  return RT_ERROR_NONE;
}
}

#ifdef __cplusplus
extern "C" {
#endif
#define EVENT_LENTH 10

void rtStubTearDown() {
  DEL_STUB_RETURN_VALUE(rtGetDevice, rtError_t);
  DEL_STUB_RETURN_VALUE(rtGetDeviceCapability, rtError_t);
  DEL_STUB_RETURN_VALUE(rtStreamWaitEvent, rtError_t);
  DEL_STUB_RETURN_VALUE(rtEventReset, rtError_t);
  DEL_STUB_RETURN_VALUE(rtEventCreate, rtError_t);
  DEL_STUB_RETURN_VALUE(rtGetEventID, rtError_t);
  DEL_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t);
}

ADD_STUB_RETURN_VALUE(rtGetDevice, rtError_t);
rtError_t rtGetDevice(int32_t *device) {
  if (__FUNCTION__ == g_runtime_stub_mock) {
    return -1;
  }
  return GET_STUB_RETURN_VALUE(rtGetDevice, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtGetDeviceCapability, rtError_t);
ADD_STUB_OUTBOUND_VALUE(rtGetDeviceCapability, int32_t, value);
rtError_t rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value) {
  *value = GET_STUB_OUTBOUND_VALUE(rtGetDeviceCapability, int32_t, value, RT_AICPU_BLOCKING_OP_SUPPORT);
  return GET_STUB_RETURN_VALUE(rtGetDeviceCapability, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtStreamWaitEvent, rtError_t);
rtError_t rtStreamWaitEvent(rtStream_t stream, rtEvent_t event) {
  return GET_STUB_RETURN_VALUE(rtStreamWaitEvent, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtEventReset, rtError_t);
rtError_t rtEventReset(rtEvent_t event, rtStream_t stream) {
  return GET_STUB_RETURN_VALUE(rtEventReset, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtEventCreate, rtError_t);
rtError_t rtEventCreate(rtEvent_t *event) {
  if (__FUNCTION__ == g_runtime_stub_mock) {
    return -1;
  }
  *event = new int[EVENT_LENTH];
  return GET_STUB_RETURN_VALUE(rtEventCreate, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtGetEventID, rtError_t);
rtError_t rtGetEventID(rtEvent_t event, uint32_t *event_id) {
  if (__FUNCTION__ == g_runtime_stub_mock) {
    return -1;
  }
  *event_id = 0;
  return GET_STUB_RETURN_VALUE(rtEventCreate, rtError_t, RT_ERROR_NONE);
}

ADD_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t);
rtError_t rtQueryFunctionRegistered(const char *stub_name) {
  return GET_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, RT_ERROR_NONE);
}

rtError_t rtCtxSetCurrent(rtContext_t ctx)
{
  const char * const kEnvRecordPath = "SET_TRANS_VAR_DATA";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtGetStreamId(rtStream_t stream, int32_t *stream_id) {
  *stream_id = 0;
  return RT_ERROR_NONE;
}

rtError_t rtCtxGetCurrent(rtContext_t *ctx) {
  if (__FUNCTION__ == g_runtime_stub_mock) {
    return -1;
  }
  uintptr_t x = 1;
  *ctx = (rtContext_t *)x;
  return RT_ERROR_NONE;
}

rtError_t rtCtxSetDryRun(rtContext_t ctx, rtDryRunFlag_t enable, uint32_t flag) { return RT_ERROR_NONE; }

rtError_t rtEventGetTimeStamp(uint64_t *time, rtEvent_t event) {
  *time = 12345;
  return RT_ERROR_NONE;
}

rtError_t rtEventCreateWithFlag(rtEvent_t *event, uint32_t flag) {
  return rtEventCreate(event);
}

rtError_t rtEventRecord(rtEvent_t event, rtStream_t stream) { return RT_ERROR_NONE; }

rtError_t rtEventSynchronize(rtEvent_t event) { return RT_ERROR_NONE; }

rtError_t rtEventDestroy(rtEvent_t event) {
  delete[](int *) event;
  return RT_ERROR_NONE;
}

rtError_t rtMemset(void *dev_ptr, uint64_t dest_max, uint32_t value, uint64_t count) {
  if (dest_max == 321) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtMalloc(void **dev_ptr, uint64_t size, rtMemType_t type) {
  return ge::RuntimeStub::GetInstance()->rtMalloc(dev_ptr, size, type);
}

rtError_t rtFree(void *dev_ptr) {
  return ge::RuntimeStub::GetInstance()->rtFree(dev_ptr);
}

rtError_t rtMallocHost(void **host_ptr, uint64_t size) {
  *host_ptr = new uint8_t[size];
  return RT_ERROR_NONE;
}

rtError_t rtFreeHost(void *host_ptr) {
  delete[](uint8_t *) host_ptr;
  return RT_ERROR_NONE;
}

rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_4";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  *stream = new uint32_t;
  return RT_ERROR_NONE;
}

rtError_t rtStreamDestroy(rtStream_t stream) {
  if (stream != nullptr) {
    delete (uint32_t *)stream;
  }
  return RT_ERROR_NONE;
}

rtError_t rtSetDie(int32_t die) { return RT_ERROR_NONE; }
rtError_t rtSetDevice(int32_t device) { return RT_ERROR_NONE; }
rtError_t rtSetDeviceV2(int32_t device, rtDeviceMode deviceMode) { return RT_ERROR_NONE; }

rtError_t rtStreamSynchronize(rtStream_t stream) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_9";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtMemcpy(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind) {
  if (std::string(__FUNCTION__) == g_runtime_stub_mock) {
    return -1;
  }
  return ge::RuntimeStub::GetInstance()->rtMemcpy(dst, dest_max, src, count, kind);
}

rtError_t rtMemcpyAsync(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind,
                        rtStream_t stream) {
  if (dst != nullptr && src != nullptr) {
    memcpy_s(dst, dest_max, src, count);
  }
  return RT_ERROR_NONE;
}

rtError_t rtSetTSDevice(uint32_t tsId) {
  return RT_ERROR_NONE;
}

rtError_t rtGetDeviceCount(int32_t *count) {
  return ge::RuntimeStub::GetInstance()->rtGetDeviceCount(count);
}

rtError_t rtDeviceReset(int32_t device) { return RT_ERROR_NONE; }

rtError_t rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end) {
  *time = 10.0f;
  return RT_ERROR_NONE;
}

rtError_t rtFunctionRegister(void *bin_handle, const void *stub_func, const char *stub_name, const void *dev_func,
                             uint32_t func_mode) {
  if (reinterpret_cast<uintptr_t>(bin_handle) == 99) {
    return -1;
  }
  if (stub_name != nullptr && stub_name[0] == 'Z') {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle) { return RT_ERROR_NONE; }

rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void **handle) {
  *handle = (void*)0x12345678;
  return RT_ERROR_NONE;
}

rtError_t rtKernelConfigTransArg(const void *ptr, uint64_t size, uint32_t flag, void **arg) { return RT_ERROR_NONE; }

rtError_t rtKernelLaunchWithHandle(void *handle, const void *devFunc, uint32_t blockDim, rtArgsEx_t *args,
                                   rtSmDesc_t *smDesc, rtStream_t stream, const void *kernelInfo) {
  if (blockDim == 99) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtKernelLaunch(const void *stub_func, uint32_t block_dim, void *args, uint32_t args_size, rtSmDesc_t *sm_desc,
                         rtStream_t stream) {
  if (block_dim == 99) {
    return -1;
  }
  return ge::RuntimeStub::GetInstance()->rtKernelLaunch(stub_func, block_dim, args, args_size, sm_desc, stream);
}

rtError_t rtKernelLaunchWithFlag(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                                 rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flag) {
  return ge::RuntimeStub::GetInstance()->rtKernelLaunchWithFlag(stubFunc, blockDim, argsInfo, smDesc, stream, flag);
}
rtError_t rtSetupArgument(const void *arg, uint32_t size, uint32_t offset) { return RT_ERROR_NONE; }
rtError_t rtLaunch(const void *stub_func) { return RT_ERROR_NONE; }
rtError_t rtDevBinaryUnRegister(void *handle) { return RT_ERROR_NONE; }
rtError_t rtConfigureCall(uint32_t num_blocks, rtSmDesc_t *sm_desc, rtStream_t stream) { return RT_ERROR_NONE; }

rtError_t rtSetProfDir(char *prof_dir) { return RT_ERROR_NONE; }

rtError_t rtSetProfDirEx(const char *profDir, const char *address, const char *jobCtx) { return RT_ERROR_NONE; }

rtError_t rtAiCoreMemorySizes(rtAiCoreMemorySize_t *aicore_memory_size) { return RT_ERROR_NONE; }

rtError_t rtSetKernelReportCallback(rtKernelReportCallback callback) {
  rtKernelInfo rt_kernel_info = {0};
  rt_kernel_info.arg_size = 12;
  rt_kernel_info.task_offset = 100;
  rt_kernel_info.arg = (void *)100;
  rt_kernel_info.module_addr = (void *)100;
  rt_kernel_info.module_size = 100;

  rtStream_t stream = nullptr;
  callback(stream, &rt_kernel_info);
  return RT_ERROR_NONE;
}

rtError_t rtMemAdvise(void *ptr, uint64_t size, uint32_t advise) { return RT_ERROR_NONE; }

/// @ingroup rt_kernel
/// @brief start fusion kernels.
/// @param [in] stream   stream for fusion kernels
/// @return RT_ERROR_NONE for ok, errno for failed
rtError_t rtKernelFusionStart(rtStream_t stream) { return RT_ERROR_NONE; }

/// @ingroup rt_kernel
/// @brief end fusion kernels.
/// @param [in] stream   stream for fusion kernels
/// @return RT_ERROR_NONE for ok, errno for failed
rtError_t rtKernelFusionEnd(rtStream_t stream) { return RT_ERROR_NONE; }
rtError_t rtMemGetInfo(size_t *free, size_t *total) {
  *free = 64UL * 1024UL * 1024UL;
  *total = 128UL * 1024UL * 1024UL;
  return RT_ERROR_NONE;
}

rtError_t rtMemGetInfoEx(rtMemInfoType_t memInfoType, size_t *free, size_t *total) {
  *free = 64UL * 1024UL * 1024UL;
  *total = 128UL * 1024UL * 1024UL;
  return RT_ERROR_NONE;
}

rtError_t rtMemAllocManaged(void **ptr, uint64_t size, uint32_t flag) {
  *ptr = malloc(size);
  return RT_ERROR_NONE;
}

rtError_t rtMemFreeManaged(void *ptr) {
  free(ptr);
  return RT_ERROR_NONE;
}

rtError_t rtMetadataRegister(void *handle, const char *meta_data) {
  if (reinterpret_cast<uintptr_t>(handle) == 99) {
    return -1;
  }
  return RT_ERROR_NONE;
}
rtError_t rtSetTaskGenCallback(rtTaskGenCallback callback) { return RT_ERROR_NONE; }

rtError_t rtModelCreate(rtModel_t *model, uint32_t flag) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_3";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  *model = new uint32_t;
  return RT_ERROR_NONE;
}

rtError_t rtModelDestroy(rtModel_t model) {
  uint32_t *stub = static_cast<uint32_t *>(model);
  delete stub;
  return RT_ERROR_NONE;
}

rtError_t rtModelBindStream(rtModel_t model, rtStream_t stream, uint32_t flag) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_5";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  return RT_ERROR_NONE;
}

rtError_t rtModelUnbindStream(rtModel_t model, rtStream_t stream) { return RT_ERROR_NONE; }
rtError_t rtModelExecute(rtModel_t model, rtStream_t stream, uint32_t flag) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_8";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtGetFunctionByName(const char *stub_name, void **stub_func) {
  *(char **)stub_func = (char *)("func");
  return RT_ERROR_NONE;
}
rtError_t rtGetAddrByFun(const void *stubFunc, void **addr) {
  *(char **)addr = (char *)("dev_func");
  return RT_ERROR_NONE;
}

rtError_t rtCtxCreate(rtContext_t *ctx, uint32_t flags, int32_t device) { return RT_ERROR_NONE; }
rtError_t rtCtxCreateV2(rtContext_t *ctx,
                        uint32_t flags,
                        int32_t device,
                        rtDeviceMode deviceMode) { return RT_ERROR_NONE; }

rtError_t rtKernelLaunchEx(void *args, uint32_t args_size, uint32_t flags, rtStream_t stream_) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_6";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  return ge::RuntimeStub::GetInstance()->rtKernelLaunchEx(args, args_size, flags, stream_);
}

rtError_t rtCpuKernelLaunch(const void *so_name, const void *kernel_name, uint32_t block_dim, const void *args,
                            uint32_t args_size, rtSmDesc_t *sm_desc, rtStream_t stream) {
  if (std::string(__FUNCTION__) == g_runtime_stub_mock) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtModelGetTaskId(void *handle, uint32_t *task_id, uint32_t *stream_id) {
  if (std::string(__FUNCTION__) == g_runtime_stub_mock) {
    return -1;
  }
  *task_id = 0;
  *stream_id = 0;
  return RT_ERROR_NONE;
}
rtError_t rtEndGraph(rtModel_t model, rtStream_t stream) { return RT_ERROR_NONE; }
rtError_t rtEndGraphEx(rtModel_t model, rtStream_t stream, uint32_t flags)
{
  return RT_ERROR_NONE;
}
rtError_t rtProfilerStop(uint64_t profConfig, int32_t numsDev, uint32_t *deviceList) {
  return RT_ERROR_NONE;
}

rtError_t rtSetDvfsProfile(DvfsProfileMode mode) { return RT_ERROR_NONE; }

rtError_t rtUnsetDvfsProfile() { return RT_ERROR_NONE; }

rtError_t rtGetDvfsProfile(DvfsProfileMode *pmode) { return RT_ERROR_NONE; }

rtError_t rtCtxDestroy(rtContext_t ctx) { return RT_ERROR_NONE; }

rtError_t rtProfilerInit(const char *prof_dir, const char *address, const char *job_ctx) { return RT_ERROR_NONE; }

rtError_t rtProfilerStart(uint64_t profConfig, int32_t numsDev, uint32_t *deviceList) {
  return RT_ERROR_NONE;
}

rtError_t rtLabelCreate(rtLabel_t *label) {
  *label = new uint64_t;
  return RT_ERROR_NONE;
}

rtError_t rtLabelCreateEx(rtLabel_t *label, rtStream_t stream) {
  *label = new uint64_t;
  return RT_ERROR_NONE;
}

rtError_t rtLabelCreateV2(rtLabel_t *label, rtModel_t model) {
  *label = new uint64_t;
  return RT_ERROR_NONE;
}

rtError_t rtLabelCreateExV2(rtLabel_t *label, rtModel_t model, rtStream_t stream) {
  *label = new uint64_t;
  return RT_ERROR_NONE;
}

rtError_t rtLabelListCpy(rtLabel_t *label, uint32_t labelNumber, void *dst, uint32_t dstMax) {
  return RT_ERROR_NONE;
}

rtError_t rtLabelDestroy(rtLabel_t label) {
  uint64_t *stub = static_cast<uint64_t *>(label);
  delete stub;
  return RT_ERROR_NONE;
}

rtError_t rtLabelSet(rtLabel_t label, rtStream_t stream) { return RT_ERROR_NONE; }

rtError_t rtLabelSwitch(void *ptr, rtCondition_t condition, uint32_t value, rtLabel_t true_label, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtLabelSwitchByIndex(void *ptr, uint32_t max, void *labelInfoPtr, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtLabelGoto(rtLabel_t label, rtStream_t stream) { return RT_ERROR_NONE; }

rtError_t rtLabelGotoEx(rtLabel_t label, rtStream_t stream) {
  return RT_ERROR_NONE;
}


rtError_t rtInvalidCache(void *base, size_t len) {
  return RT_ERROR_NONE;
}

rtError_t rtModelLoadComplete(rtModel_t model) {
  const char * const kEnvRecordPath = "CONSTANT_FOLDING_PASS_7";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  return RT_ERROR_NONE;
}

rtError_t rtStreamCreateWithFlags(rtStream_t *stream, int32_t priority, uint32_t flags) {
  *stream = new uint32_t;
  return RT_ERROR_NONE;
}

rtError_t rtFlushCache(void *base, size_t len) {
  return RT_ERROR_NONE;
}

rtError_t rtProfilerTrace(uint64_t id, bool notify, uint32_t flags, rtStream_t stream_) { return RT_ERROR_NONE; }

ADD_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t);
rtError_t rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stream) {
  return GET_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t, RT_ERROR_NONE);
}

rtError_t rtMemSetRC(const void *dev_ptr, uint64_t size, uint32_t read_count) { return RT_ERROR_NONE; }

rtError_t rtStreamSwitch(void *ptr, rtCondition_t condition, int64_t value, rtStream_t true_stream, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtStreamSwitchN(void *ptr, uint32_t size, void *valuePtr, rtStream_t *trueStreamPtr, uint32_t elementSize,
                          rtStream_t stream, rtSwitchDataType_t dataType) {
  return RT_ERROR_NONE;
}

rtError_t rtStreamSwitchEx(void *ptr, rtCondition_t condition, void *value_ptr, rtStream_t true_stream,
                           rtStream_t stream, rtSwitchDataType_t data_type) {
  return RT_ERROR_NONE;
}

rtError_t rtStreamActive(rtStream_t active_stream, rtStream_t stream) { return RT_ERROR_NONE; }

rtError_t rtDatadumpInfoLoad(const void *dump_info, uint32_t length) { return RT_ERROR_NONE; }

rtError_t rtCpuKernelLaunchWithFlag(const void *so_name, const void *kernel_name, uint32_t core_dim,
                                    const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream_, uint32_t flags) {
  return ge::RuntimeStub::GetInstance()->rtCpuKernelLaunchWithFlag(so_name, kernel_name, core_dim, args, smDesc,
                                                                   stream_, flags);
}

rtError_t rtModelSetExtId(rtModel_t model, uint32_t extId)
{
  return RT_ERROR_NONE;
}

rtError_t rtModelGetId(rtModel_t model, uint32_t *modelId)
{
  return RT_ERROR_NONE;
}

rtError_t rtModelBindQueue(rtModel_t model, uint32_t queueId, rtModelQueueFlag_t flag)
{
  return RT_ERROR_NONE;
}

rtError_t rtSetSocVersion(const char *version)
{
  return RT_ERROR_NONE;
}

rtError_t rtGetSocVersion(char *version, const uint32_t maxLen)
{
  return RT_ERROR_NONE;
}

rtError_t rtGetAiCoreCount(uint32_t *aiCoreCnt)
{
  return RT_ERROR_NONE;
}

rtError_t rtGetAiCpuCount(uint32_t *aiCpuCnt)
{
  return RT_ERROR_NONE;
}

RTS_API rtError_t rtSetOpWaitTimeOut(uint32_t timeout)
{
  return RT_ERROR_NONE;
}

RTS_API rtError_t rtSetOpExecuteTimeOut(uint32_t timeout)
{
  return RT_ERROR_NONE;
}

rtError_t rtSetTaskFailCallback(rtTaskFailCallback callback)
{
  return RT_ERROR_NONE;
}

rtError_t rtMallocHostSharedMemory(rtMallocHostSharedMemoryIn *in,
		                               rtMallocHostSharedMemoryOut *out)
{
  out->ptr = new uint8_t[in->size];
  out->devPtr = new uint8_t[in->size];
  return RT_ERROR_NONE;
}

rtError_t rtFreeHostSharedMemory(rtFreeHostSharedMemoryIn *in)
{
  delete[] (uint8_t*)in->ptr;
  delete[] (uint8_t*)in->devPtr;
  return RT_ERROR_NONE;
}

ADD_STUB_RETURN_VALUE(rtGetAicpuDeploy, rtError_t);
ADD_STUB_OUTBOUND_VALUE(rtGetAicpuDeploy, rtAicpuDeployType_t, value);
rtError_t rtGetAicpuDeploy(rtAicpuDeployType_t *deplyType)
{
  *deplyType = GET_STUB_OUTBOUND_VALUE(rtGetAicpuDeploy, rtAicpuDeployType_t, value, AICPU_DEPLOY_CROSS_PROCESS);
  return GET_STUB_RETURN_VALUE(rtGetAicpuDeploy, rtError_t, RT_ERROR_NONE);
}

rtError_t rtDebugRegister(rtModel_t model, uint32_t flag, const void *addr, uint32_t *streamId, uint32_t *taskId)
{
  return RT_ERROR_NONE;
}

rtError_t rtDebugUnRegister(rtModel_t model)
{
  return RT_ERROR_NONE;
}

rtError_t rtDumpAddrSet(rtModel_t model, void *addr, uint32_t dumpSize, uint32_t flag)
{
  if (__FUNCTION__ == g_runtime_stub_mock) {
    return -1;
  }
  return RT_ERROR_NONE;
}

rtError_t rtSetCtxINFMode(bool mode)
{
  return RT_ERROR_NONE;
}

rtError_t rtGetRtCapability(rtFeatureType_t featureType, int32_t featureInfo, int64_t *value)
{
  const char * const kEnvRecordPath = "SET_CAPA_VALUE";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    *value = 1;
  }

  return RT_ERROR_NONE;
}

uint32_t rtGetTsMemType(rtMemRequestFeature_t featureType, uint32_t memSize) {
  return RT_MEMORY_HBM;
}

rtError_t rtGetMaxStreamAndTask(uint32_t streamType, uint32_t *maxStrCount, uint32_t *maxTaskCount)
{
  *maxStrCount = 1024;
  *maxTaskCount = 1024;
  return RT_ERROR_NONE;
}

rtError_t rtModelExit(rtModel_t model, rtStream_t stream)
{
 return RT_ERROR_NONE;
}

rtError_t rtGetTaskIdAndStreamID(uint32_t *taskId, uint32_t *streamId)
{
 if (*taskId == 999 || *streamId == 999) {
  return -1;
 }
 return RT_ERROR_NONE;
}

rtError_t rtDebugRegisterForStream(rtStream_t stream, uint32_t flag, const void *addr, uint32_t *streamId, uint32_t *taskId) {
  return RT_ERROR_NONE;
}

rtError_t rtDebugUnRegisterForStream(rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtFftsTaskLaunch(rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtKernelGetAddrAndPrefCnt(void *handle, const uint64_t tilingKey, const void *const stubFunc,
                                    const uint32_t flag, void **addr, uint32_t *prefetchCnt) {
  return RT_ERROR_NONE;
}

rtError_t rtFftsPlusTaskLaunch(rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtStarsTaskLaunch(void *address, uint32_t len, void *stream) {
  return RT_ERROR_NONE;
}

rtError_t rtKernelLaunchFwk(const char *opName, void *args, uint32_t argSize, uint32_t flags, rtStream_t rtStream) {
  return RT_ERROR_NONE;
}

rtError_t rtAicpuKernelLaunchWithFlag(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
                                         const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream,
                                         uint32_t flags) {
  return ge::RuntimeStub::GetInstance()->rtAicpuKernelLaunchWithFlag(launchNames, blockDim, args, smDesc, stream,
                                                                     flags);
}

rtError_t rtAicpuKernelLaunch(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim, const void *args,
                              uint32_t argSize, rtSmDesc_t *smDesc, rtStream_t stream) {
  return RT_ERROR_NONE;
}

rtError_t rtSetDeviceIdByGeModelIdx(uint32_t modelIdx, uint32_t deviceId) {
  return RT_ERROR_NONE;
}

rtError_t rtUnsetDeviceIdByGeModelIdx(uint32_t modelIdx, uint32_t deviceId) {
  return RT_ERROR_NONE;
}

rtError_t rtProfRegisterCtrlCallback(uint32_t logId, rtProfCtrlHandle callback) {
  return RT_ERROR_NONE;
}

rtError_t rtFftsPlusTaskLaunchWithFlag(rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stream, uint32_t flag) {
  return RT_ERROR_NONE;
}

rtError_t rtFftsTaskLaunchWithFlag(rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stream, uint32_t flag) {
  return RT_ERROR_NONE;
}

rtError_t rtGetC2cCtrlAddr(uint64_t *addr, uint32_t *len) {
  return RT_ERROR_NONE;
}

rtError_t rtGetDevMsg(rtGetDevMsgType_t getMsgType, rtGetMsgCallback callback) {
  const char * const kEnvRecordPath = "NPU_COLLECT_PATH";
  char record_path[MMPA_MAX_PATH] = {};
  (void)mmGetEnv(kEnvRecordPath, &record_path[0], static_cast<uint32_t>(MMPA_MAX_PATH));

  if (std::string(&record_path[0]).find("mock_fail") != std::string::npos) {
    return -1;
  }

  const char *snapshot = "snapshot";
  callback(snapshot, strlen(snapshot));
  return RT_ERROR_NONE;
}

rtError_t rtSetTaskTag(const char *taskTag) {
  return RT_ERROR_NONE;
}

rtError_t rtSetAicpuAttr(const char *key, const char *val) {
  return RT_ERROR_NONE;
}

rtError_t rtGetIsHeterogenous(int32_t *heterogeneous) {
  return ge::RuntimeStub::GetInstance()->rtGetIsHeterogenous(heterogeneous);
}

rtError_t rtMemQueueGetQidByName(int32_t device, const char *name, uint32_t *qId) {
  return RT_ERROR_NONE;
}

rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr) {
  return 0;
}

rtError_t rtMemQueueCreate(int32_t device, const rtMemQueueAttr_t *queAttr, uint32_t *qid) {
  *qid = ge::mem_queues_[device].size();
  ge::mem_queues_[device][*qid] = std::queue<void *>{};
  return 0;
}

rtError_t rtMemQueueDestroy(int32_t device, uint32_t qid) {
  ge::mem_queues_[device].erase(qid);
  return 0;
}

rtError_t rtMemQueueEnQueueBuff(int32_t device, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout) {
  return 0;
}

rtError_t rtMemQueueDeQueueBuff(int32_t device, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) {
  return 0;
}

rtError_t rtMemQueueEnQueue(int32_t device, uint32_t qid, void *mbuf) {
  ge::mem_queues_[device][qid].push(mbuf);
  return 0;
}

rtError_t rtMemQueueDeQueue(int32_t device, uint32_t qid, void **mbuf) {
  if (ge::mem_queues_[device][qid].empty()) {
    return 1;
  }
  *mbuf = ge::mem_queues_[device][qid].back();
  ge::mem_queues_[device][qid].pop();
  return 0;
}

rtError_t rtMemQueuePeek(int32_t device, uint32_t qid, size_t *bufLen, int32_t timeout) {
  return 0;
}

rtError_t rtMbufInit(rtMemBuffCfg_t *cfg) {
  return 0;
}

rtError_t rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size) {
  *mbuf = new ge::MbufStub(size);
  return 0;
}

rtError_t rtMbufFree(rtMbufPtr_t mbuf) {
  delete reinterpret_cast<ge::MbufStub *>(mbuf);
  return 0;
}

rtError_t rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf) {
  *databuf = reinterpret_cast<ge::MbufStub *>(mbuf)->buffer;
  return 0;
}

rtError_t rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size) {
  *size = reinterpret_cast<ge::MbufStub *>(mbuf)->length;
  return 0;
}

rtError_t rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size) {
  static char priv_fake[1024] = {};
  *priv = priv_fake;
  *size = 512;
  return 0;
}

rtError_t rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeout) {
  return 0;
}

rtError_t rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack) {
  return 0;
}

rtError_t rtEschedSubmitEvent(int32_t devId, rtEschedEventSummary_t *event) {
  return 0;
}

rtError_t rtCmoTaskLaunch(rtCmoTaskInfo_t *taskInfo, rtStream_t stm, uint32_t flag) {
  return RT_ERROR_NONE;
}

rtError_t rtBarrierTaskLaunch(rtBarrierTaskInfo_t *taskInfo, rtStream_t stm, uint32_t flag) {
  return RT_ERROR_NONE;
}

rtError_t rtEschedAttachDevice(int32_t device) {
  return 0;
}

rtError_t rtEschedCreateGrp(int32_t devId, uint32_t grpId, rtGroupType_t type) {
  return 0;
}

rtError_t rtEschedWaitEvent(int32_t devId,
                            uint32_t grpId,
                            uint32_t threadId,
                            int32_t timeout,
                            rtEschedEventSummary_t *event) {
  return ge::RuntimeStub::GetInstance()->rtEschedWaitEvent(devId, grpId, threadId, timeout, event);
}

rtError_t rtEschedSubscribeEvent(int32_t devId,
                                 uint32_t grpId,
                                 uint32_t threadId,
                                 uint64_t eventBitmap) {
  return 0;
}

rtError_t rtQueueSubscribe(int32_t devId, uint32_t qid, uint32_t groupId, int32_t type) {
  return 0;
}

rtError_t rtQueueSubF2NFEvent(int32_t devId, uint32_t qid, uint32_t groupId) {
  return 0;
}

#ifdef __cplusplus
}
#endif
