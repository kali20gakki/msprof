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
#include "graph/passes/hccl_tailing_optimization_pass.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/graph_utils.h"
#include "init/gelib.h"
#undef private

using namespace ge;

class UtestGraphPassesHcclTailingOptimizationPass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

static ComputeGraphPtr BuildGraphHcclTailingPass() {
  DEF_GRAPH(castTranslateGraph) {
    CHAIN(NODE("allReduce", HCOMALLREDUCE)->EDGE(0U, 0U)->NODE("cast0", CAST));
    CHAIN(NODE("cast0", CAST)->EDGE(0U, 0U)->NODE("translate1", TRANSLATE));
    CHAIN(NODE("cast0", CAST)->EDGE(0U, 0U)->NODE("translate2", TRANSLATE));
    CHAIN(NODE("translate1", TRANSLATE)->EDGE(0U, 0U)->NODE("cast2", CAST));
  };

  const auto graph = ToGeGraph(castTranslateGraph);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  compute_graph->TopologicalSorting();
  return compute_graph;
}

TEST_F(UtestGraphPassesHcclTailingOptimizationPass, cast_optimize) {
  auto graph = BuildGraphHcclTailingPass();
  auto cast0 = graph->FindNode("cast0");
  auto translate1 = graph->FindNode("translate1");
  GraphUtils::AddEdge(cast0->GetOutControlAnchor(), translate1->GetInControlAnchor());
  ge::HcclTailingOptimizationPass pass_;
  Status status = pass_.Run(graph);
  EXPECT_EQ(SUCCESS, status);
}
