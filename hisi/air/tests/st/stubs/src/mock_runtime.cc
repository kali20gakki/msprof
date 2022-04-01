/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include "utils/mock_runtime.h"
#include "securec.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "graph/def_types.h"
#include "cce/fwk_adpt_struct.h"

namespace ge {

rtError_t MockRtKernelLaunchEx(void *args, uint32_t args_size, uint32_t flags, rtStream_t stream_) {
  return RT_ERROR_NONE;
}

rtError_t MockRtKernelLaunchExForHostMemAicpuTfKernel(void *args, uint32_t args_size, uint32_t flags,
                                                      rtStream_t stream_) {
  EXPECT_GE(args_size, sizeof(aicpu::AicpuParamHead));
  return RT_ERROR_NONE;
}

int32_t CheckArgs(const rtArgsEx_t *const args) {
  if (args->hostInputInfoNum == 0U) {
    return 0;
  }
  EXPECT_NE(args->hostInputInfoPtr, nullptr);
  // check offset
  for (size_t i = 0; i < args->hostInputInfoNum; i++) {
    rtHostInputInfo_t &host_info = args->hostInputInfoPtr[i];
    uint64_t data_ptr = PtrToValue(args->args) + host_info.dataOffset;

    uint64_t *value_ptr = PtrToPtr<void, uint64_t>(ValueToPtr(data_ptr));
    EXPECT_EQ(*value_ptr, kHostMemInputValue);

    uint64_t *addr_ptr = reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(args->args) + host_info.addrOffset);
    EXPECT_EQ(*addr_ptr, PtrToValue(value_ptr));
  }
  return 0;
}

rtError_t MockRtKernelLaunchWithFlagForHostMem(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *args,
                                               rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flags) {
  // check offset
  EXPECT_EQ(CheckArgs(args), 0);
  return RT_ERROR_NONE;
}

rtError_t MockRtCpuKernelLaunchWithFlagForHostMem(const void *so_name, const void *kernel_name, uint32_t core_dim,
                                                  const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream_,
                                                  uint32_t flags) {
  // check offset
  EXPECT_EQ(CheckArgs(args), 0);
  return RT_ERROR_NONE;
}

rtError_t MockRtAicpuKernelLaunchWithFlagForHostMem(const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
                                                    const rtArgsEx_t *args, rtSmDesc_t *smDesc, rtStream_t stream,
                                                    uint32_t flags) {
  // check offset
  EXPECT_EQ(CheckArgs(args), 0);
  return RT_ERROR_NONE;
}

rtError_t MockRtMemcpy(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind) {
  if (count == 0) {
    return RT_ERROR_NONE;
  }
  if (count == sizeof(aicpu::FWKAdapter::ResultSummary) && kind == RT_MEMCPY_DEVICE_TO_HOST) {
    aicpu::FWKAdapter::ResultSummary summary{};
    summary.shape_data_size = 0;
    summary.raw_data_size = 0;
    return memcpy_s(dst, dest_max, &summary, count);
  } else {
    return memcpy_s(dst, dest_max, src, count);
  }
}

std::shared_ptr<MockRuntime> MockForKernelLaunchWithHostMemInput() {
  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchEx)
      .WillRepeatedly(testing::Invoke(MockRtKernelLaunchExForHostMemAicpuTfKernel));
  EXPECT_CALL(*runtime_stub, rtKernelLaunchWithFlag)
      .WillRepeatedly(testing::Invoke(MockRtKernelLaunchWithFlagForHostMem));
  EXPECT_CALL(*runtime_stub, rtCpuKernelLaunchWithFlag)
      .WillRepeatedly(testing::Invoke(MockRtCpuKernelLaunchWithFlagForHostMem));
  EXPECT_CALL(*runtime_stub, rtAicpuKernelLaunchWithFlag)
      .WillRepeatedly(testing::Invoke(MockRtAicpuKernelLaunchWithFlagForHostMem));

  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(MockRtMemcpy));
  return runtime_stub;
}
}