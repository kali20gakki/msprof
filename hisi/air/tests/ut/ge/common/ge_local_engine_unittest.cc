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
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "local_engine/engine/ge_local_engine.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
using namespace ge_local;

class UtestGeLocalEngine : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestGeLocalEngine, Normal) {
    auto &instance = GeLocalEngine::Instance();
}

TEST_F(UtestGeLocalEngine, Initialize) {
    auto &instance = GeLocalEngine::Instance();
    std::map<std::string, std::string> options;
    EXPECT_EQ(instance.Initialize(options), SUCCESS);
    EXPECT_EQ(Initialize(options), SUCCESS);
}

TEST_F(UtestGeLocalEngine, GetOpsKernelInfoStores) {
    auto &instance = GeLocalEngine::Instance();
    std::map<std::string, OpsKernelInfoStorePtr> ops_kernel_map;
    instance.GetOpsKernelInfoStores(ops_kernel_map);
    GetOpsKernelInfoStores(ops_kernel_map);
}

TEST_F(UtestGeLocalEngine, GetGraphOptimizerObjs) {
    auto &instance = GeLocalEngine::Instance();
    std::map<std::string, GraphOptimizerPtr> grpgh_opt_map;
    instance.GetGraphOptimizerObjs(grpgh_opt_map);
    GetGraphOptimizerObjs(grpgh_opt_map);
}

TEST_F(UtestGeLocalEngine, Finalize) {
    auto &instance = GeLocalEngine::Instance();
    EXPECT_EQ(instance.Finalize(), SUCCESS);
    EXPECT_EQ(Finalize(), SUCCESS);
}

} // namespace ge