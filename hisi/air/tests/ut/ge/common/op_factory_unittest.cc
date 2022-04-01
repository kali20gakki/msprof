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
#include "local_engine/ops_kernel_store/op/op_factory.h"
#include "local_engine/ops_kernel_store/op/no_op.h"
#include "local_engine/ops_kernel_store/ge_local_ops_kernel_builder.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
namespace ge_local{

class UtestOpFactory : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

std::shared_ptr<Op> testFunc(const Node &node, RunContext &runcontext)
{
  auto op = std::make_shared<NoOp>(node, runcontext);
  return op;
}

TEST_F(UtestOpFactory, Abnormal) {
  auto p = std::make_shared<OpFactory>();
  const OP_CREATOR_FUNC func1 = nullptr;
  const std::string type1 = "test";
  p->RegisterCreator(type1, func1);
  OP_CREATOR_FUNC func2 = testFunc;
  const std::string type2 = "Reshape";
  p->RegisterCreator(type2, func2);
  p->RegisterCreator(type2, func2);
}

}
} // namespace ge