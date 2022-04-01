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
#include "hybrid/executor/hybrid_model_executor.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;

class UtestHybridModelExecutor : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() { }
};

TEST_F(UtestHybridModelExecutor, Test_execute_for_singleop) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("data", DATA);
  GeTensorDescPtr tensor_desc = make_shared<GeTensorDesc>(GeTensorDesc());
  op_desc->AddInputDesc(*tensor_desc);
  op_desc->AddOutputDesc(*tensor_desc);

  NodePtr node = graph->AddNode(op_desc);
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_item_.reset(new GraphItem);
  hybrid_model.has_observer_ = true;

  HybridModelExecutor executor(&hybrid_model, 0, nullptr);
  ASSERT_EQ(executor.Init(), SUCCESS);
  HybridModelExecutor::ExecuteArgs args;
  args.input_desc.push_back(tensor_desc);
  TensorValue tensor;
  args.inputs.push_back(tensor);
  ASSERT_EQ(executor.ExecuteForSingleOp(args), SUCCESS);
  ASSERT_EQ(executor.Cleanup(), SUCCESS);
}
} // namespace ge