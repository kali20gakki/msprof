/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#include <memory>

#include "graph/anchor.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "omg/omg_inner_types.h"
#include "../passes/graph_builder_utils.h"

#define protected public
#define private public
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "register/ops_kernel_builder_registry.h"
#include "graph/build/task_generator.h"
#include "graph/ge_local_context.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/manager/graph_var_manager.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "common/profiling/profiling_properties.h"

using namespace std;
using namespace testing;
using namespace ge;
namespace {
const char *const kIsInputVar = "INPUT_IS_VAR";
const char *const kIsOutputVar = "OUTPUT_IS_VAR";
const char *const kKernelInfoNameHccl = "ops_kernel_info_hccl";
const char *const kKernelLibName = "ops_kernel_lib";
const char *kSessionId = "123456";
const char *const kPartiallySupported = "partially_supported";
const std::set<std::string> kAicpuKernelLibs = {"aicpu_ascend_kernel", "aicpu_tf_kernel"};
const char *const kMixL2Engine = "_ffts_plus_mix_l2";
}  // namespace

class UtestTaskGeneratorTest : public testing::Test {
 public:
  struct FakeOpsKernelBuilder : OpsKernelBuilder {
    FakeOpsKernelBuilder(){};

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
  struct FakeOpsKernelInfoStore : OpsKernelInfoStore {
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

  ge::ComputeGraphPtr BuildGraphFpProfiling() {
    ge::ut::GraphBuilder builder("graph");
    auto data = builder.AddNode("data", "phony", 1, 1);
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
    auto netoutput = builder.AddNode("netoutput", "NetOutput", 2, 0);
    auto op_desc = data->GetOpDesc();
    (void)AttrUtils::SetStr(op_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "IteratorV2");
    op_desc->SetOpKernelLibName("GE");
    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(addn1, 0, netoutput, 0);
    return builder.GetGraph();
  }
  ge::ComputeGraphPtr BuildGraphBpProfiling() {
    ge::ut::GraphBuilder builder("graph");
    auto data = builder.AddNode("data", "phony", 1, 1);
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
    auto netoutput = builder.AddNode("Node_Output", "NetOutput", 2, 0);
    auto data_desc = data->GetOpDesc();
    (void)AttrUtils::SetStr(data_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "IteratorV2");
    data_desc->SetOpKernelLibName("GE");
    auto output_desc = netoutput->GetOpDesc();
    output_desc->SetOpKernelLibName("output");
    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddControlEdge(addn1, netoutput);
    return builder.GetGraph();
  }
  ge::ComputeGraphPtr BuildGraphWithVar(int64_t session_id) {
    // init
    MemManager::Instance().Initialize(std::vector<rtMemType_t>({RT_MEMORY_HBM}));
    VarManager::Instance(session_id)->Init(0, 0, 0, 0);
    ge::ut::GraphBuilder builder("graph");
    auto var_input = builder.AddNode("var", "Variable", 1, 1);
    auto const_input = builder.AddNode("const", "Const", 1, 1);
    auto assign = builder.AddNode("assgin", "Assign", 2, 1);
    // add link
    builder.AddDataEdge(var_input, 0, assign, 0);
    builder.AddDataEdge(const_input, 0, assign, 1);
    // set offset
    var_input->GetOpDesc()->SetOutputOffset({10000});
    const_input->GetOpDesc()->SetOutputOffset({1000});
    assign->GetOpDesc()->SetInputOffset({10100, 1000});
    assign->GetOpDesc()->SetOutputOffset({10100});
    // set inner offset
    int64_t inner_offset = 100;
    ge::AttrUtils::SetInt(assign->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset);
    ge::AttrUtils::SetInt(assign->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset);
    // add var addr
    VarManager::Instance(session_id)->var_resource_->var_offset_map_.emplace(10000, RT_MEMORY_HBM);

    return builder.GetGraph();
  }
  ge::ComputeGraphPtr BuildHcclGraph() {
    ge::ut::GraphBuilder builder("graph");
    auto hccl_node = builder.AddNode("hccl_phony_node", "HCCL_PHONY", 0, 0);
    auto op_desc = hccl_node->GetOpDesc();
    op_desc->SetOpKernelLibName(kKernelInfoNameHccl);
    op_desc->SetStreamId(0);
    return builder.GetGraph();
  }
  ComputeGraphPtr BuildFftsGraph() {
    auto builder = ut::GraphBuilder("g1");
    auto data = builder.AddNode("data", DATA, 1, 1);
    AttrUtils::SetInt(data->GetOpDesc(), ATTR_NAME_INDEX, 0);
    auto partitioned_call = builder.AddNode("PartitionedCall", PARTITIONEDCALL, 1, 1);
    ge::AttrUtils::SetBool(partitioned_call->GetOpDesc(), ATTR_NAME_FFTS_SUB_GRAPH, true);
    auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data, 0, partitioned_call, 0);
    builder.AddDataEdge(partitioned_call, 0, net_output, 0);

    auto sub_builder = ut::GraphBuilder("subgraph");
    auto sub_data = sub_builder.AddNode("data", DATA, 1, 1);
    AttrUtils::SetInt(data->GetOpDesc(), ATTR_NAME_INDEX, 0);
    auto addn = sub_builder.AddNode("addn", ADDN, 1, 1);
    auto sub_net_output = sub_builder.AddNode("NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(sub_data, 0, addn, 0);
    builder.AddDataEdge(addn, 0, sub_net_output, 0);
    auto sub_graph = sub_builder.GetGraph();
    auto root_graph = builder.GetGraph();

    AttrUtils::SetStr(*sub_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
    sub_graph->SetParentNode(partitioned_call);
    partitioned_call->GetOpDesc()->AddSubgraphName("subgraph");
    partitioned_call->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph");
    sub_graph->SetParentGraph(root_graph);
    root_graph->AddSubgraph("subgraph", sub_graph);

    for (const auto &node : root_graph->GetDirectNode()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }

    for (const auto &node : sub_graph->GetDirectNode()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }

    return root_graph;
  }

  ComputeGraphPtr BuildFftsPlusGraph() {
    auto builder = ut::GraphBuilder("g1");
    auto data = builder.AddNode("data", DATA, 1, 1);
    AttrUtils::SetInt(data->GetOpDesc(), ATTR_NAME_INDEX, 0);
    auto partitioned_call = builder.AddNode("PartitionedCall", PARTITIONEDCALL, 1, 1);
    ge::AttrUtils::SetBool(partitioned_call->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);
    auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(data, 0, partitioned_call, 0);
    builder.AddDataEdge(partitioned_call, 0, net_output, 0);

    auto sub_builder = ut::GraphBuilder("subgraph");
    auto sub_data = sub_builder.AddNode("data", DATA, 1, 1);
    AttrUtils::SetInt(data->GetOpDesc(), ATTR_NAME_INDEX, 0);
    auto addn = sub_builder.AddNode("addn", ADDN, 1, 1);
    auto sub_net_output = sub_builder.AddNode("NetOutput", NETOUTPUT, 1, 1);
    builder.AddDataEdge(sub_data, 0, addn, 0);
    builder.AddDataEdge(addn, 0, sub_net_output, 0);
    auto sub_graph = sub_builder.GetGraph();
    auto root_graph = builder.GetGraph();

    AttrUtils::SetStr(*sub_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
    sub_graph->SetParentNode(partitioned_call);
    partitioned_call->GetOpDesc()->AddSubgraphName("subgraph");
    partitioned_call->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph");
    sub_graph->SetParentGraph(root_graph);
    root_graph->AddSubgraph("subgraph", sub_graph);

    for (const auto &node : root_graph->GetDirectNode()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }

    for (const auto &node : sub_graph->GetDirectNode()) {
      node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
      node->GetOpDesc()->SetOpEngineName(kKernelLibName);
    }

    return root_graph;
  }

  ge::ComputeGraphPtr BuildGraphBpProfilingWithGlobalStep() {
    ge::ut::GraphBuilder builder("graph");
    auto data = builder.AddNode("data", "phony", 1, 1);
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
    auto global_step1 = builder.AddNode("ge_global_step", "Variable", 0, 1);
    auto netoutput = builder.AddNode("Node_Output", "NetOutput", 2, 0);
    auto data_desc = data->GetOpDesc();
    (void)AttrUtils::SetStr(data_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "IteratorV2");
    data_desc->SetOpKernelLibName("GE");
    auto output_desc = netoutput->GetOpDesc();
    output_desc->SetOpKernelLibName("output");

    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(global_step1, 0, netoutput, 1);
    builder.AddControlEdge(addn1, netoutput);
    return builder.GetGraph();
  }

  ge::ComputeGraphPtr BuildGraphBpProfilingWithSubGraph() {
    ge::ut::GraphBuilder builder("graph");
    auto data = builder.AddNode("data", "phony", 1, 1);
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
    auto func_node = builder.AddNode("Case", "Case", 1, 1);
    auto global_step1 = builder.AddNode("ge_global_step", "Variable", 0, 1);
    auto netoutput = builder.AddNode("Node_Output", "NetOutput", 2, 0);
    auto data_desc = data->GetOpDesc();
    (void)AttrUtils::SetStr(data_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "IteratorV2");
    data_desc->SetOpKernelLibName("GE");
    auto output_desc = netoutput->GetOpDesc();
    output_desc->SetOpKernelLibName("output");

    std::string subgraph_name_1 = "instance_branch_1";
    ComputeGraphPtr subgraph_1 = std::make_shared<ComputeGraph>(subgraph_name_1);
    subgraph_1->SetParentNode(func_node);
    subgraph_1->SetParentGraph(builder.GetGraph());
    size_t index = func_node->GetOpDesc()->GetSubgraphInstanceNames().size();
    func_node->GetOpDesc()->AddSubgraphName("branch1");
    func_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name_1);

    NodePtr global_step2 = subgraph_1->AddNode(CreateOpDesc("ge_global_step", "Variable", 0, 1));
    NodePtr output_node = subgraph_1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    GraphUtils::AddEdge(global_step2->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));
    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(func_node, 0, netoutput, 1);
    builder.AddControlEdge(addn1, netoutput);
    return builder.GetGraph();
  }
  void Init() {
    map<string, string> options;
    Status ret = ge::GELib::Initialize(options);
    EXPECT_EQ(ret, SUCCESS);

    OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<FakeOpsKernelInfoStore>();
    OpsKernelManager::GetInstance().ops_kernel_store_.insert(make_pair(kKernelLibName, ops_kernel_info_store_ptr));

    OpsKernelBuilderPtr fake_builder = MakeShared<FakeOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
  }
  void Finalize() {
    auto instance_ptr = ge::GELib::GetInstance();
    if (instance_ptr != nullptr) {
      instance_ptr->Finalize();
    }
  }

 protected:
  void SetUp() {
    Init();
  }
  void TearDown() {
    Finalize();
  }
};

namespace {
void ProfilingInit() {
  ProfilingProperties::Instance().is_load_profiling_ = true;
  ProfilingProperties::Instance().is_execute_profiling_ = true;
  ProfilingProperties::Instance().fp_point_ = "Add";
  ProfilingProperties::Instance().bp_point_ = "MatMul";
}

void ProfilingExit() {
  ProfilingProperties::Instance().is_load_profiling_ = false;
  ProfilingProperties::Instance().is_execute_profiling_ = false;
  ProfilingProperties::Instance().fp_point_ = "";
  ProfilingProperties::Instance().bp_point_ = "";
}

void BuildGraphProfiling(ge::ComputeGraphPtr &graph) {
  GeTensorDesc ge_tensor_desc;
  auto data_op = std::make_shared<OpDesc>("Data", DATA);
  data_op->AddInputDesc(ge_tensor_desc);
  data_op->AddOutputDesc(ge_tensor_desc);
  auto data_node = graph->AddNode(data_op);

  auto data_op1 = std::make_shared<OpDesc>("Data", DATA);
  data_op1->AddInputDesc(ge_tensor_desc);
  data_op1->AddOutputDesc(ge_tensor_desc);
  auto data_node1 = graph->AddNode(data_op1);

  auto flatten_op = std::make_shared<OpDesc>("Add", ADD);
  flatten_op->AddInputDesc(ge_tensor_desc);
  flatten_op->AddInputDesc(ge_tensor_desc);
  flatten_op->AddOutputDesc(ge_tensor_desc);
  auto flatten_node = graph->AddNode(flatten_op);

  auto ar1_op = std::make_shared<OpDesc>("ar1", HCOMALLREDUCE);
  ar1_op->AddInputDesc(ge_tensor_desc);
  ar1_op->AddOutputDesc(ge_tensor_desc);
  auto ar1_node = graph->AddNode(ar1_op);

  auto softmax_op = std::make_shared<OpDesc>("MatMul", MATMUL);
  softmax_op->AddInputDesc(ge_tensor_desc);
  softmax_op->AddOutputDesc(ge_tensor_desc);
  auto softmax_node = graph->AddNode(softmax_op);

  auto ar2_op = std::make_shared<OpDesc>("ar2", HCOMALLREDUCE);
  ar2_op->AddInputDesc(ge_tensor_desc);
  ar2_op->AddOutputDesc(ge_tensor_desc);
  auto ar2_node = graph->AddNode(ar2_op);

  auto netouput_op = std::make_shared<OpDesc>(NODE_NAME_NET_OUTPUT, NETOUTPUT);
  auto netouput_node = graph->AddNode(netouput_op);

  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), flatten_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), flatten_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(flatten_node->GetOutDataAnchor(0), ar1_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(ar1_node->GetOutDataAnchor(0), softmax_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(softmax_node->GetOutDataAnchor(0), ar2_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(ar2_node->GetOutDataAnchor(0), netouput_node->GetInDataAnchor(0));
}
void SetGraphNodeKernel(ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetAllNodes()) {
    node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
    node->GetOpDesc()->SetOpEngineName(kKernelLibName);
  }
}

void SetFusiondNode(ComputeGraphPtr &graph) {
  int group_id = 1;
  for (const auto &node : graph->GetAllNodes()) {
    (void)AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_L1_FUSION_GROUP_ID, group_id);
  }
}

void SetPartiallySupportedNode(ComputeGraphPtr &graph) {
  OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<UtestTaskGeneratorTest::FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_.insert(make_pair("aicpu_ascend_kernel", ops_kernel_info_store_ptr));

  OpsKernelBuilderPtr fake_builder = MakeShared<UtestTaskGeneratorTest::FakeOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_["aicpu_ascend_kernel"] = fake_builder;
  for (const auto &node : graph->GetAllNodes()) {
    (void)AttrUtils::SetBool(node->GetOpDesc(), kPartiallySupported, true);
  }
}

void SetMixL2Node(ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetAllNodes()) {
    if (node->GetType() == ADD) {
      (void)AttrUtils::SetStr(node->GetOpDesc(), "_alias_engine_name", kKernelLibName);
    }
  }
}
}  // namespace
TEST_F(UtestTaskGeneratorTest, GetTaskInfo_with_profiling_success) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ComputeGraph>("TestGraph");
  BuildGraphProfiling(compute_graph);
  SetGraphNodeKernel(compute_graph);

  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(this));
  TaskGenerator task_generator(nullptr, 0);
  ge::Model model_def;
  ProfilingInit();
  auto ret = task_generator.GetTaskInfo(model_def, compute_graph, 0, run_context);
  EXPECT_EQ(ret, SUCCESS);
  ProfilingExit();
}

TEST_F(UtestTaskGeneratorTest, GetTaskInfo_with_mix_l2_nodes_success) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ComputeGraph>("TestGraph");
  BuildGraphProfiling(compute_graph);
  SetGraphNodeKernel(compute_graph);
  SetMixL2Node(compute_graph);

  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(this));
  TaskGenerator task_generator(nullptr, 0);
  ge::Model model_def;
  auto ret = task_generator.GetTaskInfo(model_def, compute_graph, 0, run_context);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestTaskGeneratorTest, GetTaskInfo_with_fusion_nodes_success) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ComputeGraph>("TestGraph");
  BuildGraphProfiling(compute_graph);
  SetGraphNodeKernel(compute_graph);
  SetFusiondNode(compute_graph);

  std::string buffer_optimize = "l1_optimized";
  std::map<std::string, std::string> options;
  options[BUFFER_OPTIMIZE] = buffer_optimize;
  GetThreadLocalContext().SetGraphOption(options);
  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(this));
  TaskGenerator task_generator(nullptr, 0);
  ge::Model model_def;
  auto ret = task_generator.GetTaskInfo(model_def, compute_graph, 0, run_context);
  EXPECT_EQ(ret, SUCCESS);

  std::map<std::string, std::string> options_map_recovery;
  GetThreadLocalContext().SetGraphOption(options_map_recovery);
}

TEST_F(UtestTaskGeneratorTest, GetTaskInfo_with_partially_supported_nodes_success) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ComputeGraph>("TestGraph");
  BuildGraphProfiling(compute_graph);
  SetGraphNodeKernel(compute_graph);
  SetPartiallySupportedNode(compute_graph);

  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(this));
  TaskGenerator task_generator(nullptr, 0);
  ge::Model model_def;
  auto ret = task_generator.GetTaskInfo(model_def, compute_graph, 0, run_context);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestTaskGeneratorTest, AutoFindFpOpIndex) {
  auto graph = BuildGraphFpProfiling();
  TaskGenerator task_generator(nullptr, 0);
  ProfilingPoint profiling_point;
  profiling_point.fp_index = -1;
  EXPECT_EQ(task_generator.AutoFindFpOpIndex(graph, profiling_point), SUCCESS);
  // addn1 is fp
  EXPECT_EQ(profiling_point.fp_index, 2);
}

TEST_F(UtestTaskGeneratorTest, FindLastBpFromBpNodeWithSubGraph) {
  auto graph = BuildGraphBpProfilingWithSubGraph();
  TaskGenerator task_generator(nullptr, 0);
  auto net_output = graph->FindNode("Node_Output");
  // netoutput has no data input, return default value 0
  uint32_t bp_index = 0;
  EXPECT_EQ(task_generator.FindLastBpFromBpNode(graph, net_output, bp_index), 0);
}

TEST_F(UtestTaskGeneratorTest, FindLastBpFromBpNode) {
  auto graph = BuildGraphBpProfilingWithGlobalStep();
  TaskGenerator task_generator(nullptr, 0);
  auto net_output = graph->FindNode("Node_Output");
  // netoutput has no data input, return default value 0
  uint32_t bp_index = 0;
  EXPECT_EQ(task_generator.FindLastBpFromBpNode(graph, net_output, bp_index), 0);
}

TEST_F(UtestTaskGeneratorTest, UpdateOpIsVarAttr) {
  int64_t session_id = 0;
  ge::ComputeGraphPtr graph = BuildGraphWithVar(session_id);
  graph->SetSessionID(session_id);
  TaskGenerator task_generator(nullptr, 0);
  auto assign = graph->FindNode("assgin");
  task_generator.UpdateOpIsVarAttr(assign->GetOpDesc(), session_id);
  // input
  vector<bool> input_var;
  AttrUtils::GetListBool(assign->GetOpDesc(), kIsInputVar, input_var);
  EXPECT_EQ(input_var.size(), 2);
  EXPECT_EQ(input_var[0], true);
  EXPECT_EQ(input_var[1], false);
  // output
  vector<bool> output_var;
  AttrUtils::GetListBool(assign->GetOpDesc(), kIsOutputVar, output_var);
  EXPECT_EQ(output_var.size(), 1);
  EXPECT_EQ(output_var[0], true);

  MemManager::Instance().Finalize();
}

TEST_F(UtestTaskGeneratorTest, AutoFindBpOpIndex) {
  auto graph = BuildGraphBpProfiling();
  TaskGenerator task_generator(nullptr, 0);
  auto net_output = graph->FindNode("Node_Output");
  ProfilingPoint profiling_point;
  vector<uint32_t> all_reduce_nodes;
  EXPECT_EQ(task_generator.AutoFindBpOpIndex(graph, profiling_point, all_reduce_nodes), SUCCESS);

  auto output_desc = net_output->GetOpDesc();
  output_desc->SetType("HcomAllReduce");
  output_desc->SetName("hcom");
  EXPECT_EQ(task_generator.AutoFindBpOpIndex(graph, profiling_point, all_reduce_nodes), SUCCESS);

  setenv("PROFILING_MODE", "true", true);
  EXPECT_EQ(task_generator.FindProfilingTaskIndex(graph, profiling_point, all_reduce_nodes), SUCCESS);
  EXPECT_EQ(profiling_point.end_index.size(), 1);
  EXPECT_EQ(*profiling_point.end_index.begin(), 3);
}

TEST_F(UtestTaskGeneratorTest, GenerateTask) {
  map<string, string> options;
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);

  OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_.insert(make_pair(kKernelInfoNameHccl, ops_kernel_info_store_ptr));

  OpsKernelBuilderPtr fake_builder = MakeShared<FakeOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelInfoNameHccl] = fake_builder;

  auto graph = BuildHcclGraph();
  TaskGenerator task_generator(nullptr, 0);
  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(ops_kernel_info_store_ptr.get()));
  vector<uint32_t> all_reduce_nodes;
  vector<domi::TaskDef> task_def_list;
  map<uint32_t, string> op_name_map;

  EXPECT_EQ(task_generator.GenerateTask(run_context, graph, task_def_list, op_name_map), SUCCESS);
  EXPECT_EQ(task_def_list.size(), 1);

  auto node = graph->FindNode("hccl_phony_node");
  EXPECT_TRUE(node != nullptr);
  auto *ops_kernel_info_store =
      node->GetOpDesc()->TryGetExtAttr<OpsKernelInfoStore *>("OpsKernelInfoStorePtr", nullptr);
  EXPECT_EQ(ops_kernel_info_store, ops_kernel_info_store_ptr.get());
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_.erase(kKernelInfoNameHccl);
}

TEST_F(UtestTaskGeneratorTest, SetFpBpByOptions) {
  map<std::string, string> options_map = {
      {OPTION_EXEC_PROFILING_OPTIONS, R"({"fp_point":"fp_node","bp_point":"bp_node"})"}};
  ge::GEThreadLocalContext &context = GetThreadLocalContext();
  context.SetGraphOption(options_map);

  auto graph = BuildGraphBpProfiling();
  TaskGenerator task_generator(nullptr, 0);
  ProfilingPoint profiling_point;
  vector<uint32_t> all_reduce_nodes;
  std::string fp_str;
  std::string bp_str;
  EXPECT_EQ(task_generator.GetFpBpIndex(graph, profiling_point, all_reduce_nodes, fp_str, bp_str), SUCCESS);
  EXPECT_EQ(fp_str, "fp_node");
  EXPECT_EQ(bp_str, "bp_node");
}

TEST_F(UtestTaskGeneratorTest, GenerateFftsTask) {
  map<string, string> options;
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);

  OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_.insert(make_pair(kKernelLibName, ops_kernel_info_store_ptr));

  OpsKernelBuilderPtr fake_builder = MakeShared<FakeOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;

  auto graph = BuildFftsGraph();
  TaskGenerator task_generator(nullptr, 0);
  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(ops_kernel_info_store_ptr.get()));
  vector<domi::TaskDef> task_def_list;
  map<uint32_t, string> op_name_map;

  EXPECT_EQ(task_generator.GenerateTask(run_context, graph, task_def_list, op_name_map), SUCCESS);
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_.erase(kKernelLibName);
}

TEST_F(UtestTaskGeneratorTest, GenerateFftsPlusTask) {
  map<string, string> options;
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);

  OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_.insert(make_pair(kKernelLibName, ops_kernel_info_store_ptr));

  OpsKernelBuilderPtr fake_builder = MakeShared<FakeOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;

  auto graph = BuildFftsPlusGraph();
  TaskGenerator task_generator(nullptr, 0);
  RunContext run_context;
  run_context.graphStreamList.push_back(static_cast<void *>(ops_kernel_info_store_ptr.get()));
  vector<domi::TaskDef> task_def_list;
  map<uint32_t, string> op_name_map;

  EXPECT_EQ(task_generator.GenerateTask(run_context, graph, task_def_list, op_name_map), INTERNAL_ERROR);
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_.erase(kKernelLibName);
}
