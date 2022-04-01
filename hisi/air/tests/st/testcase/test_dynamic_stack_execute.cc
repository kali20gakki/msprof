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

// To test the execution of dynamic data flow ops (Stack series)

#include "gtest/gtest.h"
#include "ge_graph_dsl/graph_dsl.h"

#include "test_tools_task_info.h"
#include "framework/executor/ge_executor.h"
#include "graph/execute/model_executor.h"

using namespace std;
using namespace testing;

namespace ge {
class DynamicStackExecuteTest : public testing::Test {
 protected:
  void SetUp() {
    GeExecutor::Initialize({});
  }
  void TearDown() {
    GeExecutor::FinalizeEx();
  }
};

const static std::unordered_set<std::string> kDataFlowOps = {STACK, STACKPUSH, STACKPOP, STACKCLOSE};
const static std::unordered_set<std::string> kDependComputeOps = {STACK};
/*******************************************************************************

******************************************************************************/
static void BuildNormalStackGraph(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  DEF_GRAPH(g0) {
    CHAIN(NODE("_arg_0", DATA)->EDGE(0, 0)->NODE("add0", ADD));
    CHAIN(NODE("_arg_1", DATA)->EDGE(0, 1)->NODE("add0", ADD));

    CHAIN(NODE("const_0", CONSTANT)->EDGE(0, 0)->NODE("stack", STACK));
    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("add0", ADD)->EDGE(0, 1)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("stack_push", STACKPUSH)->EDGE(0, 0)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_pop", STACKPOP));
    CHAIN(NODE("stack_pop", STACKPOP)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("_arg_2", DATA)->EDGE(0, 1)->NODE("add1", ADD));
    CHAIN(NODE("add1", ADD)->EDGE(0, 1)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CTRL_CHAIN(NODE("stack_push", STACKPUSH)->NODE("stack_pop", STACKPOP));
  };
  graph = ToComputeGraph(g0);
  graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph, mem_offset, true);
  for (const auto &node : graph->GetDirectNode()) {
    const auto op_desc = node->GetOpDesc();
    if (kDataFlowOps.count(node->GetType()) != 0L) {
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_DATA_FLOW_HANDLE, 1);
      op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
    }
    if (kDependComputeOps.count(node->GetType()) != 0L) {
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_COMPUTE);
    }
    if (node->GetType() == STACK) {
      const map<string, uint32_t> name_index = {{"max_size", 0}};
      op_desc->UpdateInputName(name_index);
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_DATA_FLOW_MAX_SIZE, 1);
    }
    // for InitConstantNode
    if (node->GetName() == "const_0") {
      op_desc->MutableOutputDesc(0)->SetDataType(DT_INT32);
    }
  }
}

/*******************************************************************************

******************************************************************************/
static void BuildGraphWithTwoStack(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  DEF_GRAPH(g0) {
    // input
    CHAIN(NODE("_arg_0", DATA)->EDGE(0, 0)->NODE("add0", ADD));
    CHAIN(NODE("_arg_1", DATA)->EDGE(0, 1)->NODE("add0", ADD));

    // stack
    CHAIN(NODE("const_0", CONSTANT)->EDGE(0, 0)->NODE("stack", STACK));
    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("add0", ADD)->EDGE(0, 1)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("stack_push", STACKPUSH)->EDGE(0, 0)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_pop", STACKPOP));
    CHAIN(NODE("stack_pop", STACKPOP)->EDGE(0, 0)->NODE("add1", ADD));

    // stack_1
    CHAIN(NODE("const_1", CONSTANT)->EDGE(0, 0)->NODE("stack_1", STACK));
    CHAIN(NODE("stack_1", STACK)->EDGE(0, 0)->NODE("stack_push_1", STACKPUSH));
    CHAIN(NODE("add0", ADD)->EDGE(0, 1)->NODE("stack_push_1", STACKPUSH));
    CHAIN(NODE("stack_push_1", STACKPUSH)->EDGE(0, 1)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CHAIN(NODE("stack_1", STACK)->EDGE(0, 0)->NODE("stack_pop_1", STACKPOP));
    CHAIN(NODE("stack_pop_1", STACKPOP)->EDGE(0, 1)->NODE("add1", ADD));
    CHAIN(NODE("add1", ADD)->EDGE(0, 2)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CTRL_CHAIN(NODE("stack_push", STACKPUSH)->NODE("stack_push_1", STACKPUSH));
    CTRL_CHAIN(NODE("stack_push_1", STACKPUSH)->NODE("stack_pop", STACKPOP));
    CTRL_CHAIN(NODE("stack_pop", STACKPOP)->NODE("stack_pop_1", STACKPOP));
  };
  graph = ToComputeGraph(g0);
  graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph, mem_offset, true);
  for (const auto &node : graph->GetDirectNode()) {
    const auto op_desc = node->GetOpDesc();
    if (kDataFlowOps.count(node->GetType()) != 0L) {
      if (node->GetName().find("_1") != std::string::npos) {
        (void)AttrUtils::SetInt(op_desc, ATTR_NAME_DATA_FLOW_HANDLE, 2);
      } else {
        (void)AttrUtils::SetInt(op_desc, ATTR_NAME_DATA_FLOW_HANDLE, 1);
      }
      op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
    }
    if (kDependComputeOps.count(node->GetType()) != 0L) {
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_COMPUTE);
    }
    if (node->GetType() == STACK) {
      const map<string, uint32_t> name_index = {{"max_size", 0}};
      op_desc->UpdateInputName(name_index);
    }
    // for InitConstantNode
    if (node->GetName() == "const_0" || (node->GetName() == "const_1")) {
      op_desc->MutableOutputDesc(0)->SetDataType(DT_INT32);
    }
  }
}

/*******************************************************************************

******************************************************************************/
static void BuildStackGraphWithExeception(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  DEF_GRAPH(g0) {
    CHAIN(NODE("_arg_0", DATA)->EDGE(0, 0)->NODE("add0", ADD));
    CHAIN(NODE("_arg_1", DATA)->EDGE(0, 1)->NODE("add0", ADD));

    CHAIN(NODE("const_0", CONSTANT)->EDGE(0, 0)->NODE("stack", STACK));
    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("add0", ADD)->EDGE(0, 1)->NODE("stack_push", STACKPUSH));
    CHAIN(NODE("stack_push", STACKPUSH)->EDGE(0, 0)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    // close stack
    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_close", STACKCLOSE));

    // pop after close
    CHAIN(NODE("stack", STACK)->EDGE(0, 0)->NODE("stack_pop", STACKPOP));
    CHAIN(NODE("stack_pop", STACKPOP)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("_arg_2", DATA)->EDGE(0, 1)->NODE("add1", ADD));
    CHAIN(NODE("add1", ADD)->EDGE(0, 1)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));

    CTRL_CHAIN(NODE("stack_push", STACKPUSH)->NODE("stack_close", STACKCLOSE));
    CTRL_CHAIN(NODE("stack_close", STACKCLOSE)->NODE("stack_pop", STACKPOP));
  };
  graph = ToComputeGraph(g0);
  graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph, mem_offset, true);
  for (const auto &node : graph->GetDirectNode()) {
    const auto op_desc = node->GetOpDesc();
    if (kDataFlowOps.count(node->GetType()) != 0L) {
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_DATA_FLOW_HANDLE, 1);
      op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
    }
    if (kDependComputeOps.count(node->GetType()) != 0L) {
      (void)AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_COMPUTE);
    }
    if (node->GetType() == STACK) {
      const map<string, uint32_t> name_index = {{"max_size", 0}};
      op_desc->UpdateInputName(name_index);
    }
    // for InitConstantNode
    if (node->GetName() == "const_0") {
      op_desc->MutableOutputDesc(0)->SetDataType(DT_UINT32); // will not get max_size from tensor
    }
  }
}

static void CreateGeModel(const ComputeGraphPtr &graph, uint32_t mem_offset, GeModelPtr &ge_model,
                          TBEKernelStore &tbe_kernel_store,
                          const std::vector<int32_t> &weights_value) {
  std::shared_ptr<domi::ModelTaskDef> model_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef_TE(graph, *model_def, "add0", tbe_kernel_store);
  InitKernelTaskDef_TE(graph, *model_def, "add1", tbe_kernel_store);

  size_t weight_size = weights_value.size() * sizeof(int32_t);
  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_def);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));
}

static void BuildNormalStackModel(const ComputeGraphPtr &graph, uint32_t mem_offset, GeModelPtr &ge_model,
                                  TBEKernelStore &tbe_kernel_store) {
  InitConstantNode(graph, "const_0", 1);
  std::vector<int32_t> weights_value = {1};
  CreateGeModel(graph, mem_offset, ge_model, tbe_kernel_store, weights_value);
}

static void BuildModelForTwoStackGraph(const ComputeGraphPtr &graph, uint32_t mem_offset, GeModelPtr &ge_model,
                                       TBEKernelStore &tbe_kernel_store) {
  InitConstantNode(graph, "const_0", 1);
  InitConstantNode(graph, "const_1", 1);
  std::vector<int32_t> weights_value = {1, 2};
  CreateGeModel(graph, mem_offset, ge_model, tbe_kernel_store, weights_value);
}

static Status DynamicStackExecute(ComputeGraphPtr &graph, const GeModelPtr &ge_model,
                                  const int32_t input_num, const size_t output_num, const bool check_output = true) {
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  std::vector<Tensor> input_tensors;
  for (int32_t i = 0; i < input_num; ++i) {
    int64_t value = 100 + i;
    TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
    Tensor tensor(tensor_desc, (uint8_t *)&value, sizeof(value));
    input_tensors.emplace_back(std::move(tensor));
  }

  std::mutex run_mutex;
  std::condition_variable model_run_cv;
  Status run_status = SUCCESS;
  const auto callback = [&](Status status, std::vector<Tensor> &outputs) {
    std::unique_lock<std::mutex> lock(run_mutex);
    run_status = status;
    if (check_output) {
      EXPECT_EQ(outputs.size(), output_num);
    }
    model_run_cv.notify_one();
  };

  GEThreadLocalContext context;
  error_message::Context error_context;
  graph_node->Lock();
  RunArgs run_args{graph_node, graph_id, 2001, error_context, input_tensors, flow_root_model, context, callback};
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  std::unique_lock<std::mutex> lock(run_mutex);
  EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  return run_status;
}

TEST_F(DynamicStackExecuteTest, normal_dynamic_stack_execute) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildNormalStackGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);
  graph->SetGraphUnknownFlag(true);

  GeModelPtr ge_model;
  TBEKernelStore tbe_kernel_store;
  BuildNormalStackModel(graph, mem_offset, ge_model, tbe_kernel_store);
  EXPECT_NE(ge_model, nullptr);
  EXPECT_EQ(DynamicStackExecute(graph, ge_model, 3, 2), SUCCESS);
}

TEST_F(DynamicStackExecuteTest, dynamic_graph_with_two_stack_execute) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildGraphWithTwoStack(graph, mem_offset);
  EXPECT_NE(graph, nullptr);
  graph->SetGraphUnknownFlag(true);

  GeModelPtr ge_model;
  TBEKernelStore tbe_kernel_store;
  BuildModelForTwoStackGraph(graph, mem_offset, ge_model, tbe_kernel_store);
  EXPECT_NE(ge_model, nullptr);
  EXPECT_EQ(DynamicStackExecute(graph, ge_model, 2, 3), SUCCESS);
}

TEST_F(DynamicStackExecuteTest, dynamic_stack_graph_with_execption_execute) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildStackGraphWithExeception(graph, mem_offset);
  EXPECT_NE(graph, nullptr);
  graph->SetGraphUnknownFlag(true);

  GeModelPtr ge_model;
  TBEKernelStore tbe_kernel_store;
  BuildNormalStackModel(graph, mem_offset, ge_model, tbe_kernel_store);
  EXPECT_NE(ge_model, nullptr);
  EXPECT_EQ(DynamicStackExecute(graph, ge_model, 3, 2, false), INTERNAL_ERROR);
}
} // namespace ge

