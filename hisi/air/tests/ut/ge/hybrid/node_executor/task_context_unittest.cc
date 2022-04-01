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

#define private public
#define protected public
#include "framework/common/taskdown_common.h"
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "init/gelib.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
namespace ge {
using namespace hybrid;
class TaskContextTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "", int in_num = 0, int out_num = 0) {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  TensorUtils::SetSize(tensor, 64);
  vector<int64_t> input_offset;
  for (int i = 0; i < in_num; ++i) {
    op_desc->AddInputDesc(tensor);
    input_offset.emplace_back(index * 64 + i * 64);
  }
  op_desc->SetInputOffset(input_offset);

  vector<int64_t> output_offset;
  for (int i = 0; i < out_num; ++i) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(index * 64 + in_num * 64 + i * 64);
  }
  op_desc->SetOutputOffset(output_offset);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});

  ge::AttrUtils::SetStr(op_desc, ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  bool support_dynamic = true;
  ge::AttrUtils::GetBool(op_desc, "support_dynamicshape", support_dynamic);
  return op_desc;
}

TEST_F(TaskContextTest, test_attribute_of_context) {
  dlog_setlevel(0, 0, 0);
  OpDescPtr op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ge::AttrUtils::SetInt(*op_desc, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_SHAPE_RANGE);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);
  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  node_item->is_dynamic = true;
  node_item->shape_inference_type = DEPEND_SHAPE_RANGE;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  auto & task_context = *node_state->GetTaskContext();
  task_context.SetStatus(FAILED);
  ASSERT_EQ(task_context.status_, FAILED);
  task_context.stream_id_ = 10;
  ASSERT_EQ(task_context.GetStreamId(), 10);
  task_context.SetOverFlow(false);
  ASSERT_EQ(task_context.IsOverFlow(), false);
  ASSERT_EQ(task_context.IsForceInferShape(), task_context.force_infer_shape_);
  task_context.OnError(SUCCESS);
  ASSERT_EQ(task_context.execution_context_->status, SUCCESS);
  ASSERT_EQ(task_context.Synchronize(), SUCCESS);
  ASSERT_EQ(task_context.RegisterCallback(nullptr), SUCCESS);
  ASSERT_EQ(task_context.GetOutput(-1), nullptr);

  TensorValue tensor_value;
  ASSERT_EQ(task_context.SetOutput(-1, tensor_value), PARAM_INVALID);
  ASSERT_EQ(task_context.GetInput(-1), nullptr);
  ASSERT_EQ(task_context.MutableInput(-1), nullptr);
  ASSERT_EQ(task_context.MutableWorkspace(-1), nullptr);
  ASSERT_NE(task_context.node_item_->node, nullptr);
  ASSERT_NE(task_context.node_item_->node->GetOpDesc(), nullptr);
  task_context.node_item_->node->GetOpDesc()->SetWorkspaceBytes({0});
  task_context.execution_context_->allocator = NpuMemoryAllocator::GetAllocator();
  ASSERT_EQ(task_context.AllocateWorkspaces(), MEMALLOC_FAILED);

  dlog_setlevel(0, 3, 0);
}
}  // namespace ge