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

#include <gtest/gtest.h>

#define private public
#define protected public
#include "compiler/opskernel_manager/ops_kernel_manager.h"
#include "ge_running_env/ge_running_env_faker.h"

namespace ge {
class OpsKernelManagerSt : public testing::Test {};

TEST_F(OpsKernelManagerSt, register_test_ok) {
  GeRunningEnvFaker ge_env;
  ge_env.InstallDefault();
  OpsKernelManager::GetInstance().InitGraphOptimizerPriority();
}
}