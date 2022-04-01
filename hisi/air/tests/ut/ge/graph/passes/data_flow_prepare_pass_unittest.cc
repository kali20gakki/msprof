/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

// To test the preparation of data flow ops

#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/graph_utils.h"
#include "graph/passes/data_flow_prepare_pass.h"

namespace ge {
class UtestDataFlowPreparePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static ComputeGraphPtr BuildGraph() {
  int32_t max_size1 = 100;
  GeTensorDesc tensor_desc1(GeShape(), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor1 =
      std::make_shared<GeTensor>(tensor_desc1, reinterpret_cast<uint8_t *>(&max_size1), sizeof(int32_t));
  const auto const1 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor1);

  int32_t max_size2 = 200;
  GeTensorDesc tensor_desc2(GeShape(), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor2 =
      std::make_shared<GeTensor>(tensor_desc2, reinterpret_cast<uint8_t *>(&max_size2), sizeof(int32_t));
  const auto const2 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor2);

  DEF_GRAPH(g1) {
    // stack1
    CHAIN(NODE("const1", const1)->EDGE(0, 0)->NODE("stack1", STACK));
    CHAIN(NODE("stack1", STACK)->EDGE(0, 0)->NODE("identity1", REFIDENTITY));

    CHAIN(NODE("identity1", REFIDENTITY)->EDGE(0, 0)->NODE("stackpush1", STACKPUSH));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("stackpush1", STACKPUSH));

    CHAIN(NODE("identity1", REFIDENTITY)->EDGE(0, 0)->NODE("stackpop1", STACKPOP));
    CHAIN(NODE("stackpop1", STACKPOP)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("add1", ADD));

    CTRL_CHAIN(NODE("stackpop1", STACKPOP)->NODE("stack2", STACK));

    // stack2
    CHAIN(NODE("const2", const2)->EDGE(0, 0)->NODE("stack2", STACK));

    CHAIN(NODE("stack2", STACK)->EDGE(0, 0)->NODE("stackpush2", STACKPUSH));
    CHAIN(NODE("add1", ADD)->EDGE(0, 1)->NODE("stackpush2", STACKPUSH));

    CHAIN(NODE("stack2", STACK)->EDGE(0, 0)->NODE("stackpop2", STACKPOP));
    CHAIN(NODE("stackpop2", STACKPOP)->EDGE(0, 0)->NODE("identity2", IDENTITY));
  };

  const auto graph = ToGeGraph(g1);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  compute_graph->TopologicalSorting();
  const map<string, uint32_t> name_index = {{"max_size", 0}};
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (node->GetType() == STACK && node->GetOpDesc() != nullptr) {
      node->GetOpDesc()->UpdateInputName(name_index);
    }
  }
  return compute_graph;
}

TEST_F(UtestDataFlowPreparePass, set_handle_succ) {
  const auto graph = BuildGraph();

  DataFlowPreparePass pass;
  const Status ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);

  // stack1
  const auto stack1 = graph->FindNode("stack1");
  ASSERT_NE(stack1, nullptr);
  const auto stack_push1 = graph->FindNode("stackpush1");
  ASSERT_NE(stack_push1, nullptr);
  const auto stack_pop1 = graph->FindNode("stackpop1");
  ASSERT_NE(stack_pop1, nullptr);

  const int64_t kStack1Handle = 0;
  int64_t handle1 = -1;
  (void)AttrUtils::GetInt(stack1->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle1);
  EXPECT_EQ(handle1, kStack1Handle);
  handle1 = -1;
  (void)AttrUtils::GetInt(stack_push1->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle1);
  EXPECT_EQ(handle1, kStack1Handle);
  handle1 = -1;
  (void)AttrUtils::GetInt(stack_pop1->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle1);
  EXPECT_EQ(handle1, kStack1Handle);
  int64_t max_size1 = -1;
  (void)AttrUtils::GetInt(stack1->GetOpDesc(), ATTR_NAME_DATA_FLOW_MAX_SIZE, max_size1);
  EXPECT_EQ(max_size1, 100);

  // stack2
  const auto stack2 = graph->FindNode("stack2");
  ASSERT_NE(stack2, nullptr);
  const auto stack_push2 = graph->FindNode("stackpush2");
  ASSERT_NE(stack_push2, nullptr);
  const auto stack_pop2 = graph->FindNode("stackpop2");
  ASSERT_NE(stack_pop2, nullptr);

  const int64_t kStack2Handle = 1;
  int64_t handle2 = -1;
  (void)AttrUtils::GetInt(stack2->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle2);
  EXPECT_EQ(handle2, kStack2Handle);
  handle1 = -1;
  (void)AttrUtils::GetInt(stack_push2->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle2);
  EXPECT_EQ(handle2, kStack2Handle);
  handle1 = -1;
  (void)AttrUtils::GetInt(stack_pop2->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle2);
  EXPECT_EQ(handle2, kStack2Handle);
  int64_t max_size2 = -1;
  (void)AttrUtils::GetInt(stack2->GetOpDesc(), ATTR_NAME_DATA_FLOW_MAX_SIZE, max_size2);
  EXPECT_EQ(max_size2, 200);
}
}  // namespace ge
