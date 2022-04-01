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
#include <string>
#include "common/ge_inner_error_codes.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/node_adapter.h"
#include "graph/passes/subexpression_migration_pass.h"

using namespace ge;

class UtestSubexpressionMigrationPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static void PrepareGraphForTest(const ComputeGraphPtr &graph) {
  const auto case_node = graph->FindNode("case");
  if (case_node != nullptr) {
    (void)AttrUtils::SetInt(case_node->GetOpDesc(), ATTR_NAME_BATCH_NUM, 2);
  }
  for (const auto subgraph : graph->GetAllSubgraphs()) {
    const auto add_node = subgraph->FindFirstNodeMatchType(ADD);
    if (add_node != nullptr) {
      add_node->SetHostNode(true);
    }
    const auto less_node = subgraph->FindFirstNodeMatchType(LESS);
    if (less_node != nullptr) {
      less_node->SetHostNode(true);
    }
  }
}

static ComputeGraphPtr BuildNormalGraph() {

  const auto sub1_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  const auto sub1_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  DEF_GRAPH(sub_1) {
    CHAIN(NODE("sub1_data_0", sub1_data_0)->NODE("add", ADD)->NODE("less", LESS));
    CHAIN(NODE("sub1_data_1", sub1_data_1)->NODE("add", ADD));
    CHAIN(NODE("sub1_const_0", CONSTANTOP)->NODE("less", LESS)->NODE("netoutput", NETOUTPUT));
  };

  const auto sub2_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  const auto sub2_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  DEF_GRAPH(sub_2) {
    CHAIN(NODE("sub2_data_0", sub2_data_0)->NODE("add", ADD)->NODE("less", LESS));
    CHAIN(NODE("sub2_data_1", sub2_data_1)->NODE("add", ADD));
    CHAIN(NODE("sub2_const_0", CONSTANTOP)->NODE("less", LESS)->NODE("netoutput", NETOUTPUT));
  };

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_0", DATA)->NODE("case", CASE, sub_1, sub_2)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("data_1", DATA)->NODE("case"));
  };

  sub_1.Layout();
  sub_2.Layout();
  const auto compute_graph = ToComputeGraph(g1);
  PrepareGraphForTest(compute_graph);
  return compute_graph;
}

static ComputeGraphPtr BuildGraphMultiMigration() {

  const auto sub1_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  const auto sub1_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  DEF_GRAPH(sub_1) {
    CHAIN(NODE("sub1_data_0", sub1_data_0)->NODE("sub1/add", ADD)->NODE("sub1/less", LESS));
    CHAIN(NODE("sub1_data_1", sub1_data_1)->NODE("sub1/add", ADD));
    CHAIN(NODE("sub1_data_1", sub1_data_1)->NODE("sub1/less", LESS)->NODE("sub1/netoutput", NETOUTPUT));
  };

  const auto sub2_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  const auto sub2_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  DEF_GRAPH(sub_2) {
    CHAIN(NODE("sub2_data_0", sub2_data_0)->NODE("sub2/add", ADD)->NODE("sub2/less", LESS));
    CHAIN(NODE("sub2_data_1", sub2_data_1)->NODE("sub2/add", ADD));
    CHAIN(NODE("sub2_data_1", sub2_data_1)->NODE("sub2/less", LESS)->NODE("sub2/netoutput", NETOUTPUT));
  };

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_0", DATA)->NODE("case", CASE, sub_1, sub_2)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("data_1", DATA)->NODE("case"));
  };

  sub_1.Layout();
  sub_2.Layout();
  const auto compute_graph = ToComputeGraph(g1);
  PrepareGraphForTest(compute_graph);
  return compute_graph;
}

static ComputeGraphPtr BuildGraphNoNeedMigration() {

  const auto sub1_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  DEF_GRAPH(sub_1) {
    CHAIN(NODE("sub1_data_0", sub1_data_0)->NODE("add", ADD)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("sub1_const_0", CONSTANTOP)->NODE("add", ADD));
  };

  const auto sub2_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  DEF_GRAPH(sub_2) {
    CHAIN(NODE("sub2_data_0", sub2_data_0)->NODE("add", ADD)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("sub2_const_0", CONSTANTOP)->NODE("add", ADD));
  };

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_0", DATA)->NODE("case", CASE, sub_1, sub_2)->NODE("netoutput", NETOUTPUT));
  };

  sub_1.Layout();
  sub_2.Layout();
  const auto compute_graph = ToComputeGraph(g1);
  PrepareGraphForTest(compute_graph);
  return compute_graph;
}

TEST_F(UtestSubexpressionMigrationPass, Run0) {
  Status retStatus;
  SubexpressionMigrationPass csubExpMigPass;

  //string name, string type, int in_cnt, int out_cnt, Format format,
  //DataType data_type, vector<int64_t> shape
  DEF_GRAPH(g1) {
    CHAIN(NODE("arg_0", DATA)->NODE("addNode", ADD)->NODE("output", NETOUTPUT));
    CHAIN(NODE("arg_1", DATA)->NODE("addNode"));
  };
  ComputeGraphPtr graph = ToComputeGraph(g1);

  retStatus = csubExpMigPass.Run(graph);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestSubexpressionMigrationPass, normal_migration_succ) {
  ComputeGraphPtr graph = BuildNormalGraph();
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);
  SubexpressionMigrationPass pass;
  const auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 5);
  const auto add_node = graph->FindFirstNodeMatchType(ADD);
  EXPECT_NE(add_node, nullptr);
  const auto case_node = graph->FindFirstNodeMatchType(CASE);
  ASSERT_NE(case_node, nullptr);
  EXPECT_EQ(case_node->GetAllInDataAnchorsSize(), 2);
}

TEST_F(UtestSubexpressionMigrationPass, multi_migration_succ) {
  ComputeGraphPtr graph = BuildGraphMultiMigration();
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);
  SubexpressionMigrationPass pass;
  const auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 6);
  const auto add_node = graph->FindFirstNodeMatchType(ADD);
  EXPECT_NE(add_node, nullptr);
  const auto case_node = graph->FindFirstNodeMatchType(CASE);
  ASSERT_NE(case_node, nullptr);
  const auto less_node = graph->FindFirstNodeMatchType(LESS);
  ASSERT_NE(less_node, nullptr);
  EXPECT_EQ(case_node->GetAllInDataAnchorsSize(), 2);
}

TEST_F(UtestSubexpressionMigrationPass, graph_no_need_migration) {
  ComputeGraphPtr graph = BuildGraphNoNeedMigration();
  EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  SubexpressionMigrationPass pass;
  const auto ret = pass.Run(graph);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  const auto add_node = graph->FindFirstNodeMatchType(ADD);
  EXPECT_EQ(add_node, nullptr);
  const auto case_node = graph->FindFirstNodeMatchType(CASE);
  ASSERT_NE(case_node, nullptr);
  EXPECT_EQ(case_node->GetAllInDataAnchorsSize(), 1);
}

