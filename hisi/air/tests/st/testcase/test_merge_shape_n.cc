/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include "utils/graph_factory.h"
#include <gtest/gtest.h>
#include <map>
#include "external/ge/ge_api.h"
#include "utils/synchronizer.h"
#include "ge_running_env/tensor_utils.h"
#include "utils/tensor_adapter.h"
#include "graph/utils/graph_utils.h"
#include "mmpa/mmpa_api.h"
#include "init_ge.h"
#include "ge_graph_dsl/assert/graph_assert.h"

namespace ge {
class MergeShapeNSt : public testing::Test {
 protected:
  void SetUp() override {
    ReInitGe();
  }
};

TEST_F(MergeShapeNSt, merge_unknown_shapeN) {
  Graph graph = GraphFactory::BuildGraphForMergeShapeNPass();
  
  DUMP_GRAPH_WHEN("OptimizeStage1_1");
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);
  std::vector<InputTensorInfo> inputs;
  session.BuildGraph(1, inputs);

  CHECK_GRAPH(OptimizeStage1_1) {
    auto ret = graph->TopologicalSorting();
    EXPECT_EQ(ret, SUCCESS);
    int n = 0;
    for (const auto node : graph->GetDirectNode()) {
      if (node->GetType() != SHAPEN) {
        continue;
      }
      n++;
    }
    ASSERT_EQ(n, 1);
    ASSERT_EQ(graph->FindNode("shapeN1"), nullptr);
    ASSERT_EQ(graph->FindNode("shapeN2"), nullptr);
  };  
}
}  // namespace ge