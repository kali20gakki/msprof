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

#include "test_tools_task_info.h"
#include "framework/executor/ge_executor.h"
#include "graph/execute/model_executor.h"
#include "framework/common/helper/om_file_helper.h"
#include "framework/common/helper/model_helper.h"

using namespace std;
using namespace testing;

namespace ge {
class CtrlFlowDynamicTest : public testing::Test {
 protected:
  void SetUp() {
    GeExecutor::Initialize({});

    //NodeExecutorManager::GetInstance().engine_mapping_.clear();
    //auto &engine_mapping = NodeExecutorManager::GetInstance().engine_mapping_;
    //engine_mapping.emplace("DNN_VM_RTS_OP_STORE", NodeExecutorManager::ExecutorType::RTS);
    //engine_mapping.emplace("DNN_VM_GE_LOCAL_OP_STORE", NodeExecutorManager::ExecutorType::GE_LOCAL);
    //
    //NodeExecutorManager::GetInstance().executors_.clear();
    //auto &task_executor = NodeExecutorManager::GetInstance().executors_;
    //task_executor.emplace(NodeExecutorManager::ExecutorType::RTS, std::unique_ptr<NodeExecutor>(new RtsNodeExecutor()));
    //task_executor.emplace(NodeExecutorManager::ExecutorType::GE_LOCAL, std::unique_ptr<NodeExecutor>(new GeLocalNodeExecutor()));
  }
  void TearDown() {
    //NodeExecutorManager::GetInstance().engine_mapping_.clear();
    //NodeExecutorManager::GetInstance().executors_.clear();

    GeExecutor::FinalizeEx();
  }
};

/*******************************************************************************
*             |
*           Merge
*          /     \.
*  Active /       \ Active
*        /         \.
*       Add       Sub
*      |   \     /   |
*      |    \ _ /    |
*      |    /   \    |
*      |   /     \   |
*    Switch       Switch
*     |   \       /   |
*     |    \     /    |
*     |    Active     |
*     |     \  /      |
*     |     Less      |
*     |     /   \     |
*     |    /     \    |
*      Data       Data
******************************************************************************/
static void BuildSampleCondGraph(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g0) {
    CHAIN(NODE("_arg_0", DATA)->NODE("add", IDENTITY)->NODE("merge", STREAMMERGE)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));
    CHAIN(NODE("const_0", CONSTANT)->NODE("add"));
    CHAIN(NODE("_arg_1", DATA)->NODE("sub", IDENTITY)->NODE("merge"));
    CHAIN(NODE("const_1", CONSTANT)->NODE("sub"));

    const auto switch_t = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_EQUAL))
                                              .Attr(ATTR_NAME_SWITCH_DATA_TYPE, static_cast<int64_t>(RT_SWITCH_INT64))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    const auto switch_f = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_NOT_EQUAL))
                                              .Attr(ATTR_NAME_SWITCH_DATA_TYPE, static_cast<int64_t>(RT_SWITCH_INT64))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("_arg_0")->EDGE(0, 0)->NODE("less", IDENTITY)->EDGE(0, 0)->NODE("Less/StreamSwitch_t", switch_t)->CTRL_EDGE()->NODE("add"));
    CHAIN(NODE("const_0")->EDGE(0, 1)->NODE("Less/StreamSwitch_t"));
    CHAIN(NODE("_arg_1")->EDGE(0, 1)->NODE("less", IDENTITY)->EDGE(0, 0)->NODE("Less/StreamSwitch_f", switch_f)->CTRL_EDGE()->NODE("sub"));
    CHAIN(NODE("const_1")->EDGE(0, 1)->NODE("Less/StreamSwitch_f"));

    const auto active_s = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{1});
    CHAIN(NODE("less")->CTRL_EDGE()->NODE("Less_StreamActive", active_s)->CTRL_EDGE()->NODE("Less/StreamSwitch_t"));
    CHAIN(NODE("Less_StreamActive")->CTRL_EDGE()->NODE("Less/StreamSwitch_f"));

    const auto active_0 = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    const auto active_1 = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("add")->CTRL_EDGE()->NODE("merge_input_0_active", active_0)->CTRL_EDGE()->NODE("merge"));
    CHAIN(NODE("sub")->CTRL_EDGE()->NODE("merge_input_1_active", active_1)->CTRL_EDGE()->NODE("merge"));
  };
  graph = ToComputeGraph(g0);
  graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph, mem_offset, true);
}

void BuildGraphModel(const ComputeGraphPtr &graph, uint32_t mem_offset, GeModelPtr &ge_model, TBEKernelStore &tbe_kernel_store) {
  InitConstantNode(graph, "const_0", 1);
  InitConstantNode(graph, "const_1", 0);

  std::shared_ptr<domi::ModelTaskDef> model_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef_TE(graph, *model_def, "less", tbe_kernel_store);

  InitStreamActiveDef(graph, *model_def, "Less_StreamActive");
  InitStreamSwitchDef(graph, *model_def, "Less/StreamSwitch_f");
  InitStreamSwitchDef(graph, *model_def, "Less/StreamSwitch_t");

  InitKernelTaskDef_TE(graph, *model_def, "add", tbe_kernel_store);
  InitKernelTaskDef_TE(graph, *model_def, "sub", tbe_kernel_store);

  InitStreamActiveDef(graph, *model_def, "merge_input_0_active");
  InitStreamActiveDef(graph, *model_def, "merge_input_1_active");
  InitStreamMergeDef(graph, *model_def, "merge");

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_def);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));
}

Status OnlineInferDynamic(ComputeGraphPtr &graph, const GeModelPtr &ge_model) {
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  int64_t value_0 = 127;
  int64_t value_1 = 100;
  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  const std::vector<Tensor> input_tensors{tensor_0, tensor_1};

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
  EXPECT_EQ(run_outputs.size(), 1U);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  return run_status;
}

TEST_F(CtrlFlowDynamicTest, ctrl_flow_dynamic_execute) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildSampleCondGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);
  graph->SetGraphUnknownFlag(true);

  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "ctrl_flow_dynamic");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");

  GeModelPtr ge_model;
  TBEKernelStore tbe_kernel_store;
  BuildGraphModel(graph, mem_offset, ge_model, tbe_kernel_store);
  EXPECT_NE(ge_model, nullptr);

  {
    // Test LoadModelOnline: V1 control flow dynamic.
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model), SUCCESS);
  }

  // Serialization GeModel to memory.
  SaveParam save_param;
  ModelHelper model_helper;
  model_helper.SetSaveMode(false);  // Save to buffer.
  ModelBufferData model_buffer;
  EXPECT_TRUE(tbe_kernel_store.Build());
  ge_model->SetTBEKernelStore(tbe_kernel_store);
  EXPECT_EQ(model_helper.SaveToOmModel(ge_model, save_param, "file_name_prefix", model_buffer), SUCCESS);
  const ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", ""};

  {
    // Test LoadModelOffline: V1 control flow dynamic.
    int64_t arg_0 = 127;
    int64_t arg_1 = 100;
    RunModelData run_input_data;
    run_input_data.blobs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
    run_input_data.blobs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});

    int64_t arg_3 = 111;
    RunModelData run_output_data;
    run_output_data.blobs.emplace_back(DataBuffer{&arg_3, sizeof(arg_3), false, 0});

    uint32_t model_id = 0;
    GeExecutor ge_executor;
    EXPECT_EQ(ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0U, nullptr, 0U), SUCCESS);
    EXPECT_EQ(ge_executor.ExecModel(model_id, nullptr, run_input_data, run_output_data, true), SUCCESS);
    EXPECT_EQ(ge_executor.UnloadModel(model_id), SUCCESS);
  }
}
} // namespace ge

