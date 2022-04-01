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
#include <gtest/gtest.h>
#include <string>
#include <map>
#define private public
#define protected public
#include "graph/passes/inplace_support_check_pass.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/graph_utils.h"
#include "init/gelib.h"
#include "graph/node.h"
#undef private
#undef protected

using namespace ge;

class UtestGraphPassesInplaceSupportCheckPass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

static ComputeGraphPtr BuildGraphInplaceSupportCheckPass() {
  DEF_GRAPH(inplaceSupportCheckGraph) {
    CHAIN(NODE("cast0", CAST)->EDGE(0U, 0U)->NODE("cast1", CAST));
    CHAIN(NODE("cast1", CAST)->EDGE(0U, 0U)->NODE("translate1", TRANSLATE));
    CHAIN(NODE("translate1", TRANSLATE)->EDGE(0U, 0U)->NODE("cast2", CAST));
    CHAIN(NODE("translate1", TRANSLATE)->EDGE(1U, 0U)->NODE("cast3", CAST));
    CHAIN(NODE("cast4", CAST)->EDGE(0U, 0U)->NODE("cast2", CAST));
    CHAIN(NODE("cast2", CAST)->EDGE(0U, 0U)->NODE("cast5", CAST));
    CHAIN(NODE("cast4", CAST)->EDGE(1U, 0U)->NODE("cast5", CAST));
  };

  const auto graph = ToGeGraph(inplaceSupportCheckGraph);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  compute_graph->TopologicalSorting();

  return compute_graph;
}

TEST_F(UtestGraphPassesInplaceSupportCheckPass, cast_optimize) {
  auto graph = BuildGraphInplaceSupportCheckPass();
  ge::InplaceSupportCheckPass pass_;
  auto node_cast1 = graph->FindNode("cast1");

  Status status = pass_.Run(node_cast1);
  EXPECT_EQ(SUCCESS, status);

  // out_anchor_num > 1
  auto node_translate1 = graph->FindNode("translate1");
  status = pass_.Run(node_translate1);
  EXPECT_EQ(SUCCESS, status);

  auto node_cast2 = graph->FindNode("cast2");
  status = pass_.Run(node_cast2);
  EXPECT_EQ(SUCCESS, status);

  node_cast2->GetOpDesc()->UpdateInputDesc(0U, GeTensorDesc(GeShape({0,1}), FORMAT_NCHW, DT_INT8));
  status = pass_.Run(node_cast2);
  EXPECT_EQ(SUCCESS, status);

  node_cast2->GetOpDesc()->UpdateOutputDesc(0U, GeTensorDesc(GeShape(), FORMAT_NCHW, DT_INT8));
  status = pass_.Run(node_cast2);
  EXPECT_EQ(SUCCESS, status);
}

