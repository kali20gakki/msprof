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

// To test the engine reassign processing of dynamic data flow ops

#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "framework/common/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/partition/optimizer/dynamic_data_flow_engine_reassign_pass.h"

namespace ge {
class UtestDynamicDataFlowEngineReassign : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static ComputeGraphPtr BuildGraph() {
  int32_t max_size = 100;
  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor =
      std::make_shared<GeTensor>(tensor_desc, reinterpret_cast<uint8_t *>(&max_size), sizeof(int32_t));

  const auto const_op = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor);
  const auto stack = OP_CFG(STACK).InCnt(1).OutCnt(1).Attr(ATTR_NAME_IS_UNKNOWN_SHAPE, true);
  const auto stack_push = OP_CFG(STACKPUSH).InCnt(2).OutCnt(1).Attr(ATTR_NAME_IS_UNKNOWN_SHAPE, true);
  const auto stack_pop = OP_CFG(STACKPOP).InCnt(1).OutCnt(1).Attr(ATTR_NAME_IS_UNKNOWN_SHAPE, true);

  DEF_GRAPH(g1) {
    CHAIN(NODE("const", const_op)->EDGE(0, 0)->NODE("stack", stack));
    CHAIN(NODE("stack", stack)->EDGE(0, 0)->NODE("identity", REFIDENTITY));

    CHAIN(NODE("identity", REFIDENTITY)->EDGE(0, 0)->NODE("stackpush", stack_push));
    CHAIN(NODE("const", const_op)->EDGE(0, 1)->NODE("stackpush", stack_push));

    CHAIN(NODE("identity", REFIDENTITY)->EDGE(0, 0)->NODE("stackpop", stack_pop));
    CHAIN(NODE("stackpop", stack_pop)->EDGE(0, 0)->NODE("add", ADD));
    CHAIN(NODE("const", const_op)->EDGE(0, 1)->NODE("add", ADD));
  };

  const auto graph = ToGeGraph(g1);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  (void)compute_graph->TopologicalSorting();
  return compute_graph;
}

TEST_F(UtestDynamicDataFlowEngineReassign, reassign_succ) {
  const auto graph = BuildGraph();
  EnginePlacer placer(graph);
  const Status ret = placer.ReAssignEngine();
  EXPECT_EQ(ret, SUCCESS);

  const auto stack = graph->FindNode("stack");
  ASSERT_NE(stack, nullptr);
  const auto stack_push = graph->FindNode("stackpush");
  ASSERT_NE(stack_push, nullptr);
  const auto stack_pop = graph->FindNode("stackpop");
  ASSERT_NE(stack_pop, nullptr);
  const auto identity = graph->FindNode("identity");
  ASSERT_NE(identity, nullptr);

  static const char_t *const kGeLocalEngineName = "DNN_VM_GE_LOCAL";
  static const char_t *const kGeLocalOpKernelLibName = "DNN_VM_GE_LOCAL_OP_STORE";

  EXPECT_EQ(stack->GetOpDesc()->GetOpEngineName(), kGeLocalEngineName);
  EXPECT_EQ(stack->GetOpDesc()->GetOpKernelLibName(), kGeLocalOpKernelLibName);
  EXPECT_EQ(stack_push->GetOpDesc()->GetOpEngineName(), kGeLocalEngineName);
  EXPECT_EQ(stack_push->GetOpDesc()->GetOpKernelLibName(), kGeLocalOpKernelLibName);
  EXPECT_EQ(stack_pop->GetOpDesc()->GetOpEngineName(), kGeLocalEngineName);
  EXPECT_EQ(stack_pop->GetOpDesc()->GetOpKernelLibName(), kGeLocalOpKernelLibName);
  EXPECT_EQ(identity->GetOpDesc()->GetOpEngineName(), "");
  EXPECT_EQ(identity->GetOpDesc()->GetOpKernelLibName(), "");

  const NodeEngineMap &node_atomic_engine_map = placer.GetNodeEngineMap(false);
  EXPECT_EQ(node_atomic_engine_map.size(), 3);
  EXPECT_EQ(node_atomic_engine_map.at(stack), kGeLocalEngineName);
  EXPECT_EQ(node_atomic_engine_map.at(stack_push), kGeLocalEngineName);
  EXPECT_EQ(node_atomic_engine_map.at(stack_pop), kGeLocalEngineName);
}
}  // namespace ge
