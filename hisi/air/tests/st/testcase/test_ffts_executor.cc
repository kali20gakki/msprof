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

#include "ge_graph_dsl/graph_dsl.h"

#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/tbe_kernel_handle.h"
#include "register/ffts_plus_update_manager.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"

using namespace std;
using namespace testing;

namespace ge {
class FftsTest : public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

/**********************************************************************************************************************
 *
 * Data           Data
 *   \             /
 *    \           /
 *     \         /                                       Data      Data
 *      \       /                                          \        /
 *   PartitionedCall                                        \      /
 *          |                                                Conv2D
 *          |                                                  |
 *          |                                                  |
 *          |                                                Relu
 *      NetOutput                                             |
 *                                                            |
 *                                                        NetOutput
 *
 *
 **********************************************************************************************************************/
static void BuildFftsGraph(ComputeGraphPtr &root_graph, ComputeGraphPtr &ffts_graph) {
  const auto SetOpDescParam = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    static uint32_t index = 0U;
    const static std::set<std::string> kGeLocalTypes{ DATA, CONSTANT, VARIABLE, NETOUTPUT, AIPP_DATA_TYPE };
    const static std::set<std::string> kTbeTypes{ CONV2D, RELU };

    GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
    TensorUtils::SetSize(tensor, 64);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      vector<int64_t> output_offset;
      for (size_t i = 0U; i < op_desc->GetOutputsSize(); ++i) {
        op_desc->UpdateOutputDesc(i, tensor);
        output_offset.emplace_back(index * 64);
        ++index;
      }
      op_desc->SetOutputOffset(output_offset);
      op_desc->SetWorkspace({});
      op_desc->SetWorkspaceBytes({});
      if (kTbeTypes.find(op_desc->GetType()) != kTbeTypes.end()) {
        int64_t run_mode = static_cast<int64_t>(domi::ImplyType::TVM);
        (void)AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, run_mode);
        std::vector<char> kernel_bin(64, '\0');
        TBEKernelPtr kernel_handle = std::make_shared<ge::OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
        op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, kernel_handle);
        AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName());
      }
    }

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      vector<int64_t> input_offset;
      for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
        op_desc->UpdateInputDesc(i, tensor);
        const auto in_anchor = node->GetInDataAnchor(i);
        if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr) {
          continue;
        }

        const auto out_anchor = in_anchor->GetPeerOutAnchor();
        const auto peer_node = out_anchor->GetOwnerNode();
        const vector<int64_t> output_offset = peer_node->GetOpDesc()->GetOutputOffset();
        if (static_cast<size_t>(out_anchor->GetIdx()) >= output_offset.size()) {
          continue;
        }

        input_offset.emplace_back(output_offset.at(out_anchor->GetIdx()));
      }
      op_desc->SetInputOffset(input_offset);
    }
  };

  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  SetOpDescParam(root_graph->GetDirectNode());
  const auto root_call_0 = root_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(root_call_0, nullptr);
  const auto node_output_0 = root_graph->FindNode("Node_Output");
  EXPECT_NE(node_output_0, nullptr);
  node_output_0->GetOpDesc()->SetSrcIndex({0});
  node_output_0->GetOpDesc()->SetSrcName({"partition_call"});

  auto sgt_graph_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto sgt_graph_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  auto sgt_graph_conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
      .Attr(ATTR_NAME_IMPLY_TYPE, 1)           // domi::ImplyType::TVM
      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  auto sgt_graph_relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIV")
      .Attr(ATTR_NAME_IMPLY_TYPE, 1)
      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt_graph/_arg_0", sgt_graph_data_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Conv2D", sgt_graph_conv_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Relu", sgt_graph_relu_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Node_Output", NETOUTPUT)
    );
    CHAIN(NODE("sgt_graph/_arg_1", sgt_graph_data_1)->EDGE(0, 1)
              ->NODE("sgt_graph/Conv2D", sgt_graph_conv_0)
    );
  };
  ffts_graph = ToComputeGraph(g2);
  SetOpDescParam(ffts_graph->GetDirectNode());
  ffts_graph->SetParentNode(root_call_0);
  ffts_graph->SetParentGraph(root_graph);
  root_call_0->GetOpDesc()->AddSubgraphName("ffts");
  root_call_0->GetOpDesc()->SetSubgraphInstanceName(0, ffts_graph->GetName());
  root_graph->AddSubGraph(ffts_graph);
  int64_t op_id = 0;
  for (auto &node : root_graph->GetAllNodes()) {
    node->GetOpDesc()->SetId(op_id);
    op_id++;
  }
}

static void InitFftsDescInfo(domi::FftsTaskDef *task_def) {
  domi::FftsDescInfoDef *desc_info_def = task_def->mutable_ffts_desc();
  desc_info_def->set_tm(1);
  desc_info_def->set_di(1);
  desc_info_def->set_dw(1);
  desc_info_def->set_df(1);
  desc_info_def->set_data_split_unit(1);
  desc_info_def->set_prefetch_ost_num(1);
  desc_info_def->set_cache_maintain_ost_num(1);
  desc_info_def->set_aic_prefetch_upper(1);
  desc_info_def->set_aic_prefetch_lower(1);
  desc_info_def->set_aiv_prefetch_upper(1);
  desc_info_def->set_aiv_prefetch_lower(1);
}

static void InitFftsSubTask(domi::FftsSubTaskDef *sub_task_def) {
  sub_task_def->set_sub_task_type(0);
  sub_task_def->set_thread_dim(1);
  sub_task_def->set_dst_tick_cache_vld_bitmap(1);
  sub_task_def->set_src_tick_cache_vld_bitmap(1);
  sub_task_def->set_src_data_out_of_subgraph_bitmap(1);
  sub_task_def->add_dst_tick_cache_id(0);
  sub_task_def->add_src_tick_cache_id(0);
  domi::AutoThreadAicAivDef *auto_thread_aic_aiv_def = sub_task_def->mutable_auto_thread_aic_aiv();
  // task_addr: {0, 200, 400, 800}
  // task_addr_offset: {20, 40, 60, 80}
  auto_thread_aic_aiv_def->add_task_addr(0);
  auto_thread_aic_aiv_def->add_task_addr(200);
  auto_thread_aic_aiv_def->add_task_addr(400);
  auto_thread_aic_aiv_def->add_task_addr(800);
  auto_thread_aic_aiv_def->add_task_addr_offset(20);
  auto_thread_aic_aiv_def->add_task_addr_offset(40);
  auto_thread_aic_aiv_def->add_task_addr_offset(60);
  auto_thread_aic_aiv_def->add_task_addr_offset(80);
  auto_thread_aic_aiv_def->set_task_param_offset(20);
  auto_thread_aic_aiv_def->set_sat_mode(0);
  auto_thread_aic_aiv_def->set_schedule_mode(0);
  auto_thread_aic_aiv_def->set_cache_prefetch_cnt(1);
  auto_thread_aic_aiv_def->set_prefetch_enable_bitmap(0);
  auto_thread_aic_aiv_def->set_prefetch_once_bitmap(0);
  auto_thread_aic_aiv_def->set_tail_blk_dim(1);
  auto_thread_aic_aiv_def->set_non_tail_blk_dim(1);
  auto_thread_aic_aiv_def->set_non_tail_task_func_stub("non_tail");
  auto_thread_aic_aiv_def->set_tail_task_func_stub("tail");
  auto_thread_aic_aiv_def->set_input_output_count(3);
  domi::AutoThreadPrefetchDef *prefetch_def = auto_thread_aic_aiv_def->add_src_prefetch();
  prefetch_def->set_data_addr(0);
  prefetch_def->set_data_addr_offset(20);
  prefetch_def->set_non_tail_data_len(15);
  prefetch_def->set_tail_data_len(1);
}

static void InitTicketCache(domi::TicketCacheDef *cache_def) {
  cache_def->set_cache_option(1);
  cache_def->set_ticket_cache_window(1);
  domi::AutoThreadCacheDef *auto_thread_cache_def = cache_def->mutable_auto_thread_cache();
  auto_thread_cache_def->set_data_addr(20);
  auto_thread_cache_def->set_data_addr_offset(20);
  auto_thread_cache_def->set_non_tail_data_len(5);
  auto_thread_cache_def->set_tail_data_len(5);
  auto_thread_cache_def->set_ticket_cache_ref_cnt(1);
}

TEST_F(FftsTest, ffts_graph) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_graph;
  BuildFftsGraph(root_graph, ffts_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> task = MakeShared<domi::ModelTaskDef>();
  domi::TaskDef *task_def = task->add_task();
  task_def->set_type(RT_MODEL_TASK_FFTS_TASK);
  domi::FftsTaskDef *ffts_task_def = task_def->mutable_ffts_task();
  ffts_task_def->set_op_index(ffts_graph->GetParentNode()->GetOpDesc()->GetId());
  ffts_task_def->set_ffts_type(2);
  ffts_task_def->set_addr_size(4);
  InitFftsDescInfo(ffts_task_def);
  domi::FftsSubTaskDef *sub_task_def = ffts_task_def->add_sub_task();
  InitFftsSubTask(sub_task_def);
  domi::TicketCacheDef *ticket_cache_def = ffts_task_def->add_ticket_cache();
  InitTicketCache(ticket_cache_def);

  // Build GeModel.
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(task);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);

  // Test for Load.
  model.Assign(ge_model);
  ASSERT_EQ(model.Init(), SUCCESS);
}
} // namespace ge
