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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#define protected public
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/executor/subgraph_executor.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_node_task.h"
#include "common/model/ge_root_model.h"
#include "graph/utils/graph_utils.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;
class UtestFftsPlusNodeTask : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestFftsPlusNodeTask, test_callback) {
  GraphItem graph_item;
  FftsPlusNodeTask node_task(&graph_item);
  node_task.ext_args_.emplace_back(nullptr);

  ASSERT_EQ(node_task.Callback(nullptr), SUCCESS);

  const auto callback = []() {};
  ASSERT_EQ(node_task.Callback(callback), SUCCESS);
}

TEST_F(UtestFftsPlusNodeTask, test_failed_branch) {
  ComputeGraphPtr sgtgraph = MakeShared<ComputeGraph>("sgt_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(sgtgraph);
  HybridModel hybrid_model(ge_root_model);
  const NodePtr node = CreateNode(*sgtgraph, "unknown_1", PARTITIONEDCALL, 2, 1);
  const ComputeGraphPtr subgraph = MakeShared<ComputeGraph>("ffts_plus_graph");

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  ASSERT_NE(node_state->GetTaskContext(), nullptr);
  auto &task_context = *node_state->GetTaskContext();

  // GetTaskDefs is null.
  FftsPlusNodeTask node_task(&graph_item);
  ASSERT_EQ(node_task.Load(hybrid_model, node, subgraph), INTERNAL_ERROR);

  // SUCCESS for empty function.
  ASSERT_EQ(node_task.UpdateArgs(task_context), SUCCESS);

  // FFTSGraphPreThread failed.
  ASSERT_EQ(node_task.ExecuteAsync(task_context, nullptr), INTERNAL_ERROR);
}
} // namespace ge
