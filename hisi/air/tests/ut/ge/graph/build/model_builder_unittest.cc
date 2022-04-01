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

#define protected public
#define private public
#include "ge_graph_dsl/graph_dsl.h"
#include "common/local_context.h"
#include "graph/anchor.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "omg/omg_inner_types.h"
#include "../passes/graph_builder_utils.h"
#include "graph/build/model_builder.h"
#include "memory/memory_assigner.h"
#include "graph/build/label_allocator.h"
#include "graph/utils/tensor_adapter.h"
#include "inc/pass_manager.h"
#include "ir_build/option_utils.h"
#include "common/local_context.h"
#include "common/omg_util.h"
#include "common/formats/utils/formats_trans_utils.h"
#include "../passes/graph_builder_utils.h"
#include "register/custom_pass_helper.h"
#include "graph/ops_stub.h"
#include "ge_attr_value.h"
#include "graph/manager/graph_context.h"
#include "graph/optimize/graph_optimize.h"
#include "generator/ge_generator.h"
#include "graph/utils/tensor_utils.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "init/gelib.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
using namespace ge;
using domi::GetContext;
using namespace domi;

namespace ge {
const char *const kKernelLibName = "DNN_VM_GE_LOCAL";
class UtestModelBuilderTest : public testing::Test {
 public:
  ge::OpDescPtr CreateOpWithWsSize(const string &name, int64_t wsByte, const string &type = "some") {
    ge::OpDescPtr op_def = std::make_shared<ge::OpDesc>(name, type);
    auto desc_temp_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_temp = *desc_temp_ptr;

    TensorUtils::SetSize(desc_temp, 1024);
    op_def->AddInputDesc(desc_temp);
    op_def->AddOutputDesc(desc_temp);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(wsByte);
    op_def->SetWorkspaceBytes(workspace_bytes);
    return op_def;
  }
  ge::OpDescPtr CreateRefOpWithWsSize(const string &name, int64_t wsByte, const string &type = "some") {
    ge::OpDescPtr op_def = std::make_shared<ge::OpDesc>(name, type);
    auto desc_temp_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_temp = *desc_temp_ptr;

    TensorUtils::SetSize(desc_temp, 1024);
    op_def->AddInputDesc(desc_temp);

    auto desc_output_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_output = *desc_output_ptr;
    TensorUtils::SetSize(desc_output, 6500);
    ge::TensorUtils::SetReuseInput(desc_output, true);
    ge::TensorUtils::SetReuseInputIndex(desc_output, 0);
    op_def->AddOutputDesc(desc_output);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(wsByte);
    op_def->SetWorkspaceBytes(workspace_bytes);
    return op_def;
  }
  void MakeGraph(ge::ComputeGraphPtr &graph) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 6000);
    op_def_a->SetStreamId(0);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 120000);
    op_def_b->SetStreamId(0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 16000);
    op_def_c->SetStreamId(1);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 24000);
    op_def_d->SetStreamId(2);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 24000);
    op_def_e->SetStreamId(3);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 30000);
    op_def_f->SetStreamId(2);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 32000);
    op_def_g->SetStreamId(3);
    ge::OpDescPtr op_def_h = CreateOpWithWsSize("H", 48000);
    op_def_h->SetStreamId(2);
    ge::OpDescPtr op_def_i = CreateOpWithWsSize("I", 60000);
    op_def_i->SetStreamId(2);
    ge::OpDescPtr op_def_j = CreateOpWithWsSize("J", 256000, NETOUTPUT);
    op_def_j->SetStreamId(3);

    // add node
    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);
    ge::NodePtr node_h = graph->AddNode(op_def_h);
    ge::NodePtr node_i = graph->AddNode(op_def_i);
    ge::NodePtr node_j = graph->AddNode(op_def_j);

    // add edge
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_g->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_h->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_g->GetOutDataAnchor(0), node_j->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_h->GetOutDataAnchor(0), node_i->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_i->GetOutDataAnchor(0), node_j->GetInDataAnchor(1));

    GetContext().out_nodes_map["H"] = {0};
    GetContext().out_nodes_map["I"] = {0};
    GetContext().out_nodes_map["J"] = {0};
    graph->TopologicalSorting();
  }

void MakeSessionScopeReuseGraph(ge::ComputeGraphPtr graph) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 512);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 512);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 512);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 1024);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 512);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 0);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(1024);
    workspace_bytes.push_back(512);
    op_def_c->SetWorkspaceBytes(workspace_bytes);
    vector<int32_t> workspace_no_reuse_scope = { 0 , 1 };
    (void)ge::AttrUtils::SetListInt(op_def_c, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope);

    vector<int32_t> workspace_no_reuse_scope_e = { 1 };
    (void)ge::AttrUtils::SetListInt(op_def_e, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope_e);

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    graph->TopologicalSorting();
  }

 protected:
  void SetUp() {
    SetLocalOmgContext(domi::GetContext());
  }

  void TearDown() { GetContext().out_nodes_map.clear(); }

  class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
   public:
    FakeOpsKernelInfoStore(){supported_ = true;};
    bool supported_;

   private:
    Status Initialize(const std::map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
      return supported_;
    };
    void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override {};
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

    OpsKernelInfoStorePtr ops_kernel_info_store_ptr = std::make_shared<FakeOpsKernelInfoStore>();
    OpsKernelManager::GetInstance().ops_kernel_store_.emplace(kKernelLibName, ops_kernel_info_store_ptr);
    OpsKernelBuilderPtr fake_builder = std::make_shared<FakeOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
    OpInfo op_info;
    op_info.engine = kKernelLibName;
    op_info.opKernelLib = kKernelLibName;
    OpsKernelManager &ops_kernel_manager = instance_ptr->OpsKernelManagerObj();
    ops_kernel_manager.ops_kernel_info_[DATA].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[ADD].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[NETOUTPUT].emplace_back(op_info);
  }

  void FinalizeGeLib() {
    auto instance_ptr = ge::GELib::GetInstance();
    if (instance_ptr != nullptr) {
      instance_ptr->Finalize();
    }
  }
};

// when check GetMemoryRanges return fail, Assign return fail
TEST_F(UtestModelBuilderTest, SetInputIsConst) {
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph);
  graph->TopologicalSorting();
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);
  EXPECT_EQ(builder.PreBuildModel(), SUCCESS);
}

TEST_F(UtestModelBuilderTest,test_sava_atomic_workspace) {
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);

  auto atomic_op_desc = std::make_shared<OpDesc>("Atomic", "Atomic");
  auto op_desc=std::make_shared<OpDesc>("Sum", "Sum");

  map<string, map<int64_t, int64_t>> workspace_info;
  map<int64_t, int64_t> workspace_info_pair;
  workspace_info_pair.insert(std::make_pair(1, 1));
  workspace_info.insert(std::make_pair("1", workspace_info_pair));
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO,workspace_info);
  EXPECT_EQ(builder.SavaAtomicWorkspace(op_desc), SUCCESS);
}

TEST_F(UtestModelBuilderTest, test_save_atomic_bin) {
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);

  auto atomic_op_desc = std::make_shared<OpDesc>("Atomic", "Atomic");
  auto kernel_buffer = static_cast<Buffer>(Buffer(10));
  AttrUtils::SetStr(atomic_op_desc, ATTR_NAME_TBE_KERNEL_NAME, "Atomic");
  AttrUtils::SetBytes(atomic_op_desc, ATTR_NAME_TBE_KERNEL_BUFFER, kernel_buffer);

  ge::NodePtr atomic_node = graph->AddNode(atomic_op_desc);
  auto op_desc = std::make_shared<OpDesc>("Sum", "Sum");
  op_desc->SetExtAttr("atomic_clean_node_ptr", atomic_node);
  EXPECT_EQ(builder.SaveAtomicTBEKernel(op_desc), SUCCESS);
}

TEST_F(UtestModelBuilderTest, build_model_for_get_task) {
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeSessionScopeReuseGraph(graph);
  std::map<std::string, std::string> option;
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);

  MemoryAssigner mem_assigner(graph);
  EXPECT_EQ(mem_assigner.AssignMemory(builder.mem_type_to_mem_offset_, builder.zero_copy_mem_size_), SUCCESS);

  ge::Model model;
  EXPECT_EQ(builder.BuildModelDef(model), SUCCESS);
  int64_t session_scope_mem_offset = 0;
  ge::AttrUtils::GetInt(&model, ATTR_MODEL_SESSION_SCOPE_MEMORY_SIZE, session_scope_mem_offset);
  EXPECT_EQ(session_scope_mem_offset, 1536);
}

TEST_F(UtestModelBuilderTest, test_model_save) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("relu", RELU)->NODE("conv2d", CONV2D)->NODE("PartitionedCall_0"));
    CHAIN(NODE("_arg_2", DATA)->NODE("framework", FRAMEWORKOP)->NODE("PartitionedCall_0"));
  };
  const ComputeGraphPtr graph = ToComputeGraph(g1);
  const auto call_node = graph->FindNode("PartitionedCall_0");
  EXPECT_NE(call_node, nullptr);
  const auto call_desc = call_node->GetOpDesc();
  const auto relu_node = graph->FindNode("relu");
  EXPECT_NE(relu_node, nullptr);
  const auto relu_desc = relu_node->GetOpDesc();
  const auto conv_node = graph->FindNode("conv2d");
  EXPECT_NE(conv_node, nullptr);
  const auto conv_desc = conv_node->GetOpDesc();
  const auto cust_node = graph->FindNode("framework");
  EXPECT_NE(cust_node, nullptr);
  const auto cust_desc = cust_node->GetOpDesc();

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/_arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt/_arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr sgt_graph = ToComputeGraph(g2);
  const auto ffts_node = sgt_graph->FindNode("sgt/conv2d");
  EXPECT_NE(ffts_node, nullptr);
  const auto ffts_desc = ffts_node->GetOpDesc();

  sgt_graph->SetParentNode(call_node);
  sgt_graph->SetParentGraph(call_node->GetOwnerComputeGraph());
  call_node->GetOpDesc()->AddSubgraphName("f");
  call_node->GetOpDesc()->SetSubgraphInstanceName(0, sgt_graph->GetName());
  graph->AddSubgraph(sgt_graph);
  graph->SetGraphUnknownFlag(true);
  EXPECT_TRUE(AttrUtils::SetBool(call_desc, ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true));

  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);

  size_t tbe_kernel_len = 0U;
  size_t cpu_kernel_len = 0U;
  const Buffer kernel_buffer(20U, 0x11U);
  (void) AttrUtils::SetStr(relu_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, std::string("MIX_AIC"));
  AttrUtils::SetStr(relu_desc, std::string("_mix_aic") + ATTR_NAME_TBE_KERNEL_NAME, "relu_aic");
  AttrUtils::SetBytes(relu_desc, std::string("_mix_aic") + ATTR_NAME_TBE_KERNEL_BUFFER, kernel_buffer);
  tbe_kernel_len += (sizeof(KernelStoreItemHead) + std::string("relu_aic").length() + kernel_buffer.size());
  AttrUtils::SetStr(relu_desc, std::string("_mix_aiv") + ATTR_NAME_TBE_KERNEL_NAME, "relu_aiv");
  AttrUtils::SetBytes(relu_desc, std::string("_mix_aiv") + ATTR_NAME_TBE_KERNEL_BUFFER, kernel_buffer);
  tbe_kernel_len += (sizeof(KernelStoreItemHead) + std::string("relu_aiv").length() + kernel_buffer.size());

  AttrUtils::SetStr(conv_desc, ATTR_NAME_TBE_KERNEL_NAME, "conv2d");
  AttrUtils::SetBytes(conv_desc, ATTR_NAME_TBE_KERNEL_BUFFER, kernel_buffer);
  tbe_kernel_len += (sizeof(KernelStoreItemHead) + std::string("conv2d").length() + kernel_buffer.size());

  AttrUtils::SetStr(ffts_desc, ATTR_NAME_TBE_KERNEL_NAME, "sgt/conv2d");
  AttrUtils::SetBytes(ffts_desc, ATTR_NAME_TBE_KERNEL_BUFFER, kernel_buffer);
  tbe_kernel_len += (sizeof(KernelStoreItemHead) + std::string("sgt/conv2d").length() + kernel_buffer.size());

  std::vector<char> data(20U, 0x16U);
  const auto cust_kernel = MakeShared<OpKernelBin>(cust_desc->GetName(), std::move(data));
  cust_desc->SetExtAttr(OP_EXTATTR_CUSTAICPU_KERNEL, cust_kernel);
  cpu_kernel_len += (sizeof(KernelStoreItemHead) + cust_desc->GetName().length() + 20U);

  ge::Model ge_model;
  ge::GeModel ge_gemodel;
  Buffer serial_buff(64U, 0x12U);
  EXPECT_TRUE(AttrUtils::SetZeroCopyBytes(ge_model, MODEL_ATTR_TASKS, std::move(serial_buff)));
  builder.SaveDataToModel(ge_model, ge_gemodel);
  EXPECT_NE(conv_desc->TryGetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, TBEKernelPtr()), nullptr);
  EXPECT_NE(ffts_desc->TryGetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, TBEKernelPtr()), nullptr);
  EXPECT_NE(relu_desc->TryGetExtAttr(std::string("_mix_aic") + OP_EXTATTR_NAME_TBE_KERNEL, TBEKernelPtr()), nullptr);
  EXPECT_NE(relu_desc->TryGetExtAttr(std::string("_mix_aiv") + OP_EXTATTR_NAME_TBE_KERNEL, TBEKernelPtr()), nullptr);

  EXPECT_TRUE(builder.tbe_kernel_store_.kernels_.empty());
  EXPECT_TRUE(ge_gemodel.tbe_kernal_store_.kernels_.empty());
  EXPECT_EQ(ge_gemodel.tbe_kernal_store_.buffer_.size(), tbe_kernel_len); // aic_relu + aiv_relu + conv2d + sgt/conv2d

  EXPECT_TRUE(builder.cust_aicpu_kernel_store_.kernels_.empty());
  EXPECT_TRUE(ge_gemodel.cust_aicpu_kernal_store_.kernels_.empty());
  EXPECT_EQ(ge_gemodel.cust_aicpu_kernal_store_.buffer_.size(), cpu_kernel_len); // framework
}

TEST_F(UtestModelBuilderTest, CompileSingleOp) {
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("testmix");
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);

  EXPECT_NE(builder.CompileSingleOp(), SUCCESS);
}

TEST_F(UtestModelBuilderTest, CompileSingleOpGe) {
  InitGeLib();
  Graph2SubGraphInfoList subgraphs;
  std::map<std::string, int> stream_max_parallel_num;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("testmix");
  auto op_desc = std::make_shared<OpDesc>("aac", "AtomicAddrClean");
  ge::NodePtr node = graph->AddNode(op_desc);
  ge::ModelBuilder builder(0, graph, subgraphs, stream_max_parallel_num, false);


  EXPECT_NE(builder.CompileSingleOp(), SUCCESS);
  FinalizeGeLib();
}

} // namespace ge