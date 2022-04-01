/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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
#include <vector>

#define private public
#define protected public
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/node_executor/rts/rts_node_executor.h"
#include "common/model/ge_root_model.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "hybrid/node_executor/rts/rts_task_factory.h"
#include "depends/runtime/src/runtime_stub.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;

class UtestRtsNodeTask : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() { }
};

REGISTER_RTS_TASK_CREATOR(IDENTITY, IdentityNodeTask);
RtsTaskFactory::RtsTaskRegistrar test(IDENTITY, nullptr);

TEST_F(UtestRtsNodeTask, test_stream_switch_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "switch", STREAMSWITCH, 2, 0);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  uint64_t value_1 = 120;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  TensorValue in_tensor1(&value_1, sizeof(value_1));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);
  subgraph_context.SetInput(*node_item, 1, in_tensor1);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), INTERNAL_ERROR);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 10));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), INTERNAL_ERROR);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 0));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(node_state->GetSwitchIndex(), -1);
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
  ASSERT_EQ(node_state->GetSwitchIndex(), 0); // not equal, active 0

  uint64_t value_2 = 110;
  TensorValue in_tensor2(&value_2, sizeof(value_2));
  subgraph_context.SetInput(*node_item, 1, in_tensor2);
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
  ASSERT_EQ(node_state->GetSwitchIndex(), 1); // equal, active 1

  int64_t pred_value = 0;
  node_state->GetTaskContext()->MutableInputDesc(0)->SetDataType(DT_INT32);
  ASSERT_EQ(dynamic_cast<RtsNodeTask *>(task.get())->GetScalarIndexValue(*node_state->GetTaskContext(), 0, pred_value), SUCCESS);

  node_state->GetTaskContext()->MutableInputDesc(0)->SetDataType(DT_FLOAT);
  ASSERT_EQ(dynamic_cast<RtsNodeTask *>(task.get())->GetScalarIndexValue(*node_state->GetTaskContext(), 0, pred_value), UNSUPPORTED);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 2));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 3));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 4));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  ASSERT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, 5));
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);
}

TEST_F(UtestRtsNodeTask, test_stream_active_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "active", STREAMACTIVE, 0, 0);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(node_state->GetSwitchIndex(), -1);
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
  ASSERT_EQ(node_state->GetSwitchIndex(), -1);

  node_item->ctrl_send_.emplace(nullptr);
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
  ASSERT_EQ(node_state->GetSwitchIndex(), 0);
}

TEST_F(UtestRtsNodeTask, test_stream_merge_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "merge", STREAMMERGE, 2, 2);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);
  uint64_t value_1 = 220;
  TensorValue in_tensor1(&value_1, sizeof(value_1));
  subgraph_context.SetInput(*node_item, 1, in_tensor1);

  uint64_t value_2 = 123;
  TensorValue out_tensor0(&value_2, sizeof(value_2));
  subgraph_context.SetOutput(*node_item, 0, out_tensor0);
  uint64_t value_3 = 223;
  TensorValue out_tensor1(&value_3, sizeof(value_3));
  subgraph_context.SetOutput(*node_item, 1, out_tensor1);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  node_state->SetMergeIndex(1);
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
  ASSERT_EQ(node_state->GetSwitchIndex(), -1);

  uint64_t value_4 = 323;
  ASSERT_EQ(node_state->GetTaskContext()->GetOutput(0)->CopyScalarValueToHost(value_4), SUCCESS);
  ASSERT_EQ(value_4, value_1);

  uint64_t value_5 = 423;
  ASSERT_EQ(node_state->GetTaskContext()->GetOutput(1)->CopyScalarValueToHost(value_5), SUCCESS);
  ASSERT_EQ(value_5, 1);

  node_state->GetTaskContext()->GetNodeState()->merge_index_ = -1;
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), INTERNAL_ERROR);
}

TEST_F(UtestRtsNodeTask, test_memcpy_async_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "memcpy", MEMCPYASYNC, 1, 1);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);

  uint64_t value_1 = 123;
  TensorValue out_tensor0(&value_1, sizeof(value_1));
  subgraph_context.SetOutput(*node_item, 0, out_tensor0);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);

  uint64_t value_4 = 323;
  ASSERT_EQ(node_state->GetTaskContext()->GetOutput(0)->CopyScalarValueToHost(value_4), SUCCESS);
  ASSERT_EQ(value_4, value_0);
  ASSERT_EQ(value_1, value_0);
}

TEST_F(UtestRtsNodeTask, test_pass_through_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "enter", ENTER, 1, 1);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);

  uint64_t value_1 = 123;
  TensorValue out_tensor0(&value_1, sizeof(value_1));
  subgraph_context.SetOutput(*node_item, 0, out_tensor0);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);

  uint64_t value_4 = 323;
  ASSERT_EQ(node_state->GetTaskContext()->GetOutput(0)->CopyScalarValueToHost(value_4), SUCCESS);
  ASSERT_EQ(value_4, value_0);
}

TEST_F(UtestRtsNodeTask, test_unsupport_label_set) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "labelset", LABELSET, 0, 0);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 2;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), UNSUPPORTED);
}

TEST_F(UtestRtsNodeTask, test_unsupport_label_goto) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "labelgoto", LABELGOTO, 0, 0);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 2;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), UNSUPPORTED);
  ASSERT_EQ(task, nullptr);
}

TEST_F(UtestRtsNodeTask, test_unsupport_label_switch) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "labelswitch", LABELSWITCH, 0, 0);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 2;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), UNSUPPORTED);
  ASSERT_EQ(task, nullptr);
}

TEST_F(UtestRtsNodeTask, test_profiler_node_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_ = ge_root_model->GetRootGraph();
  NodePtr node = CreateNode(*graph, "switch", STREAMSWITCH, 2, 0);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  std::vector<domi::TaskDef> task_def = {*task};
  hybrid_model.task_defs_.insert(make_pair(node, task_def));
  ProfilingTraceNodeTask prof_task;
  ASSERT_EQ(prof_task.Init(hybrid_model, node), SUCCESS);

  hybrid_model.task_defs_.clear();
  ASSERT_EQ(prof_task.Init(hybrid_model, node), INTERNAL_ERROR);
}

TEST_F(UtestRtsNodeTask, profiler_node_task_execute) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_ = ge_root_model->GetRootGraph();
  NodePtr node = CreateNode(*graph, "switch", STREAMSWITCH, 2, 0);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  std::vector<domi::TaskDef> task_def = {*task};
  hybrid_model.task_defs_.insert(make_pair(node, task_def));
  ProfilingTraceNodeTask prof_task;
  ASSERT_EQ(prof_task.Init(hybrid_model, node), SUCCESS);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 2;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  uint64_t value_1 = 120;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  TensorValue in_tensor1(&value_1, sizeof(value_1));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);
  subgraph_context.SetInput(*node_item, 1, in_tensor1);

  std::function<void()> done = []() {};
  RTS_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t, RT_ERROR_NONE);
  ASSERT_EQ(prof_task.ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);

  prof_task.model_id_ = 99;
  RTS_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t, 0x07110001);
  ASSERT_NE(prof_task.ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
}

TEST_F(UtestRtsNodeTask, test_start_node_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_ = ge_root_model->GetRootGraph();
  NodePtr node = CreateNode(*graph, "StartOfSequence", STARTOFSEQUENCE, 0, 1);

  StartOfSequenceTask start_task;
  ASSERT_EQ(start_task.Init(hybrid_model, node), SUCCESS);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  std::function<void()> done = []() {};
  RTS_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t, RT_ERROR_NONE);
  ASSERT_EQ(start_task.ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);

  start_task.model_id_ = 99;
  RTS_STUB_RETURN_VALUE(rtProfilerTraceEx, rtError_t, 0x07110001);
  ASSERT_NE(start_task.ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
}

TEST_F(UtestRtsNodeTask, test_identiity_task) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "identity", IDENTITY, 1, 1);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);

  uint64_t value_1 = 123;
  TensorValue out_tensor0(&value_1, sizeof(value_1));
  subgraph_context.SetOutput(*node_item, 0, out_tensor0);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_EQ(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
  ASSERT_NE(task, nullptr);

  std::function<void()> done = []() {};
  ASSERT_EQ(task->ExecuteAsync(*node_state->GetTaskContext(), done), SUCCESS);
}

TEST_F(UtestRtsNodeTask, test_loadtask_fail) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeModelPtr ge_sub_model = std::make_shared<GeModel>();
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);

  NodePtr node = CreateNode(*graph, "matmul", MATMUL, 1, 1);

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[node] = std::move(new_node);
  node_item->input_start = 0;
  node_item->output_start = 0;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);

  uint64_t value_0 = 110;
  TensorValue in_tensor0(&value_0, sizeof(value_0));
  subgraph_context.SetInput(*node_item, 0, in_tensor0);

  uint64_t value_1 = 123;
  TensorValue out_tensor0(&value_1, sizeof(value_1));
  subgraph_context.SetOutput(*node_item, 0, out_tensor0);

  NodeTaskPtr task = nullptr;
  RtsNodeExecutor node_executor;
  ASSERT_NE(node_executor.LoadTask(hybrid_model, node, task), SUCCESS);
}
} // namespace ge