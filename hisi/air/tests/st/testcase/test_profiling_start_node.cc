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

#include "common/profiling/profiling_properties.h"
#include "external/ge/ge_api.h"
#include "framework/common/types.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "graph/compute_graph.h"
#include "graph/execute/model_executor.h"
#include "graph/ge_local_context.h"
#include "graph/op_desc.h"
#include "test_tools_task_info.h"

namespace ge {
class ProfilingStartNodeTest : public testing::Test {
  void SetUp() { ProfilingProperties::Instance().SetTrainingTrace(true); }
  void TearDown() { ProfilingProperties::Instance().SetTrainingTrace(false); }
};
static void BuildAddGraph(ComputeGraphPtr &graph) {
  const auto SetUnknownOpKernel = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    static uint32_t index = 0U;
    const static std::set<std::string> kGeLocalTypes{DATA, CONSTANT, VARIABLE, FILECONSTANT, NETOUTPUT, AIPP_DATA_TYPE};

    GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_FLOAT);
    TensorUtils::SetSize(tensor, 64);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      (void)AttrUtils::SetBool(op_desc, "OwnerGraphIsUnknown", true);
      if (op_desc->GetType() == ADD) {
        op_desc->SetOpKernelLibName("AIcoreEngine");
      } else {
        std::string op_kernel_name =
            (kGeLocalTypes.count(op_desc->GetType()) > 0U) ? "DNN_VM_GE_LOCAL_OP_STORE" : "DNN_VM_RTS_OP_STORE";
        op_desc->SetOpKernelLibName(op_kernel_name);
      }

      vector<int64_t> output_offset;
      for (size_t i = 0U; i < op_desc->GetOutputsSize(); ++i) {
        op_desc->UpdateOutputDesc(i, tensor);
        output_offset.emplace_back(index * 64);
        ++index;
      }
      op_desc->SetOutputOffset(output_offset);
      op_desc->SetWorkspace({});
      op_desc->SetWorkspaceBytes({});
    }

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      vector<int64_t> input_offset;
      for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
        op_desc->UpdateInputDesc(i, tensor);
        if (node->GetType() == NETOUTPUT && node->GetName() != NODE_NAME_NET_OUTPUT) {
          AttrUtils::SetInt(op_desc->MutableInputDesc(i), ATTR_NAME_PARENT_NODE_INDEX, i);
        }
        if (node->GetType() == NETOUTPUT && node->GetName() == NODE_NAME_NET_OUTPUT) {
          op_desc->SetSrcName({"add"});
          op_desc->SetSrcIndex({0});
        }

        const auto in_anchor = node->GetInDataAnchor(i);
        if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr) {
          input_offset.emplace_back(-1);
          continue;
        }

        const auto out_anchor = in_anchor->GetPeerOutAnchor();
        const auto peer_node = out_anchor->GetOwnerNode();
        const vector<int64_t> output_offset = peer_node->GetOpDesc()->GetOutputOffset();
        if (static_cast<size_t>(out_anchor->GetIdx()) >= output_offset.size()) {
          input_offset.emplace_back(-1);
          continue;
        }

        input_offset.emplace_back(output_offset.at(out_anchor->GetIdx()));
      }
      op_desc->SetInputOffset(input_offset);
    }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    CHAIN(NODE("start", STARTOFSEQUENCE)->CTRL_EDGE()->NODE("data1", DATA));
    CHAIN(NODE("start", STARTOFSEQUENCE)->CTRL_EDGE()->NODE("data2", DATA));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("add", ADD));
    CHAIN(NODE("data2", DATA)->EDGE(0, 1)->NODE("add", ADD));
    CHAIN(NODE("add", ADD)->EDGE(0, 0)->NODE("Node_Output", NETOUTPUT));
  };

  graph = ToComputeGraph(g1);
  graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(graph->GetDirectNode());
}

static void BuildAddGraphModel(ComputeGraphPtr &graph, GeModelPtr &ge_model, TBEKernelStore &tbe_kernel_store) {
  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef_TE(graph, *model_task_def, "add", tbe_kernel_store);

  InitEventTaskDef(graph, *model_task_def);
  InitFusionTaskDef(graph, *model_task_def);
  InitEndGraphDef(graph, *model_task_def, "Node_Output");

  InitProfilerTaskDef(graph, *model_task_def);
  InitModelExitTaskDef(graph, *model_task_def);

  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));

  std::vector<uint64_t> weights_value(100, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
}

TEST_F(ProfilingStartNodeTest, test_build_graph_with_profiling_success) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", DATA)->EDGE(0, 0)->NODE("add_1", ADD));
    CHAIN(NODE("data_2", DATA)->EDGE(0, 1)->NODE("add_1", ADD));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  options[GRAPH_MEMORY_MAX_SIZE] = "1073741824";
  options[VARIABLE_MEMORY_MAX_SIZE] = "1073741824";
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  Status ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    for (auto node : graph->GetAllNodes()) {
      if (node->GetType() == DATA) {
        auto in_node_before_dst_node = node->GetInControlAnchor()->GetPeerOutControlAnchors().at(0)->GetOwnerNode();
        EXPECT_NE(in_node_before_dst_node, nullptr);
        EXPECT_EQ(in_node_before_dst_node->GetType(), STARTOFSEQUENCE);
      }
    }
  };
}

TEST_F(ProfilingStartNodeTest, test_execute_graph_with_profiling_success) {
  ComputeGraphPtr graph;
  BuildAddGraph(graph);

  GeModelPtr ge_model;
  TBEKernelStore tbe_kernel_store;
  BuildAddGraphModel(graph, ge_model, tbe_kernel_store);
  EXPECT_NE(ge_model, nullptr);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  // Test for Execute.
  GeTensor input0, input1;
  std::vector<GeTensor> inputs{input0, input1};
  std::vector<GeTensor> outputs;
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}
}  // namespace ge
