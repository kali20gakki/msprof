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
#include "ge_graph_dsl/graph_dsl.h"

#define private public
#define protected public
#include "graph/build/graph_builder.h"
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "register/ops_kernel_builder_registry.h"
#undef private
#undef protected

#include "graph/passes/graph_builder_utils.h"

namespace ge {
namespace {
const char *const kKernelLibName = "ops_kernel_lib";
const char *kSessionId = "123456";
}  // namespace

class GraphBuilderTest : public testing::Test {
 protected:
  void SetUp() override {
    InitGeLib();
  }
  void TearDown() override {
    FinalizeGeLib();
  }
  class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
   public:
    FakeOpsKernelInfoStore() = default;

   private:
    Status Initialize(const std::map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
      return true;
    };
    void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override{};
  };

  class FakeOpsKernelBuilder : public OpsKernelBuilder {
   public:
    FakeOpsKernelBuilder() = default;

   private:
    Status Initialize(const map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    Status CalcOpRunningParam(Node &node) override {
      return SUCCESS;
    };
    Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) override {
      domi::TaskDef task_def;
      tasks.push_back(task_def);
      return SUCCESS;
    };
  };

  class FakeDNNEngine : public DNNEngine {
   public:
    FakeDNNEngine() = default;
    ~FakeDNNEngine() override = default;
  };

  class FakeGraphOptimizer : public GraphOptimizer {
   public:
    FakeGraphOptimizer() = default;
    Status Initialize(const std::map<std::string, std::string> &options,
                      OptimizeUtility *const optimize_utility) override {
      return SUCCESS;
    }
    Status Finalize() override {
      return SUCCESS;
    }
    Status OptimizeOriginalGraph(ComputeGraph &graph) override {
      return SUCCESS;
    }
    Status OptimizeFusedGraph(ComputeGraph &graph) override {
      return SUCCESS;
    }
    Status OptimizeWholeGraph(ComputeGraph &graph) override {
      return SUCCESS;
    }
    Status GetAttributes(GraphOptimizerAttribute &attrs) const override {
      return SUCCESS;
    }
  };

  void InitGeLib() {
    map<string, string> options;
    Status ret = ge::GELib::Initialize(options);
    EXPECT_EQ(ret, SUCCESS);
    auto instance_ptr = ge::GELib::GetInstance();
    EXPECT_NE(instance_ptr, nullptr);

    //  SchedulerConf conf;
    SchedulerConf scheduler_conf;
    scheduler_conf.name = kKernelLibName;
    scheduler_conf.cal_engines[kKernelLibName] = std::make_shared<EngineConf>();
    scheduler_conf.cal_engines[kKernelLibName]->name = kKernelLibName;
    scheduler_conf.cal_engines[kKernelLibName]->scheduler_id = kKernelLibName;
    map<string, SchedulerConf> scheduler_confs;
    scheduler_confs["scheduler"] = scheduler_conf;
    instance_ptr->DNNEngineManagerObj().schedulers_[kKernelLibName] = scheduler_conf;
    auto engine = std::make_shared<FakeDNNEngine>();
    instance_ptr->DNNEngineManagerObj().engines_map_[kKernelLibName] = engine;

    OpsKernelInfoStorePtr ops_kernel_info_store_ptr = std::make_shared<FakeOpsKernelInfoStore>();
    OpsKernelManager::GetInstance().ops_kernel_store_.emplace(kKernelLibName, ops_kernel_info_store_ptr);
    std::vector<OpInfo> op_infos;
    OpInfo op_info = {kKernelLibName, kKernelLibName, 1, false, false, false, "", ""};
    op_infos.emplace_back(op_info);
    std::vector<std::string> stub_optyes{SEND, RECV};
    for (const auto &op_type : stub_optyes) {
      OpsKernelManager::GetInstance().ops_kernel_info_.emplace(op_type, op_infos);
    }
    auto graph_optimizer = std::make_shared<FakeGraphOptimizer>();
    OpsKernelManager::GetInstance().graph_optimizers_.emplace(kKernelLibName, graph_optimizer);
    OpsKernelBuilderPtr fake_builder = std::make_shared<FakeOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
  }

  void FinalizeGeLib() {
    auto instance_ptr = ge::GELib::GetInstance();
    if (instance_ptr != nullptr) {
      instance_ptr->Finalize();
    }
  }

  NodePtr AddNode(ut::GraphBuilder &builder, const string &name, const string &type, int in_cnt, int out_cnt) {
    auto node = builder.AddNode(name, type, in_cnt, out_cnt);
    auto op_desc = node->GetOpDesc();
    op_desc->SetInputOffset(std::vector<int64_t>(in_cnt));
    op_desc->SetOutputOffset(std::vector<int64_t>(out_cnt));
    return node;
  }

  void SetSubGraph(const NodePtr &node, const string &name, bool is_dynamic_subgraph = false,
                   bool has_memory_type = false) {
    auto op_desc = node->GetOpDesc();
    auto builder = ut::GraphBuilder(name);

    int num_inputs = static_cast<int>(op_desc->GetInputsSize());
    int num_outputs = static_cast<int>(op_desc->GetOutputsSize());
    auto some_node = AddNode(builder, name + "_node", ADDN, num_inputs, 1);
    if (has_memory_type) {  // kSubgraphData
      (void)AttrUtils::SetInt(some_node->GetOpDesc(), ATTR_INPUT_MEMORY_TYPE, 0);
    }
    for (int i = 0; i < num_inputs; ++i) {
      auto data_node = AddNode(builder, name + "_data_" + std::to_string(i), DATA, 1, 1);
      AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_INDEX, i);
      AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, i);
      builder.AddDataEdge(data_node, 0, some_node, i);
    }
    auto net_output = AddNode(builder, "Node_Output", NETOUTPUT, num_outputs, num_outputs);
    for (int i = 0; i < num_outputs; ++i) {
      builder.AddDataEdge(some_node, 0, net_output, i);
      AttrUtils::SetInt(net_output->GetOpDesc()->MutableInputDesc(i), ATTR_NAME_PARENT_NODE_INDEX, i);
    }

    auto subgraph = builder.GetGraph();
    AttrUtils::SetStr(*subgraph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
    subgraph->SetGraphUnknownFlag(is_dynamic_subgraph);
    subgraph->SetParentNode(node);
    op_desc->AddSubgraphName(name);
    op_desc->SetSubgraphInstanceName(0, name);
    auto root_graph = GraphUtils::FindRootGraph(node->GetOwnerComputeGraph());
    subgraph->SetParentGraph(root_graph);
    root_graph->AddSubgraph(name, subgraph);
  }

  ComputeGraphPtr BuildCompoundGraph(bool is_dynamic_subgraph = false, bool has_memory_type = false) {
    auto builder = ut::GraphBuilder("g1");
    auto data1 = AddNode(builder, "data1", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
    if (has_memory_type) {  // kOthers
      (void)AttrUtils::SetInt(data1->GetOpDesc(), ATTR_INPUT_MEMORY_TYPE, 0);
    }
    auto data2 = AddNode(builder, "data2", DATA, 1, 1);
    AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
    auto partitioned_call_1 = AddNode(builder, "PartitionedCall1", PARTITIONEDCALL, 1, 1);
    auto partitioned_call_2 = AddNode(builder, "PartitionedCall2", PARTITIONEDCALL, 1, 1);
    auto partitioned_call_3 = AddNode(builder, "PartitionedCall3", PARTITIONEDCALL, 2, 1);
    auto net_output = AddNode(builder, "NetOutput", NETOUTPUT, 1, 1);
    if (has_memory_type) {  // kSubgraphNode
      (void)AttrUtils::SetInt(net_output->GetOpDesc(), ATTR_INPUT_MEMORY_TYPE, 0);
    }
    SetSubGraph(partitioned_call_1, "subgraph-1", is_dynamic_subgraph, has_memory_type);
    SetSubGraph(partitioned_call_2, "subgraph-2");
    SetSubGraph(partitioned_call_3, "subgraph-3");

    builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
    builder.AddDataEdge(data2, 0, partitioned_call_2, 0);
    builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_3, 0);
    builder.AddDataEdge(partitioned_call_2, 0, partitioned_call_3, 1);
    builder.AddDataEdge(partitioned_call_3, 0, net_output, 0);

    auto root_graph = builder.GetGraph();
    for (const auto &node : root_graph->GetAllNodes()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }

    AttrUtils::SetBool(root_graph, "is_compound", true);
    return root_graph;
  }

  ComputeGraphPtr BuildDynamicShapeGraph(bool has_memory_type = false) {
    auto root_graph = BuildCompoundGraph(true, has_memory_type);
    AttrUtils::SetBool(root_graph, "is_compound", false);
    AttrUtils::SetBool(root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, true);
    return root_graph;
  }

  ComputeGraphPtr BuildDynamicShapeGraphWithMemoryType() {
    auto root_graph = BuildDynamicShapeGraph(true);
    return root_graph;
  }

  ComputeGraphPtr BuildStaticShapeGraphWithMemoryType() {
    auto root_graph = BuildCompoundGraph(false, true);
    AttrUtils::SetBool(root_graph, "is_compound", false);
    return root_graph;
  }

  ComputeGraphPtr BuildGraphWithConst(bool with_weight = true) {
    DEF_GRAPH(g1) {
      CHAIN(NODE("const1", CONSTANT)->NODE("relu1", RELU)->NODE("netoutput", NETOUTPUT));
    };
    auto root_graph = ToComputeGraph(g1);
    // somehow, GeRunningEnvFaker does not work well
    for (const auto &node : root_graph->GetAllNodes()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }
    if (with_weight) {
      auto const_node = root_graph->FindNode("const1");
      EXPECT_NE(const_node, nullptr);
      float_t weight[1 * 1 * 224 * 224] = {0};
      GeTensorDesc weight_desc(GeShape({1, 1, 224, 224}), FORMAT_NCHW, DT_FLOAT);
      GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
      OpDescUtils::SetWeights(const_node, {tensor});
    }
    return root_graph;
  }

  ComputeGraphPtr BuildGraphWithLabels() {
    DEF_GRAPH(g1) {
      auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_STREAM_LABEL, "0");
      auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_STREAM_LABEL, "1");
      auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_STREAM_LABEL, "2");
      auto conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_STREAM_LABEL, "3");
      auto relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_STREAM_LABEL, "4");
      auto add_0 = OP_CFG(ADD).Attr(ATTR_NAME_STREAM_LABEL, "5");
      CHAIN(NODE("_arg_0", data_0)
                ->EDGE(0, 0)
                ->NODE("Conv2D", conv_0)
                ->EDGE(0, 0)
                ->NODE("Relu", relu_0)
                ->EDGE(0, 0)
                ->NODE("Add", add_0)
                ->EDGE(0, 0)
                ->NODE("Node_Output", NETOUTPUT));
      CHAIN(NODE("_arg_1", data_1)->EDGE(0, 1)->NODE("Conv2D", conv_0));
      CHAIN(NODE("_arg_2", data_2)->EDGE(0, 1)->NODE("Add", add_0));
    };
    auto root_graph = ToComputeGraph(g1);
    // somehow, GeRunningEnvFaker does not work well
    for (const auto &node : root_graph->GetAllNodes()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }
    (void)ge::AttrUtils::SetInt(*root_graph, ATTR_MODEL_LABEL_NUM, 6);
    return root_graph;
  }
};

TEST_F(GraphBuilderTest, CalcOpParam_failed) {
  GraphBuilder graph_builder;
  auto root_graph = BuildGraphWithLabels();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  FinalizeGeLib();
  auto ret = graph_builder.CalcOpParam(root_graph);
  EXPECT_NE(ret, SUCCESS);
  InitGeLib();
  for (const auto &node : root_graph->GetAllNodes()) {
    node->GetOpDesc()->SetOpKernelLibName("");
    node->GetOpDesc()->SetOpEngineName("");
  }
  ret = graph_builder.CalcOpParam(root_graph);
  EXPECT_NE(ret, SUCCESS);
  FinalizeGeLib();
}

TEST_F(GraphBuilderTest, SetConstantInputOffset) {
  GraphBuilder graph_builder;
  auto root_graph_with_weights = BuildGraphWithConst();
  AttrUtils::SetStr(root_graph_with_weights, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  auto ret = graph_builder.SetConstantInputOffset(root_graph_with_weights);
  EXPECT_EQ(ret, SUCCESS);
  auto root_graph_without_weights = BuildGraphWithConst(false);
  AttrUtils::SetStr(root_graph_without_weights, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  ret = graph_builder.SetConstantInputOffset(root_graph_without_weights);
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_graph_with_label) {
  GraphBuilder graph_builder;
  auto root_graph = BuildGraphWithLabels();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_static_shape_graph_with_const) {
  GraphBuilder graph_builder;
  auto root_graph = BuildGraphWithConst();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_static_shape_graph_with_memory_type) {
  GraphBuilder graph_builder;
  auto root_graph = BuildStaticShapeGraphWithMemoryType();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_dynamic_shape_graph_with_memory_type) {
  GraphBuilder graph_builder;
  auto root_graph = BuildDynamicShapeGraphWithMemoryType();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_dynamic_shape_graph) {
  GraphBuilder graph_builder;
  auto root_graph = BuildDynamicShapeGraph();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphBuilderTest, test_build_for_host_cpu_graph) {
  GraphBuilder graph_builder;
  auto root_graph = BuildDynamicShapeGraph();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  std::map<std::string, std::string> options_map;
  options_map.emplace("ge.exec.placement", "HOST");
  GetThreadLocalContext().SetGraphOption(options_map);
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
  std::map<std::string, std::string> options_map_recovery;
  GetThreadLocalContext().SetGraphOption(options_map_recovery);
}

TEST_F(GraphBuilderTest, TestBuildForCompoundGraph) {
  GraphBuilder graph_builder;
  auto root_graph = BuildCompoundGraph();
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
}

///     NetOutput           NetOutput
///         |                   |
///       PC_1                 If
///      /   \               /   \.
///    |      |            |      |
///  data1  data2        data1  data2
TEST_F(GraphBuilderTest, TestDivideSubgraphFromRootGraph) {
  ComputeGraphPtr root_graph;
  NodePtr partitioned_call_1;
  {
    auto builder = ut::GraphBuilder("g1");
    auto data1 = AddNode(builder, "data1", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
    auto data2 = AddNode(builder, "data2", DATA, 1, 1);
    AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
    partitioned_call_1 = AddNode(builder, "PartitionedCall1", PARTITIONEDCALL, 2, 1);
    auto net_output = AddNode(builder, "NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
    builder.AddDataEdge(data2, 0, partitioned_call_1, 1);
    builder.AddDataEdge(partitioned_call_1, 0, net_output, 0);
    root_graph = builder.GetGraph();
  }

  ComputeGraphPtr subgraph;
  NodePtr if_node;
  {
    auto builder = ut::GraphBuilder("subgraph-1");
    auto data1 = AddNode(builder, "data1", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data2 = AddNode(builder, "data2", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    if_node = AddNode(builder, "IF", IF, 2, 1);
    auto net_output = AddNode(builder, "Subgraph-1-NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data1, 0, if_node, 0);
    builder.AddDataEdge(data2, 0, if_node, 1);
    builder.AddDataEdge(if_node, 0, net_output, 0);
    subgraph = builder.GetGraph();

    subgraph->SetParentNode(partitioned_call_1);
    partitioned_call_1->GetOpDesc()->AddSubgraphName("subgraph-1");
    partitioned_call_1->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph-1");
    subgraph->SetParentGraph(root_graph);
    root_graph->AddSubgraph("subgraph-1", subgraph);
  }

  ComputeGraphPtr then_branch_subgraph;
  {
    auto builder = ut::GraphBuilder("then-subgraph");
    auto data1 = AddNode(builder, "then-data1", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto net_output = AddNode(builder, "then-NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data1, 0, net_output, 0);
    then_branch_subgraph = builder.GetGraph();

    then_branch_subgraph->SetParentNode(if_node);
    if_node->GetOpDesc()->AddSubgraphName("then-subgraph");
    if_node->GetOpDesc()->SetSubgraphInstanceName(0, "then-subgraph");
    then_branch_subgraph->SetParentGraph(subgraph);
    root_graph->AddSubgraph("then-subgraph", then_branch_subgraph);
  }

  ComputeGraphPtr else_branch_subgraph;
  {
    auto builder = ut::GraphBuilder("else-subgraph");
    auto data1 = AddNode(builder, "else-data1", DATA, 1, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto net_output = AddNode(builder, "else-NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data1, 0, net_output, 0);
    else_branch_subgraph = builder.GetGraph();

    else_branch_subgraph->SetParentNode(if_node);
    if_node->GetOpDesc()->AddSubgraphName("else-subgraph");
    if_node->GetOpDesc()->SetSubgraphInstanceName(1, "else-subgraph");
    else_branch_subgraph->SetParentGraph(subgraph);
    root_graph->AddSubgraph("else-subgraph", else_branch_subgraph);
  }

  // check origin graph
  ASSERT_EQ(NodeUtils::GetSubgraph(*if_node, 0), then_branch_subgraph);
  ASSERT_EQ(NodeUtils::GetSubgraph(*if_node, 1), else_branch_subgraph);
  ASSERT_EQ(GraphUtils::FindRootGraph(if_node->GetOwnerComputeGraph()), root_graph);
  ASSERT_EQ(GraphUtils::FindRootGraph(then_branch_subgraph), root_graph);
  ASSERT_EQ(GraphUtils::FindRootGraph(else_branch_subgraph), root_graph);
  ASSERT_EQ(subgraph->GetParentGraph(), root_graph);

  // check result graph
  ASSERT_EQ(GraphBuilder::DivideSubgraphFromRootGraph(root_graph, subgraph), SUCCESS);
  ASSERT_TRUE(subgraph->GetParentGraph() == nullptr);
  ASSERT_EQ(GraphUtils::FindRootGraph(subgraph), subgraph);
  ASSERT_EQ(GraphUtils::FindRootGraph(then_branch_subgraph), subgraph);
  ASSERT_EQ(GraphUtils::FindRootGraph(else_branch_subgraph), subgraph);
  ASSERT_EQ(NodeUtils::GetSubgraph(*if_node, 0), then_branch_subgraph);
  ASSERT_EQ(NodeUtils::GetSubgraph(*if_node, 1), else_branch_subgraph);
}
TEST_F(GraphBuilderTest, TestSpecialInputAndOutputSize) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->NODE("relu1", RELU)->NODE("netoutput", NETOUTPUT));
  };
  auto root_graph = ToComputeGraph(g1);
  // somehow, GeRunningEnvFaker does not work well
  for (const auto &node : root_graph->GetAllNodes()) {
    node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
    node->GetOpDesc()->SetOpEngineName(kKernelLibName);
  }
  int64_t special_size = 224 * 224 * 2 * 8 * 3 / 2;
  auto special_size_node = root_graph->FindNode("relu1");
  EXPECT_NE(special_size_node, nullptr);
  ge::AttrUtils::SetInt(special_size_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_SPECIAL_INPUT_SIZE,
                        special_size);
  ge::AttrUtils::SetInt(special_size_node->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_SPECIAL_OUTPUT_SIZE,
                        special_size);
  AttrUtils::SetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);

  GeRootModelPtr root_model;
  GraphBuilder graph_builder;
  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);
  auto check_size = [&](const NodePtr &node) {
    EXPECT_NE(node, nullptr);
    int64_t size = 0;
    if (node->GetType() == DATA) {
      EXPECT_EQ(ge::AttrUtils::GetInt(node->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_SPECIAL_INPUT_SIZE, size),
                true);
    } else if (node->GetType() == NETOUTPUT) {
      EXPECT_EQ(ge::AttrUtils::GetInt(node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_SPECIAL_OUTPUT_SIZE, size),
                true);
    }
    EXPECT_EQ(size, special_size);
  };
  check_size(root_graph->FindNode("data1"));
  check_size(root_graph->FindNode("netoutput"));
}

}  // namespace ge