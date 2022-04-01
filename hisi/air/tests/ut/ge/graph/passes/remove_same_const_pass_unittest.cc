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

// To test the RemoveSameConstPass

#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/remove_same_const_pass.h"


using namespace testing;
using namespace ge;
namespace ge {
class UtestRemoveSameConstPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {
///
///       netoutput1
///           |
///          mul
///         /   \
///      add1  add2
///        / \/ \
///       /  /\  \
///    const1 const2
///       c\   /c
///        no_op
static ComputeGraphPtr BuildGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("const1", CONSTANTOP)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("const1", CONSTANTOP)->EDGE(0, 0)->NODE("add2", ADD));
    CHAIN(NODE("const2", CONSTANTOP)->EDGE(0, 1)->NODE("add1", ADD));
    CHAIN(NODE("const2", CONSTANTOP)->EDGE(0, 1)->NODE("add2", ADD));

    CHAIN(NODE("add1", ADD)->EDGE(0, 0)->NODE("mul", MUL));
    CHAIN(NODE("add2", ADD)->EDGE(0, 1)->NODE("mul", MUL));
    CHAIN(NODE("mul", MUL)->EDGE(0, 0)->NODE("net_output", NETOUTPUT));

    CTRL_CHAIN(NODE("no_op", NOOP)->NODE("const1", CONSTANTOP));
    CTRL_CHAIN(NODE("no_op", NOOP)->NODE("const2", CONSTANTOP));
  };
  return ToComputeGraph(g1);
}
} // namespace

TEST_F(UtestRemoveSameConstPass, test_normal_succ) {
  const auto graph = BuildGraph();
  EXPECT_EQ(graph->GetDirectNodesSize(), 7);
  RemoveSameConstPass pass;
  auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 6);
  const auto add1 = graph->FindNode("add1");
  ASSERT_NE(add1, nullptr);
  ASSERT_EQ(add1->GetInDataNodes().size(), 2);
  EXPECT_EQ(add1->GetInDataNodes().at(0)->GetName(), add1->GetInDataNodes().at(1)->GetName());
  const auto add2 = graph->FindNode("add2");
  ASSERT_NE(add2, nullptr);
  ASSERT_EQ(add2->GetInDataNodes().size(), 2);
  ASSERT_EQ(add1->GetInDataNodes().at(0)->GetName(), add2->GetInDataNodes().at(1)->GetName());
}

TEST_F(UtestRemoveSameConstPass, test_unknown_const) {
  const auto graph = BuildGraph();
  EXPECT_EQ(graph->GetDirectNodesSize(), 7);
  const auto const1 = graph->FindNode("const1");
  ASSERT_NE(const1, nullptr);
  const1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({-1, 2}));
  const auto const2 = graph->FindNode("const2");
  ASSERT_NE(const2, nullptr);
  const2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({-1, 2}));
  RemoveSameConstPass pass;
  auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 7);
  const auto add1 = graph->FindNode("add1");
  ASSERT_NE(add1, nullptr);
  ASSERT_EQ(add1->GetInDataNodes().size(), 2);
  EXPECT_NE(add1->GetInDataNodes().at(0)->GetName(), add1->GetInDataNodes().at(1)->GetName());
}
} // namespace ge