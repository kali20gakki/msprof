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

#include <gtest/gtest.h>
#define private public
#include "graph/load/model_manager/task_info/super_kernel/super_kernel.h"
#undef private
#include "graph/load/model_manager/task_info/super_kernel/super_kernel_factory.h"

using namespace std;

namespace ge {

class UtestSuperKernel : public testing::Test {
 protected:
  void SetUp() {
    skt::SuperKernelFactory::GetInstance().Init();
  }

  void TearDown() {}
};

TEST_F(UtestSuperKernel, launch) {

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);

  std::unique_ptr<skt::SuperKernel> super_kernel = nullptr;
  EXPECT_EQ(skt::SuperKernelFactory::GetInstance().FuseKernels(
    { stream, stream }, { stream, stream }, 32, super_kernel), SUCCESS);

  EXPECT_EQ(super_kernel->Launch(stream, 0), SUCCESS);

  EXPECT_EQ(super_kernel->Launch(stream, 0, "op_name"), SUCCESS);
}

}  // namespace ge
