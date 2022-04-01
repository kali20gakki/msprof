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
#include "local_engine/ops_kernel_store/ge_local_ops_kernel_info.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
namespace ge_local{

class UtestGeLocalOpsKernelInfo : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestGeLocalOpsKernelInfo, Normal) {
  auto p = std::make_shared<GeLocalOpsKernelInfoStore>();
  const std::map<std::string, std::string> options = {};
  EXPECT_EQ(p->Initialize(options), SUCCESS);
  EXPECT_EQ(p->CreateSession(options), SUCCESS);
  EXPECT_EQ(p->DestroySession(options), SUCCESS);
  std::map<std::string, ge::OpInfo> infos;
  p->GetAllOpsKernelInfo(infos);
  const auto opdesc = std::make_shared<OpDesc>();
  OpDescPtr opdesc1 = nullptr;
  std::string reason = "test";
  EXPECT_EQ(p->CheckSupported(opdesc1, reason), false);
  EXPECT_EQ(p->CheckSupported(opdesc, reason), false);
  auto node = make_shared<ge::Node>();
  EXPECT_EQ(p->SetCutSupportedInfo(node), SUCCESS);
  EXPECT_EQ(p->Finalize(), SUCCESS);
}

}
} // namespace ge