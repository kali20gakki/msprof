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
#ifndef AIR_TESTS_ST_STUBS_UTILS_MOCK_RUNTIME_H_
#define AIR_TESTS_ST_STUBS_UTILS_MOCK_RUNTIME_H_

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "runtime/kernel.h"
#include "runtime/rt.h"
#include "ge_running_env/fake_ns.h"
#include "tests/depends/runtime/src/runtime_stub.h"

namespace ge {
class MockRuntime : public RuntimeStub {
 public:
  MOCK_METHOD4(rtKernelLaunchEx, int32_t(void *, uint32_t, uint32_t, rtStream_t));
  MOCK_METHOD6(rtKernelLaunchWithFlag, int32_t(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *arg_ex,
                                               rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flags));

  MOCK_METHOD7(rtCpuKernelLaunchWithFlag,
               int32_t(const void *so_name, const void *kernel_name, uint32_t core_dim, const rtArgsEx_t *args,
                       rtSmDesc_t *smDesc, rtStream_t stream_, uint32_t flags));
  MOCK_METHOD6(rtAicpuKernelLaunchWithFlag, int32_t(const rtKernelLaunchNames_t *, uint32_t,
                                                    const rtArgsEx_t *, rtSmDesc_t *, rtStream_t, uint32_t));

  MOCK_METHOD5(rtMemcpy, int32_t(void *, uint64_t, const void *, uint64_t, rtMemcpyKind_t));
};

std::shared_ptr<MockRuntime> MockForKernelLaunchWithHostMemInput();
rtError_t MockRtKernelLaunchEx(void *args, uint32_t args_size, uint32_t flags, rtStream_t stream_);
rtError_t MockRtMemcpy(void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind);
constexpr uint8_t kHostMemInputValue = 110U;
}
#endif  // AIR_TESTS_ST_STUBS_UTILS_MOCK_RUNTIME_H_
