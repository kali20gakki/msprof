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
#include "local_engine/ops_kernel_store/op/no_op.h"
#include "local_engine/ops_kernel_store/ge_local_ops_kernel_builder.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
namespace ge_local{

class UtestNoOp : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestNoOp, Normal) {
  RunContext runContext;
  OpDescPtr opdesc = std::make_shared<OpDesc>("test", "test");
  ComputeGraphPtr graph;
  const NodePtr node = std::make_shared<Node>(opdesc, graph);
  std::shared_ptr<NoOp> op = std::make_shared<NoOp>(*node, runContext);
  EXPECT_EQ(op->Run(), SUCCESS);
}

}
} // namespace ge