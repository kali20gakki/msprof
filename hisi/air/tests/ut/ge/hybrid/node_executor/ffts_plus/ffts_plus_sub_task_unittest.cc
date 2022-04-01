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
#include "ge_graph_dsl/graph_dsl.h"

#define private public
#define protected public
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/model/hybrid_model_builder.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task.h"
#include "common/model/ge_root_model.h"
#include "common/tbe_handle_store/tbe_handle_store.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "register/op_tiling_registry.h"

using namespace std;
using namespace testing;
using namespace optiling;

namespace ge {
using namespace hybrid;

class FFTSPlusTaskUpdateMix : public FFTSPlusTaskUpdate {
 public:
  Status GetAutoThreadParam(const NodePtr &node, const std::vector<optiling::utils::OpRunInfo> &op_run_info,
                            AutoThreadParam &auto_thread_param) {
    auto_thread_param.thread_dim = 32U;
    auto_thread_param.input_output_num = node->GetAllInDataAnchorsSize() + node->GetAllOutDataAnchorsSize();

    std::vector<int64_t> workspaces = node->GetOpDesc()->GetWorkspaceBytes();
    auto_thread_param.input_output_num += workspaces.size();

    auto_thread_param.task_addr_offset.resize(auto_thread_param.input_output_num, 32);

    return SUCCESS;
  }
};

class FFTSPlusTaskInvalid : public FFTSPlusTaskUpdate {
 public:
  Status GetAutoThreadParam(const NodePtr &node, const std::vector<optiling::utils::OpRunInfo> &op_run_info,
                            AutoThreadParam &auto_thread_param) {
    auto_thread_param.thread_dim = 0U;
    return SUCCESS;
  }
};

class UtestFftsPlusSubTask : public testing::Test {
 protected:
  void SetUp() {
    // Register from FE, set stub here.
    FftsPlusUpdateManager::FftsPlusUpdateRegistrar aic_001("AIC", [](){ return MakeShared<FFTSPlusTaskUpdateMix>(); });
    FftsPlusUpdateManager::FftsPlusUpdateRegistrar mix_001("MIX_AIV", [](){ return MakeShared<FFTSPlusTaskUpdateMix>(); });
    FftsPlusUpdateManager::FftsPlusUpdateRegistrar cpu_001("AICPU", [](){ return MakeShared<FFTSPlusTaskUpdateMix>(); });

    const auto op_tiling_func_aic = [](const Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
      static uint32_t tiling_key = 0;
      op_run_info.SetTilingKey(tiling_key % 2U);
      tiling_key++;

      const std::vector<char> tiling_data(126, 0);
      op_run_info.AddTilingData(tiling_data.data(), tiling_data.size());
      op_run_info.SetWorkspaces({100, 100});
      return true;
    };
    REGISTER_OP_TILING_UNIQ_V2(Conv2D, op_tiling_func_aic, 201);
    OpTilingRegistryInterf_V2(CONV2D, op_tiling_func_aic);

    const auto op_tiling_func_mix = [](const Operator &op, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
      static uint32_t tiling_key = 0;
      op_run_info.SetTilingKey(tiling_key % 4U);
      tiling_key++;

      op_run_info.SetWorkspaces({100, 128, 96});
      return true;
    };
    REGISTER_OP_TILING_UNIQ_V2(ReLU, op_tiling_func_mix, 201);
    optiling::OpTilingRegistryInterf_V2(RELU, op_tiling_func_mix);
  }
  void TearDown() {
    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(CONV2D);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(CONV2D);
    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(RELU);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(RELU);

    TBEHandleStore::GetInstance().kernels_.clear();
    FftsPlusUpdateManager::Instance().creators_.clear();
  }
};

TEST_F(UtestFftsPlusSubTask, ffts_plus_aic_task) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  ComputeGraphPtr sgt_graph = ToComputeGraph(g1);
  const auto sgt_node = sgt_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(sgt_node, nullptr);

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/_arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt/_arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr sub_graph = ToComputeGraph(g2);
  const auto sub_node = sub_graph->FindNode("sgt/conv2d");
  EXPECT_NE(sub_node, nullptr);

  SetPartitionedCall(sgt_node, sub_graph);
  AttrUtils::SetBool(sgt_node->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);
  AttrUtils::SetStr(sub_node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC");
  SetNodeAnchorStatus(sub_node);
  InitFftsThreadSliceMap(sub_node->GetOpDesc());

  //////////////////////////////////////////////////////////////////////////////
  // Build FftsPlusTaskDef.
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(sgt_node->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ctx_def->set_op_index(sub_node->GetOpDesc()->GetId());
  ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
  domi::FftsPlusAicAivCtxDef *aic_aiv_def = ctx_def->mutable_aic_aiv_ctx();
  InitAicAivCtx(aic_aiv_def, false);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(sgt_graph);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.task_defs_[sgt_node].emplace_back(task_def);

  auto unique_graph_item = MakeUnique<GraphItem>();
  auto graph_item = unique_graph_item.get();
  graph_item->total_inputs_ = 2;
  graph_item->total_outputs_ = 1;
  graph_item->SetName(sub_graph->GetName());
  hybrid_model.subgraph_items_.emplace(sub_graph->GetName(), std::move(unique_graph_item));

  {
    std::unique_ptr<NodeItem> new_node;
    ASSERT_EQ(NodeItem::Create(sgt_node, new_node), SUCCESS);
    NodeItem *node_item = new_node.get();
    hybrid_model.node_items_[sgt_node] = std::move(new_node);
    node_item->input_start = 0;
    node_item->output_start = 0;
    ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
    ASSERT_NE(node_item->node_executor, nullptr);
    ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sgt_node, node_item->kernel_task), SUCCESS);
  }

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(sub_node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[sub_node] = std::move(new_node);
  graph_item->node_items_.emplace_back(node_item);
  node_item->input_start = 0;
  node_item->output_start = 0;
  ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
  ASSERT_NE(node_item->node_executor, nullptr);
  ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sub_node, node_item->kernel_task), SUCCESS);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.allocator = NpuMemoryAllocator::GetAllocator();

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  ASSERT_NE(node_state->GetTaskContext(), nullptr);
  auto &task_context = *node_state->GetTaskContext();

  {
    // GetTaskDefs is null.
    FftsPlusAiCoreTask sub_task;
    ASSERT_EQ(sub_task.Load(hybrid_model, sub_node), SUCCESS);
    ASSERT_NE(sub_task.ffts_node_item_, nullptr);

    // SUCCESS for default function.
    ASSERT_EQ(sub_task.UpdateArgs(task_context), SUCCESS);
    ASSERT_EQ(sub_task.UpdateTilingData(task_context), SUCCESS);

    // SUCCESS for default function.
    ASSERT_EQ(sub_task.UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS);

    // SUCCESS for default function.
    sub_task.InitOpTiling(0, 0);
  }

  TBEHandleStore::GetInstance().StoreTBEHandle(sub_node->GetOpDesc()->GetName(), nullptr, nullptr);
  auto aic_aiv_task = reinterpret_cast<FftsPlusAicAivTask *>(node_item->kernel_task.get());
  ASSERT_NE(aic_aiv_task, nullptr);

  // GetAutoThreadParam failed, thread_dim is 0.
  aic_aiv_task->ffts_plus_ctx_update_ = MakeShared<FFTSPlusTaskInvalid>();
  ASSERT_EQ(aic_aiv_task->UpdateTilingData(task_context), FAILED); // GetAutoThreadParam failed for thread_dim is 0.
  ASSERT_EQ(aic_aiv_task->task_flush_.op_run_info.size(), 2U); // OpFftsCalculateV2 is success.

  aic_aiv_task->tiling_offset_ = {0, 24};
  aic_aiv_task->task_param_.thread_dim = 1;
  aic_aiv_task->task_param_.input_output_num = 2;
  aic_aiv_task->args_host_data_.resize((aic_aiv_task->task_param_.input_output_num + 2) * aic_aiv_task->task_param_.thread_dim, 0);
  aic_aiv_task->InitOpTiling(0, 0);
  ASSERT_EQ(aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS); // Test Feature

  // Same TilingKey(0, 0);
  aic_aiv_task->task_flush_.op_run_info.resize(2U);
  ASSERT_EQ(aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS);

  // Different TilingKey(0, 1)
  aic_aiv_task->task_flush_.op_run_info[1].SetTilingKey(1);
  ASSERT_EQ(aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS);

  // tune::FFTSNodeThread failed.
  auto empty_graph = MakeShared<ComputeGraph>("empty_graph");
  node_item->node->SetOwnerComputeGraph(empty_graph);
  ASSERT_EQ(aic_aiv_task->UpdateTilingData(task_context), INTERNAL_ERROR); // Failed for FFTSNodeThread.
  ASSERT_NE(aic_aiv_task->ffts_node_task_, nullptr); // ffts_node_task_ valid for Init is success.
  ASSERT_EQ(aic_aiv_task->UpdateTilingData(task_context), INTERNAL_ERROR); // Failed for FFTSNodeThread.
}

TEST_F(UtestFftsPlusSubTask, ffts_plus_mix_task) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  ComputeGraphPtr sgt_graph = ToComputeGraph(g1);
  const auto sgt_node = sgt_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(sgt_node, nullptr);

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/_arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt/_arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr sub_graph = ToComputeGraph(g2);
  const auto sub_node = sub_graph->FindNode("sgt/conv2d");
  EXPECT_NE(sub_node, nullptr);

  SetPartitionedCall(sgt_node, sub_graph);
  AttrUtils::SetBool(sgt_node->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);
  AttrUtils::SetStr(sub_node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIV");

  //////////////////////////////////////////////////////////////////////////////
  // Build FftsPlusTaskDef.
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(sgt_node->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ctx_def->set_op_index(sub_node->GetOpDesc()->GetId());
  ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIV));
  domi::FftsPlusMixAicAivCtxDef *aic_aiv_def = ctx_def->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(aic_aiv_def, false, false);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(sgt_graph);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.task_defs_[sgt_node].emplace_back(task_def);

  auto unique_graph_item = std::unique_ptr<GraphItem>(new(std::nothrow)GraphItem());
  auto graph_item = unique_graph_item.get();
  graph_item->total_inputs_ = 2;
  graph_item->total_outputs_ = 1;
  graph_item->SetName(sub_graph->GetName());
  hybrid_model.subgraph_items_.emplace(sub_graph->GetName(), std::move(unique_graph_item));

  {
    std::unique_ptr<NodeItem> new_node;
    ASSERT_EQ(NodeItem::Create(sgt_node, new_node), SUCCESS);
    NodeItem *node_item = new_node.get();
    hybrid_model.node_items_[sgt_node] = std::move(new_node);
    node_item->input_start = 0;
    node_item->output_start = 0;
    ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
    ASSERT_NE(node_item->node_executor, nullptr);
    ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sgt_node, node_item->kernel_task), SUCCESS);
  }

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(sub_node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[sub_node] = std::move(new_node);
  graph_item->node_items_.emplace_back(node_item);
  node_item->input_start = 0;
  node_item->output_start = 0;
  ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
  ASSERT_NE(node_item->node_executor, nullptr);
  ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sub_node, node_item->kernel_task), SUCCESS);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  ASSERT_NE(node_state->GetTaskContext(), nullptr);
  auto &task_context = *node_state->GetTaskContext();

  TBEHandleStore::GetInstance().StoreTBEHandle(sub_node->GetOpDesc()->GetName(), nullptr, nullptr);
  auto mix_aic_aiv_task = reinterpret_cast<FftsPlusMixAicAivTask *>(node_item->kernel_task.get());
  ASSERT_NE(mix_aic_aiv_task, nullptr);

  // tune::FFTSNodeThread failed.
  auto empty_graph = MakeShared<ComputeGraph>("empty_graph");
  node_item->node->SetOwnerComputeGraph(empty_graph);
  ASSERT_EQ(mix_aic_aiv_task->UpdateTilingData(task_context), INTERNAL_ERROR); // Failed for FFTSNodeThread.
  ASSERT_NE(mix_aic_aiv_task->ffts_node_task_, nullptr); // ffts_node_task_ valid for Init is success.
  ASSERT_EQ(mix_aic_aiv_task->UpdateTilingData(task_context), INTERNAL_ERROR); // Failed for FFTSNodeThread.

  mix_aic_aiv_task->task_param_.thread_dim = 1;
  mix_aic_aiv_task->task_param_.input_output_num = 2;
  mix_aic_aiv_task->tiling_offset_ = {0, 24, 64, 88};
  mix_aic_aiv_task->args_host_data_.resize((mix_aic_aiv_task->task_param_.input_output_num + 4U) * mix_aic_aiv_task->task_param_.thread_dim);
  mix_aic_aiv_task->InitOpTiling(0, 0);
  ASSERT_EQ(mix_aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS); // op_run_info size invalid.
  ASSERT_TRUE(mix_aic_aiv_task->task_flush_.op_run_info.empty());

  // Same Aic/Aiv TilingKey(0, 0, 0, 0).
  mix_aic_aiv_task->task_flush_.op_run_info.resize(4);
  ASSERT_EQ(mix_aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS);

  // Different Aic/Aic TilingKey(0, 1, 0, 1)
  mix_aic_aiv_task->task_flush_.op_run_info[1].SetTilingKey(1);
  mix_aic_aiv_task->task_flush_.op_run_info[3].SetTilingKey(1);
  ASSERT_EQ(mix_aic_aiv_task->UpdateAddrAndPrefCnt(*sub_node->GetOpDesc()), SUCCESS);
}

TEST_F(UtestFftsPlusSubTask, ffts_plus_cpu_task) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  ComputeGraphPtr sgt_graph = ToComputeGraph(g1);
  const auto sgt_node = sgt_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(sgt_node, nullptr);

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/_arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt/_arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr sub_graph = ToComputeGraph(g2);
  const auto sub_node = sub_graph->FindNode("sgt/conv2d");
  EXPECT_NE(sub_node, nullptr);

  SetPartitionedCall(sgt_node, sub_graph);
  AttrUtils::SetBool(sgt_node->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);
  AttrUtils::SetStr(sub_node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AICPU");

  //////////////////////////////////////////////////////////////////////////////
  // Build FftsPlusTaskDef.
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(sgt_node->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ctx_def->set_op_index(sub_node->GetOpDesc()->GetId());
  ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *ai_cpu_def = ctx_def->mutable_aicpu_ctx();
  InitAicpuCtxCtx(ai_cpu_def, false);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(sgt_graph);
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.task_defs_[sgt_node].emplace_back(task_def);

  auto unique_graph_item = std::unique_ptr<GraphItem>(new(std::nothrow)GraphItem());
  auto graph_item = unique_graph_item.get();
  graph_item->total_inputs_ = 2;
  graph_item->total_outputs_ = 1;
  graph_item->SetName(sub_graph->GetName());
  hybrid_model.subgraph_items_.emplace(sub_graph->GetName(), std::move(unique_graph_item));

  {
    std::unique_ptr<NodeItem> new_node;
    ASSERT_EQ(NodeItem::Create(sgt_node, new_node), SUCCESS);
    NodeItem *node_item = new_node.get();
    hybrid_model.node_items_[sgt_node] = std::move(new_node);
    node_item->input_start = 0;
    node_item->output_start = 0;
    ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
    ASSERT_NE(node_item->node_executor, nullptr);
    ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sgt_node, node_item->kernel_task), SUCCESS);
  }

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(sub_node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[sub_node] = std::move(new_node);
  graph_item->node_items_.emplace_back(node_item);
  node_item->input_start = 0;
  node_item->output_start = 0;
  ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
  ASSERT_NE(node_item->node_executor, nullptr);
  ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sub_node, node_item->kernel_task), SUCCESS);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.allocator = NpuMemoryAllocator::GetAllocator();

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  ASSERT_NE(node_state->GetTaskContext(), nullptr);
  auto &task_context = *node_state->GetTaskContext();

  // GetTaskDefs is null.
  FftsPlusAiCpuTask sub_task;
  ASSERT_EQ(sub_task.Load(hybrid_model, sub_node), SUCCESS);
  ASSERT_NE(sub_task.ffts_node_item_, nullptr);

  // SUCCESS for default function.
  ASSERT_EQ(sub_task.UpdateArgs(task_context), SUCCESS);

  ASSERT_EQ(sub_task.UpdateTilingData(task_context), SUCCESS);

  TBEHandleStore::GetInstance().StoreTBEHandle(sub_node->GetOpDesc()->GetName(), nullptr, nullptr);
  auto ai_cpu_task = reinterpret_cast<FftsPlusAiCpuTask *>(node_item->kernel_task.get());
  ASSERT_NE(ai_cpu_task, nullptr);
}

TEST_F(UtestFftsPlusSubTask, ffts_plus_mix_l2_task) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
    CHAIN(NODE("_arg_2", DATA)->NODE("PartitionedCall_1", PARTITIONEDCALL));
    CHAIN(NODE("_arg_3", DATA)->NODE("PartitionedCall_1"));
  };
  ComputeGraphPtr sgt_graph = ToComputeGraph(g1);
  const auto sgt_node = sgt_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(sgt_node, nullptr);

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/_arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt/_arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr sub_graph = ToComputeGraph(g2);
  const auto sub_node = sub_graph->FindNode("sgt/conv2d");
  EXPECT_NE(sub_node, nullptr);

  SetPartitionedCall(sgt_node, sub_graph);
  AttrUtils::SetStr(sub_node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIV");
  AttrUtils::SetStr(sub_node->GetOpDesc(), "_alias_engine_name", "ffts_plus");
  // set tbe kernel
  AttrUtils::SetStr(sub_node->GetOpDesc(), TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  std::vector<char> conv_bin(64, '\0');
  TBEKernelPtr conv_kernel = MakeShared<ge::OpKernelBin>("sgt/conv2d", std::move(conv_bin));
  sub_node->GetOpDesc()->SetExtAttr(std::string("_mix_aic") + OP_EXTATTR_NAME_TBE_KERNEL, conv_kernel);
  AttrUtils::SetStr(sub_node->GetOpDesc(), "_mix_aic" + sub_node->GetName() + "_kernelname", "sgt/conv2d");
  sub_node->GetOpDesc()->SetExtAttr(std::string("_mix_aiv") + OP_EXTATTR_NAME_TBE_KERNEL, conv_kernel);
  AttrUtils::SetStr(sub_node->GetOpDesc(), "_mix_aiv" + sub_node->GetName() + "_kernelname", "sgt/conv2d");
  AttrUtils::SetBool(sub_node->GetOpDesc(), "_kernel_list_first_name", true);
  AttrUtils::SetInt(sub_node->GetOpDesc(), "op_para_size", 2);
  DEF_GRAPH(g3) {
    CHAIN(NODE("_arg_0", DATA)->NODE("conv2d", CONV2D)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("conv2d"));
  };
  ComputeGraphPtr known_shape_sub_graph = ToComputeGraph(g3);
  const auto &known_partitioncall_node = sgt_graph->FindNode("PartitionedCall_1");
  EXPECT_NE(known_partitioncall_node, nullptr);
  SetPartitionedCall(known_partitioncall_node, known_shape_sub_graph);
  known_shape_sub_graph->SetGraphUnknownFlag(false);
  //////////////////////////////////////////////////////////////////////////////
  // Build FftsPlusTaskDef.
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(sub_node->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ctx_def->set_op_index(sub_node->GetOpDesc()->GetId());
  ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIV));
  domi::FftsPlusMixAicAivCtxDef *aic_aiv_def = ctx_def->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(aic_aiv_def, false, false);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(sgt_graph);
  GeModelPtr ge_model = std::make_shared<GeModel>();
  auto model_task_def = std::make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);
  sub_graph->SetGraphUnknownFlag(true);
  ge_root_model->subgraph_instance_name_to_model_[sub_graph->GetName()] = ge_model;
  HybridModel hybrid_model(ge_root_model);
  HybridModelBuilder hybrid_model_builder(hybrid_model);
  hybrid_model_builder.LoadGeModel(*known_shape_sub_graph, ge_model);
  ASSERT_NE(hybrid_model.known_shape_sub_models_.find(known_partitioncall_node),
            hybrid_model.known_shape_sub_models_.end());
  hybrid_model.task_defs_[sub_node].emplace_back(task_def);

  auto unique_graph_item = std::unique_ptr<GraphItem>(new (std::nothrow) GraphItem());
  auto graph_item = unique_graph_item.get();
  graph_item->total_inputs_ = 2;
  graph_item->total_outputs_ = 1;
  graph_item->SetName(sub_graph->GetName());
  hybrid_model.subgraph_items_.emplace(sub_graph->GetName(), std::move(unique_graph_item));

  {
    std::unique_ptr<NodeItem> new_node;
    ASSERT_EQ(NodeItem::Create(sub_node, new_node), SUCCESS);
    NodeItem *node_item = new_node.get();
    hybrid_model.node_items_[sub_node] = std::move(new_node);
    node_item->input_start = 0;
    node_item->output_start = 0;
    ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
    ASSERT_NE(node_item->node_executor, nullptr);
    ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sub_node, node_item->kernel_task), SUCCESS);
  }

  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(sub_node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  hybrid_model.node_items_[sub_node] = std::move(new_node);
  graph_item->node_items_.emplace_back(node_item);
  node_item->input_start = 0;
  node_item->output_start = 0;
  ASSERT_EQ(NodeExecutorManager::GetInstance().GetExecutor(*node_item, node_item->node_executor), SUCCESS);
  ASSERT_NE(node_item->node_executor, nullptr);
  ASSERT_EQ(node_item->node_executor->LoadTask(hybrid_model, sub_node, node_item->kernel_task), SUCCESS);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.allocator = NpuMemoryAllocator::GetAllocator();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  ASSERT_NE(node_state->GetTaskContext(), nullptr);
  auto &task_context = *node_state->GetTaskContext();

  // GetTaskDefs is null.
  FftsPlusMixL2Task sub_task;
  ASSERT_EQ(sub_task.Load(hybrid_model, sub_node), SUCCESS);
  ASSERT_EQ(sub_task.ffts_node_item_, nullptr);

  // SUCCESS for default function.
  ASSERT_EQ(sub_task.UpdateArgs(task_context), SUCCESS);

  // SUCCESS for default function.
  OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(CONV2D);
  OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(CONV2D);
  const auto op_tiling_func = [](const Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
    return true;
  };
  REGISTER_OP_TILING_UNIQ_V2(Conv2D, op_tiling_func, 202);
  OpTilingRegistryInterf_V2(CONV2D, op_tiling_func);

  AttrUtils::SetStr(sub_node->GetOpDesc(), "compile_info_json", "stub_json");
  AttrUtils::SetStr(sub_node->GetOpDesc(), "compile_info_key", "stub_key");
  sub_task.io_addrs_from_taskdef_.push_back(0);
  sub_task.mode_addr_idx_.emplace(0);
  ASSERT_EQ(sub_task.UpdateTilingData(task_context), SUCCESS);
  auto call_back = []() { return; };

  TBEHandleStore::GetInstance().StoreTBEHandle(sub_node->GetOpDesc()->GetName(), nullptr, nullptr);
  auto mixl2_task = reinterpret_cast<FftsPlusMixL2Task *>(node_item->kernel_task.get());
  ASSERT_NE(mixl2_task, nullptr);

  ASSERT_EQ(sub_task.ExecuteAsync(task_context, call_back), SUCCESS);
}
}  // namespace ge
