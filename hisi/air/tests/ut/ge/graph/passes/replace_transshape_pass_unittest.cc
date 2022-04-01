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

// To test the ReplaceTransShapePass

#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/replace_transshape_pass.h"


using namespace testing;
using namespace ge;
namespace ge {
class UtestReplaceTransShapePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {
///
///         netoutput1
///          /  c   \
/// trans_shape - identity
///        |   \c  |
///      add1  add2
///        / \/ \
///       /  /\  \
///      var1  var2
static ComputeGraphPtr BuildGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("add2", ADD));
    CHAIN(NODE("var2", VARIABLE)->EDGE(0, 1)->NODE("add1", ADD));
    CHAIN(NODE("var2", VARIABLE)->EDGE(0, 1)->NODE("add2", ADD));

    CHAIN(NODE("add1", ADD)->EDGE(0, 0)->NODE("trans_shape", TRANSSHAPE));
    CHAIN(NODE("trans_shape", TRANSSHAPE)->EDGE(0, 0)->NODE("net_output", NETOUTPUT));
    CHAIN(NODE("add2", ADD)->EDGE(0, 0)->NODE("identity", IDENTITY));
    CHAIN(NODE("identity", IDENTITY)->EDGE(0, 1)->NODE("net_output", NETOUTPUT));

    CTRL_CHAIN(NODE("add2", ADD)->NODE("trans_shape", TRANSSHAPE));
    CTRL_CHAIN(NODE("trans_shape", TRANSSHAPE)->NODE("identity", IDENTITY));
  };
  return ToComputeGraph(g1);
}
} // namespace

TEST_F(UtestReplaceTransShapePass, test_normal_succ) {
  const auto graph = BuildGraph();
  ReplaceTransShapePass pass;
  auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  const auto mem_copy = graph->FindFirstNodeMatchType(MEMCPYASYNC);
  ASSERT_NE(mem_copy, nullptr);
  EXPECT_EQ(mem_copy->GetOutDataNodesSize(), 1);
  EXPECT_EQ(mem_copy->GetInDataNodes().size(), 1);
  EXPECT_EQ(mem_copy->GetOutControlNodes().size(), 1);
  EXPECT_EQ(mem_copy->GetInControlNodes().size(), 1);
  EXPECT_EQ(mem_copy->GetInDataNodes().at(0)->GetName(), "add1");
  const auto trans_shape = graph->FindFirstNodeMatchType(TRANSSHAPE);
  ASSERT_NE(trans_shape, nullptr);
  EXPECT_EQ(trans_shape->GetInAllNodes().size(), 0);
  EXPECT_EQ(trans_shape->GetOutAllNodes().size(), 0);
}
} // namespace ge