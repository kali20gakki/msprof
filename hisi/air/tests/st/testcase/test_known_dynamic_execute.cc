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

#include "gtest/gtest.h"
#include "ge_graph_dsl/graph_dsl.h"

#include "framework/executor/ge_executor.h"
#include "graph/execute/model_executor.h"
#include "test_tools_task_info.h"

using namespace std;
using namespace testing;

namespace ge {
class DynamicKnownTest : public testing::Test {
 protected:
  void SetUp() {
    GeExecutor::Initialize({});
  }
  void TearDown() {
    GeExecutor::FinalizeEx();
  }
};

static void BuildSampleGraph(ComputeGraphPtr &graph0, ComputeGraphPtr &graph1, ComputeGraphPtr &graph2,
                             ComputeGraphPtr &graph3, ComputeGraphPtr &graph3_then, ComputeGraphPtr &graph3_else,
                             ComputeGraphPtr &graph4,  uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g0) { // Root Graph.
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 2);
    auto data_3 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 3);
    CHAIN(NODE("PartitionedCall_i", PARTITIONEDCALL)->EDGE(0, 0)->
          NODE("PartitionedCall_d", PARTITIONEDCALL)->EDGE(0, 0)->
          NODE("PartitionedCall_s", PARTITIONEDCALL)->EDGE(0, 0)->
          NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));
    CHAIN(NODE("PartitionedCall_i")->EDGE(0, 0)->
          NODE("PartitionedCall_k", PARTITIONEDCALL)->EDGE(0, 1)->
          NODE("PartitionedCall_s")->EDGE(1, 1)->
          NODE(NODE_NAME_NET_OUTPUT));

    CHAIN(NODE("_arg_0", data_0)->EDGE(0, 0)->NODE("PartitionedCall_i")->EDGE(1, 1)->NODE("PartitionedCall_d"));
    CHAIN(NODE("_arg_1", data_1)->EDGE(0, 1)->NODE("PartitionedCall_i")->EDGE(2, 2)->NODE("PartitionedCall_d"));

    CHAIN(NODE("_arg_2", data_2)->EDGE(0, 2)->NODE("PartitionedCall_i")->EDGE(2, 1)->NODE("PartitionedCall_k")->EDGE(1, 2)->NODE("PartitionedCall_s"));
    CHAIN(NODE("_arg_3", data_3)->EDGE(0, 3)->NODE("PartitionedCall_i")->EDGE(3, 2)->NODE("PartitionedCall_k"));
  };
  graph0 = ToComputeGraph(g0);
  graph0->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph0, mem_offset, true);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) { // Input Graph.
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    auto data_3 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 3);
    CHAIN(NODE("input/_arg_0", data_0)->EDGE(0, 0)->NODE("input/Node_Output", NETOUTPUT));
    CHAIN(NODE("input/_arg_1", data_1)->EDGE(0, 1)->NODE("input/Node_Output"));
    CHAIN(NODE("input/_arg_2", data_2)->EDGE(0, 2)->NODE("input/Node_Output"));
    CHAIN(NODE("input/_arg_3", data_3)->EDGE(0, 3)->NODE("input/Node_Output"));
  };
  graph1 = ToComputeGraph(g1);
  graph1->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph1, mem_offset);
  AddPartitionedCall(graph0, "PartitionedCall_i", graph1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g2) { // Unknown Graph.
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    CHAIN(NODE("dynamic/_arg_0", data_0)->EDGE(0, 0)->NODE("dynamic/add", ADD)->NODE("dynamic/mul", MUL));
    CHAIN(NODE("dynamic/_arg_1", data_1)->EDGE(0, 1)->NODE("dynamic/add"));
    CHAIN(NODE("dynamic/_arg_2", data_2)->EDGE(0, 1)->NODE("dynamic/mul")->NODE("dynamic/Node_Output", NETOUTPUT));
  };
  graph2 = ToComputeGraph(g2);
  graph2->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph2, mem_offset);
  AddPartitionedCall(graph0, "PartitionedCall_d", graph2);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3) { // Known Graph.
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    CHAIN(NODE("davinci/_arg_0", data_0)->EDGE(0, 0)->NODE("davinci/add", ADD)->NODE("davinci/relu", RELU)->NODE("davinci/Node_Output", NETOUTPUT));
    CHAIN(NODE("davinci/_arg_1", data_1)->EDGE(0, 1)->NODE("davinci/add", ADD));
    CHAIN(NODE("davinci/add")->EDGE(0, 0)->NODE("davinci/pow", POW)->NODE("davinci/Node_Output", NETOUTPUT));
    CHAIN(NODE("davinci/_arg_0", data_0)->EDGE(0, 0)->NODE("davinci/if_node", IF));
    CHAIN(NODE("davinci/_arg_1", data_1)->EDGE(0, 1)->NODE("davinci/if_node", IF));
    CHAIN(NODE("davinci/_arg_2", data_2)->EDGE(0, 2)->NODE("davinci/if_node", IF));
    CHAIN(NODE("davinci/if_node")->EDGE(0, 1)->NODE("davinci/pow", POW));
  };
  graph3 = ToComputeGraph(g3);
  graph3->SetGraphUnknownFlag(false);
  SetUnknownOpKernel(graph3, mem_offset, true);
  AddPartitionedCall(graph0, "PartitionedCall_k", graph3);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3_then) { // Known Graph - then_branch.
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    auto label_sw = OP_CFG(LABELSWITCHBYINDEX).Attr(ATTR_NAME_LABEL_SWITCH_LIST, std::vector<int64_t>{0, 1});
    auto label_01 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 1);
    auto label_go = OP_CFG(LABELGOTOEX).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 2);

    CHAIN(NODE("then_branch/SwitchIndexData", data_0)->EDGE(0, 0)->NODE("then_branch/switch", label_sw)->CTRL_EDGE()->
          NODE("then_branch/label_1", label_01)->CTRL_EDGE()->NODE("then_branch/_arg_1", data_1));
    CHAIN(NODE("then_branch/label_1", label_01)->CTRL_EDGE()->NODE("then_branch/_arg_2", data_2));

    CHAIN(NODE("then_branch/_arg_1", data_1)->EDGE(0, 0)->NODE("then_branch/add", ADD)->NODE("then_branch/Node_Output", NETOUTPUT));
    CHAIN(NODE("then_branch/_arg_2", data_2)->EDGE(0, 1)->NODE("then_branch/add", ADD));

    CHAIN(NODE("then_branch/Node_Output", NETOUTPUT)->CTRL_EDGE()->NODE("then_branch/goto", label_go));
  };
  graph3_then = ToComputeGraph(g3_then);
  graph3_then->SetGraphUnknownFlag(false);
  SetUnknownOpKernel(graph3_then, mem_offset);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3_else) { // Known Graph - else_branch.
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    auto label_00 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 0);
    auto label_02 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 2);

    CHAIN(NODE("else_branch/label_0", label_00)->CTRL_EDGE()->NODE("else_branch/_arg_1", data_1));
    CHAIN(NODE("else_branch/label_0", label_00)->CTRL_EDGE()->NODE("else_branch/_arg_2", data_2));

    CHAIN(NODE("else_branch/_arg_1", data_1)->EDGE(0, 0)->NODE("else_branch/sub", SUB)->NODE("else_branch/Node_Output", NETOUTPUT));
    CHAIN(NODE("else_branch/_arg_2", data_2)->EDGE(0, 1)->NODE("else_branch/sub", SUB));

    CHAIN(NODE("else_branch/Node_Output", NETOUTPUT)->CTRL_EDGE()->NODE("else_branch/label_2", label_02));
  };
  graph3_else = ToComputeGraph(g3_else);
  graph3_else->SetGraphUnknownFlag(false);
  SetUnknownOpKernel(graph3_else, mem_offset);
  AddIfBranchs(graph3, "davinci/if_node", graph3_then, graph3_else);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g4) { // Known Graph.
    const auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    const auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    const auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);

    GeTensorDesc data_tensor_desc(GeShape(), FORMAT_ND, DT_INT64);
    int64_t const_value_t = 1;
    auto w_tensor_t = MakeShared<GeTensor>(data_tensor_desc, (uint8_t *)&const_value_t, sizeof(int64_t));
    const auto const_op_t = OP_CFG(CONSTANT).Weight(w_tensor_t);
    int64_t const_value_f = 0;
    auto w_tensor_f = MakeShared<GeTensor>(data_tensor_desc, (uint8_t *)&const_value_f, sizeof(int64_t));
    const auto const_op_f = OP_CFG(CONSTANT).Weight(w_tensor_f);

    // cond
    CHAIN(NODE("collect/_arg_0", data_0)->EDGE(0, 0)->NODE("collect/less", LESS)->NODE("collect/less_cast", CAST));
    CHAIN(NODE("collect/_arg_1", data_1)->EDGE(0, 1)->NODE("collect/less"));
    const auto active_s = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{1});
    CHAIN(NODE("collect/less_cast", CAST)->CTRL_EDGE()->NODE("collect/active_s", active_s));

    // true branch
    const auto switch_t = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_EQUAL))
                                              .Attr(ATTR_NAME_SWITCH_DATA_TYPE, static_cast<int64_t>(RT_SWITCH_INT64))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("collect/active_s", active_s)->CTRL_EDGE()->NODE("collect/switch_t", switch_t));
    CHAIN(NODE("collect/less_cast", CAST)->EDGE(0, 0)->NODE("collect/switch_t", switch_t));
    CHAIN(NODE("collect/const_t", const_op_t)->EDGE(0, 1)->NODE("collect/switch_t", switch_t));

    CHAIN(NODE("collect/switch_t", switch_t)->CTRL_EDGE()->NODE("collect/add", ADD));
    CHAIN(NODE("collect/_arg_0", data_0)->EDGE(0, 0)->NODE("collect/add", ADD)->NODE("collect/output_t", MEMCPYASYNC));
    CHAIN(NODE("collect/_arg_2", data_2)->EDGE(0, 1)->NODE("collect/add"));

    const auto active_t = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{4});
    CHAIN(NODE("collect/output_t", MEMCPYASYNC)->CTRL_EDGE()->NODE("collect/active_t", active_t));

    // false branch
    const auto switch_f = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_EQUAL))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{3});
    CHAIN(NODE("collect/active_s", active_s)->CTRL_EDGE()->NODE("collect/switch_f", switch_f));
    CHAIN(NODE("collect/less_cast", CAST)->EDGE(0, 0)->NODE("collect/switch_f", switch_f));
    CHAIN(NODE("collect/const_f", const_op_f)->EDGE(0, 1)->NODE("collect/switch_f", switch_f));

    CHAIN(NODE("collect/switch_f", switch_f)->CTRL_EDGE()->NODE("collect/sub", SUB));
    CHAIN(NODE("collect/_arg_1", data_1)->EDGE(0, 0)->NODE("collect/sub", SUB)->NODE("collect/output_f", MEMCPYASYNC));
    CHAIN(NODE("collect/_arg_2", data_2)->EDGE(0, 1)->NODE("collect/sub"));

    const auto active_f = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{4});
    CHAIN(NODE("collect/output_f", MEMCPYASYNC)->CTRL_EDGE()->NODE("collect/active_f", active_f));

    // merge output
    CHAIN(NODE("collect/output_t")->EDGE(0, 0)->NODE("collect/merge", STREAMMERGE)->NODE("collect/relu", RELU));
    CHAIN(NODE("collect/output_f")->EDGE(0, 1)->NODE("collect/merge", STREAMMERGE));
    CHAIN(NODE("collect/active_t", active_t)->CTRL_EDGE()->NODE("collect/merge", STREAMMERGE));
    CHAIN(NODE("collect/active_f", active_f)->CTRL_EDGE()->NODE("collect/merge", STREAMMERGE));
    CHAIN(NODE("collect/relu", RELU)->EDGE(0, 0)->NODE("collect/output", NETOUTPUT)); // for: AICPU --> NETOUPUT
  };
  graph4 = ToComputeGraph(g4);
  graph4->SetGraphUnknownFlag(false);
  SetUnknownOpKernel(graph4, mem_offset, true);
  AddPartitionedCall(graph0, "PartitionedCall_s", graph4);
}

void BuildGraphModel2(const ComputeGraphPtr &graph, const uint32_t mem_offset, GeRootModelPtr &ge_root_model) {
  const auto model_task_def = MakeShared<domi::ModelTaskDef>();
  const auto ge_model = MakeShared<GeModel>();
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetName(graph->GetName());

  TBEKernelStore tbe_kernel_store;
  InitKernelTaskDef_TE(graph, *model_task_def, "dynamic/add", tbe_kernel_store);
  InitKernelTaskDef_TE(graph, *model_task_def, "dynamic/mul", tbe_kernel_store);

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);

  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));
}

static void BuildThenBranch(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, TBEKernelStore &kernel_store) {
  InitLabelSwitchDef(graph, model_def, "then_branch/switch");
  InitLabelSetDef(graph, model_def, "then_branch/label_1");

  InitKernelTaskDef(graph, model_def, "then_branch/add");

  InitLabelGotoDef(graph, model_def, "then_branch/goto");
}

static void BuildElseBranch(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, TBEKernelStore &kernel_store) {
  InitLabelSetDef(graph, model_def, "else_branch/label_0");

  InitKernelTaskDef(graph, model_def, "else_branch/sub");

  // for DavinciModel::GetOpStream
  const auto node = graph->FindNode("else_branch/label_2");
  node->GetOpDesc()->SetStreamId(1);
  InitLabelSetDef(graph, model_def, "else_branch/label_2");
}

void BuildGraphModel3(const ComputeGraphPtr &graph, const uint32_t mem_offset, GeRootModelPtr &ge_root_model,
                      const ComputeGraphPtr &then_branch, const ComputeGraphPtr &else_branch) {
  const auto model_task_def = MakeShared<domi::ModelTaskDef>();
  const auto ge_model = MakeShared<GeModel>();
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetName(graph->GetName());

  TBEKernelStore tbe_kernel_store;
  InitKernelTaskDef_TE(graph, *model_task_def, "davinci/add", tbe_kernel_store);
  InitKernelExTaskDef(graph, *model_task_def, "davinci/relu");

  BuildThenBranch(then_branch, *model_task_def, tbe_kernel_store);
  BuildElseBranch(else_branch, *model_task_def, tbe_kernel_store);

  InitKernelTaskDef_AI_CPU(graph, *model_task_def, "davinci/pow");

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);

  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 2));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));
}

void BuildGraphModel4(const ComputeGraphPtr &graph, const uint32_t mem_offset, GeRootModelPtr &ge_root_model) {
  const auto model_task_def = MakeShared<domi::ModelTaskDef>();
  const auto ge_model = MakeShared<GeModel>();
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetName(graph->GetName());

  TBEKernelStore tbe_kernel_store;
  InitKernelTaskDef_TE_SM(graph, *model_task_def, "collect/less");
  InitKernelTaskDef_TE_SM(graph, *model_task_def, "collect/less_cast");
  InitStreamActiveDef(graph, *model_task_def, "collect/active_s");

  InitStreamSwitchDef(graph, *model_task_def, "collect/switch_t");
  InitKernelTaskDef_TE_SM(graph, *model_task_def, "collect/add");
  InitMemcpyAsyncDef(graph, *model_task_def, "collect/output_t");
  InitStreamActiveDef(graph, *model_task_def, "collect/active_t");

  InitStreamSwitchDef(graph, *model_task_def, "collect/switch_f");
  InitKernelTaskDef_TE_SM(graph, *model_task_def, "collect/sub");
  InitMemcpyAsyncDef(graph, *model_task_def, "collect/output_f");
  InitStreamActiveDef(graph, *model_task_def, "collect/active_f");

  InitStreamMergeDef(graph, *model_task_def, "collect/merge");
  InitKernelTaskDef_AI_CPU(graph, *model_task_def, "collect/relu");  // for: AICPU -> NETOUTPUT

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);

  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 5));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));
}

Status KnownDynamicExecute(ComputeGraphPtr &graph, const GeRootModelPtr &ge_root_model, const FlowModelPtr &flow_root_model) {
  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  //-- Run Asynchronous
  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  int64_t value_0 = 127;
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  int64_t value_1 = 100;
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  int64_t value_2 = 127;
  Tensor tensor_2(tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  int64_t value_3 = 100;
  Tensor tensor_3(tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<Tensor> input_tensors{ tensor_0, tensor_1, tensor_2, tensor_3 };

  std::mutex run_mutex;
  std::condition_variable model_run_cv;
  Status run_status = FAILED;
  std::vector<Tensor> run_outputs;
  const auto callback = [&](Status status, std::vector<Tensor> &outputs) {
    std::unique_lock<std::mutex> lock(run_mutex);
    run_status = status;
    run_outputs.swap(outputs);
    model_run_cv.notify_one();
  };

  GEThreadLocalContext context;
  error_message::Context error_context;
  graph_node->Lock();
  RunArgs run_args{graph_node, graph_id, 2001, error_context, input_tensors, flow_root_model, context, callback};
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  std::unique_lock<std::mutex> lock(run_mutex);
  EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
  EXPECT_EQ(run_status, SUCCESS);
  EXPECT_EQ(run_outputs.size(), 2U);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  ge_root_model->ClearAllModelId();
  return run_status;
}

Status KnownDynamicRunSync(ComputeGraphPtr &graph, const GeRootModelPtr &ge_root_model, const FlowModelPtr &flow_root_model) {
  GraphId graph_id = 2001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(false);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  //-- Run Synchronous
  GeTensorDesc sync_tensor_desc(GeShape(), FORMAT_ND, DT_INT64);
  int64_t value_0 = 127;
  GeTensor sync_tensor_0(sync_tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  int64_t value_1 = 100;
  GeTensor sync_tensor_1(sync_tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  int64_t value_2 = 127;
  GeTensor sync_tensor_2(sync_tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  int64_t value_3 = 100;
  GeTensor sync_tensor_3(sync_tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<GeTensor> sync_input_tensors{ sync_tensor_0, sync_tensor_1, sync_tensor_2, sync_tensor_3 };

  std::vector<GeTensor> sync_outputs;
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, sync_input_tensors, sync_outputs), SUCCESS);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  ge_root_model->ClearAllModelId();
  return SUCCESS;
}

TEST_F(DynamicKnownTest, execute_known_from_dynamic) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph, inputs, dynamic, then_branch, else_branch, davinci, collect;
  BuildSampleGraph(graph, inputs, dynamic, davinci, then_branch, else_branch, collect, mem_offset);
  EXPECT_NE(graph, nullptr);

  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "known_dynamic_execute");

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  BuildGraphModel2(dynamic, mem_offset, ge_root_model);
  BuildGraphModel3(davinci, mem_offset, ge_root_model, then_branch, else_branch);
  BuildGraphModel4(collect, mem_offset, ge_root_model);

  {
    // Test LoadModelOnline: -> DoLoadHybridModelOnline.
    EXPECT_EQ(KnownDynamicExecute(graph, ge_root_model, flow_root_model), SUCCESS);
    EXPECT_EQ(KnownDynamicRunSync(graph, ge_root_model, flow_root_model), SUCCESS);
  }
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
}
} // namespace ge
