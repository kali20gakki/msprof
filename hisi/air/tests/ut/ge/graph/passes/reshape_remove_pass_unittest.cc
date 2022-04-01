/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "graph/passes/reshape_remove_pass.h"

#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"

namespace ge {
class UtestReshapeRemovePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};


TEST_F(UtestReshapeRemovePass, reshape_remove_succ) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("var2", VARIABLE)->EDGE(0, 1)->NODE("add1", ADD));

    CHAIN(NODE("add1", ADD)->EDGE(0, 0)->NODE("reshape", RESHAPE));
    CHAIN(NODE("const1", CONSTANT)->EDGE(0, 1)->NODE("reshape", RESHAPE));
    CHAIN(NODE("reshape", RESHAPE)->EDGE(0, 0)->NODE("net_output", NETOUTPUT));
  };
  auto compute_graph = ToComputeGraph(g1);
  EXPECT_NE(compute_graph->FindNode("reshape"), nullptr);
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ReshapeRemovePass});
  GEPass pass(compute_graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(compute_graph->FindNode("reshape"), nullptr);
}

TEST_F(UtestReshapeRemovePass, unknown_shape_reshape_not_remove) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("var2", VARIABLE)->EDGE(0, 1)->NODE("add1", ADD));

    CHAIN(NODE("add1", ADD)->EDGE(0, 0)->NODE("reshape", RESHAPE));
    CHAIN(NODE("const1", CONSTANT)->EDGE(0, 1)->NODE("reshape", RESHAPE));
    CHAIN(NODE("reshape", RESHAPE)->EDGE(0, 0)->NODE("net_output", NETOUTPUT));
  };
  auto compute_graph = ToComputeGraph(g1);
  const auto reshape_node = compute_graph->FindNode("reshape");
  ASSERT_NE(reshape_node, nullptr);
  reshape_node->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape({-1, 2, 2}));
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ReshapeRemovePass});
  GEPass pass(compute_graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_NE(compute_graph->FindNode("reshape"), nullptr);
}

TEST_F(UtestReshapeRemovePass, reformat_remove_succ) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("var2", VARIABLE)->EDGE(0, 1)->NODE("add1", ADD));

    CHAIN(NODE("add1", ADD)->EDGE(0, 0)->NODE("reformat", REFORMAT));
    CHAIN(NODE("const1", CONSTANT)->EDGE(0, 1)->NODE("reformat", REFORMAT));
    CHAIN(NODE("reformat", REFORMAT)->EDGE(0, 0)->NODE("net_output", NETOUTPUT));
  };
  auto compute_graph = ToComputeGraph(g1);
  EXPECT_NE(compute_graph->FindNode("reformat"), nullptr);
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ReshapeRemovePass});
  GEPass pass(compute_graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(compute_graph->FindNode("reformat"), nullptr);
}
}  // namespace ge
