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

#include <gtest/gtest.h>
#include <vector>

#include "runtime/rt.h"
#include "ge_graph_dsl/graph_dsl.h"
#define protected public
#define private public
#include "hybrid/model/graph_item.h"
#include "hybrid/model/node_item.h"
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "hybrid/executor/node_state.h"
#include "single_op/single_op.h"
#include "single_op/single_op_manager.h"
#include "single_op/task/build_task_utils.h"
#include "common/dump/dump_manager.h"
#include "common/utils/executor_utils.h"
#undef private
#undef protected

using namespace std;
using namespace ge;
using namespace hybrid;

namespace {
struct FakeGraphItem : GraphItem {
  FakeGraphItem(NodePtr node) {
    NodeItem::Create(node, node_item);
    node_item->num_inputs = 2;
    node_item->input_start = 0;
    node_item->output_start = 0;
    node_items_.emplace_back(node_item.get());
    total_inputs_ = 2;
    total_outputs_ = 1;
  }

  NodeItem *GetNodeItem() {
    return node_item.get();
  }

 private:
  std::unique_ptr<NodeItem> node_item;
};
}  // namespace
class UtestExecutorUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST(UtestExecutorUtils, test_if_has_host_mem_input) {
  DEF_GRAPH(single_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("data1");

    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("transdata1");

    CHAIN(NODE(op_ptr)->NODE(transdata1));
  };

  auto graph = ToComputeGraph(single_op);
  auto node = graph->FindNode("transdata1");
  for (size_t i = 0U; i < node->GetOpDesc()->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr &input_desc = node->GetOpDesc()->MutableInputDesc(static_cast<uint32_t>(i));
    if (input_desc == nullptr) {
      continue;
    }
    int64_t mem_type = 0;
    (void)ge::AttrUtils::SetInt(*input_desc, ge::ATTR_NAME_PLACEMENT, 2);
  }
  EXPECT_TRUE(ExecutorUtils::HasHostMemInput(node->GetOpDesc()));
}

TEST(UtestExecutorUtils, test_update_host_mem_input_args_util_for_hybrid) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc tensor_desc;
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);
  FrameState frame_state;
  FakeGraphItem graphItem(node);
  auto node_item = graphItem.GetNodeItem();
  SubgraphContext subgraph_context(nullptr, nullptr);
  uint8_t host_mem[8];
  TensorValue tensorValue(&host_mem, 8);
  subgraph_context.all_inputs_.emplace_back(tensorValue);
  subgraph_context.all_inputs_.emplace_back(tensorValue);
  NodeState node_state(*node_item, &subgraph_context, frame_state);
  TaskContext context(nullptr, &node_state, &subgraph_context);

  size_t io_addrs_size = 3 * sizeof(void *) + kMaxHostMemInputLen;
  uint64_t io_addrs[io_addrs_size];
  size_t host_mem_input_data_offset_in_args = 3 * sizeof(void *);

  vector<rtHostInputInfo_t> host_inputs;
  auto ret = ExecutorUtils::UpdateHostMemInputArgs(context, io_addrs, io_addrs_size,
                                                       host_mem_input_data_offset_in_args, host_inputs, false);
  EXPECT_EQ(ret, SUCCESS);
}
