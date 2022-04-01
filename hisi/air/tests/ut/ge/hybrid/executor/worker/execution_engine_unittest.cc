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
#include <gmock/gmock.h>
#include <vector>
#include "runtime/rt.h"

#define protected public
#define private public
#include "hybrid/model/hybrid_model.h"
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/executor/hybrid_execution_context.h"
#include "hybrid/executor/hybrid_model_executor.h"
#include "hybrid/executor/worker/execution_engine.h"
#include "hybrid/executor/subgraph_executor.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/profiling/profiling_properties.h"

using namespace std;
using namespace testing;
using namespace ge;
using namespace hybrid;


class UtestExecutionEngine : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {
  }
};
namespace {
const int kIntBase = 10;
}
namespace {
struct FakeGraphItem : GraphItem {
  FakeGraphItem(NodePtr node, UnknowShapeOpType op_type = DEPEND_COMPUTE) {
    NodeItem::Create(node, node_item);
    node_item->input_start = 0;
    node_item->output_start = 0;
    node_item->is_dynamic = true;
    node_item->shape_inference_type = op_type;
    node_item->is_output_shape_static = false;
    node_item->is_profiling_report = true;
    node_item->to_const_output_id_list.insert(0);
    node_items_.emplace_back(node_item.get());
    total_inputs_ = node->GetAllInAnchors().size();
    total_outputs_ = node->GetAllOutAnchors().size();
  }

  NodeItem *GetNodeItem() {
    return node_item.get();
  }

 private:
  std::unique_ptr<NodeItem> node_item;
};
}  // namespace

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "") {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);
  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({});
  op_desc->SetOutputOffset({});

  ge::AttrUtils::SetStr(op_desc, ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  bool support_dynamic = true;
  ge::AttrUtils::GetBool(op_desc, "support_dynamicshape", support_dynamic);
  return op_desc;
}

TEST_F(UtestExecutionEngine, ExecuteAsync_without_kernel_task) {
  auto graph = make_shared<ComputeGraph>("graph");
  OpDescPtr op_desc = CreateOpDesc("Add", "Add");
  GeShape shape({2, 16});
  GeTensorDesc tensor_desc(shape);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);
  GraphExecutionContext execution_context;
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_item_ = std::unique_ptr<GraphItem>(new(std::nothrow)GraphItem());
  execution_context.model = &hybrid_model;
  execution_context.profiling_level = 1;
  FakeGraphItem graph_item(node);
  SubgraphContext subgraph_context(&graph_item, &execution_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(graph_item.GetNodeItem());
  ASSERT_TRUE(node_state->GetTaskContext() != nullptr);

  std::function<void()> callback;
  SubgraphExecutor executor(hybrid_model.GetRootGraphItem(), &execution_context);
  executor.InitCallback(node_state, callback);
  ExecutionEngine execution_engine;
  ProfilingProperties::Instance().is_load_profiling_ = true;
  EXPECT_EQ(execution_engine.ExecuteAsync(*node_state, node_state->GetTaskContext(), execution_context, callback), INTERNAL_ERROR);
}

TEST_F(UtestExecutionEngine, ExecuteAsync_without_callback_and_kernel_task) {
  auto graph = make_shared<ComputeGraph>("graph");
  OpDescPtr op_desc = CreateOpDesc("Add", "Add");
  GeShape shape({2, 16});
  GeTensorDesc tensor_desc(shape);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);
  GraphExecutionContext execution_context;
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.root_graph_item_ = std::unique_ptr<GraphItem>(new(std::nothrow)GraphItem());
  execution_context.model = &hybrid_model;
  FakeGraphItem graph_item(node, DEPEND_SHAPE_RANGE);
  SubgraphContext subgraph_context(&graph_item, &execution_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  ProfilingProperties::Instance().is_load_profiling_ = true;
  auto node_state = subgraph_context.GetNodeState(graph_item.GetNodeItem());
  std::string task_type = "rts";
  uint32_t block_dim = 0;
  node_state->GetTaskContext()->SaveProfilingTaskDescInfo(task_type, block_dim, op_desc->GetType());

  ASSERT_TRUE(node_state->GetTaskContext() != nullptr);

  std::function<void()> callback;
  SubgraphExecutor executor(hybrid_model.GetRootGraphItem(), &execution_context);
  executor.InitCallback(node_state, callback);
  ExecutionEngine execution_engine;
  ModelManager::GetInstance().dump_exception_flag_ = true;
  EXPECT_EQ(execution_engine.ExecuteAsync(*node_state, node_state->GetTaskContext(), execution_context, callback),
            INTERNAL_ERROR);

  NodeDoneCallback node_done_callback(&execution_context, node_state->GetTaskContext());
  node_done_callback.context_->execution_context_->dump_properties.model_dump_properties_map_.clear();
  EXPECT_EQ(node_done_callback.DumpDynamicNode(), SUCCESS);
  EXPECT_EQ(node_done_callback.PrepareConstInputs(*graph_item.GetNodeItem()), INTERNAL_ERROR);
  std::vector<TaskDescInfo> task_desc_info;
  EXPECT_EQ(node_done_callback.GetTaskDescInfo(*node_state->GetTaskContext(), node, task_desc_info), SUCCESS);


  ShapeInferenceEngine shape_infer_engine(&execution_context, true);
  shape_infer_engine.Config(&subgraph_context);
  EXPECT_EQ(shape_infer_engine.DoInferShape(*node_state), GRAPH_FAILED);
  EXPECT_EQ(shape_infer_engine.PropagateOutputShapes(*node_state), SUCCESS);
  auto allocator = NpuMemoryAllocator::GetAllocator();
  auto tensor_buffer = TensorBuffer::Create(allocator, 100);
  auto tensor_value = TensorValue(shared_ptr<TensorBuffer>(tensor_buffer.release()));
  GeShape ge_shape({16, 16});
  GeTensorDesc desc(ge_shape);
  EXPECT_EQ(shape_infer_engine.UpdateShapeAndValue(graph_item.GetNodeItem(), &tensor_value, &desc), SUCCESS);
}
