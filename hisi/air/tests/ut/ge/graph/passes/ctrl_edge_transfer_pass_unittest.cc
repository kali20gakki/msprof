/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
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

#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/ctrl_edge_transfer_pass.h"

namespace ge {
class UtestCtrlEdgeTransferPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {
///     netoutput
///         |
///     identity2
///         |
///     const2   const3
///         |c    c/
///       identity1
///         |
///       const1
static ComputeGraphPtr BuildGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("const1", CONSTANT)->NODE("identity1", IDENTITY));
    CHAIN(NODE("const2", CONSTANT)->NODE("identity2", IDENTITY));
    CHAIN(NODE("identity2", IDENTITY)->NODE("net_output", NETOUTPUT));

    CTRL_CHAIN(NODE("identity1", IDENTITY)->NODE("const2", CONSTANT));
    CTRL_CHAIN(NODE("identity1", IDENTITY)->NODE("const3", CONSTANT));
  };
  return ToComputeGraph(g1);
}
}  // namespace

TEST_F(UtestCtrlEdgeTransferPass, test_normal_succ) {
  const auto graph = BuildGraph();
  CtrlEdgeTransferPass pass;
  Status ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  (void)AttrUtils::SetBool(graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, true);
  ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  const auto identity2 = graph->FindNode("identity2");
  ASSERT_NE(identity2, nullptr);
  EXPECT_EQ(identity2->GetInControlNodes().size(), 1);
  EXPECT_EQ(identity2->GetInControlNodes().at(0)->GetName(), "identity1");
}

}  // namespace ge
