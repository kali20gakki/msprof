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

#include <stdio.h>
#include <map>
#include <memory>
#include "gtest/gtest.h"
#include "proto/om.pb.h"

#define protected public
#define private public
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_multi_output_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_reduce_elemwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_segment_elemwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class UB_FUSION_UT_ELT_ELT : public testing::Test {
 public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

 protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }
  static void TearDownTestCase() { std::cout << "UB fusion TearDown" << std::endl; }
  std::shared_ptr<AutomaticBufferFusion> auto_buffer_fusion_ptr_;
  virtual void SetUp() {
    auto_buffer_fusion_ptr_ = std::make_shared<AutomaticBufferFusion>(nullptr);

  }

  virtual void TearDown() {

  }
  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }
  void SetTvmType(ge::OpDescPtr opdef) { ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM)); }
  void BuildGraph(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr addn = std::make_shared<OpDesc>("addn", "AddN");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1","ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2","ReLU");
    SetPattern(addn, "ElemWise");
    SetPattern(elemwise, "ElemWise");

    SetTvmType(addn);
    SetTvmType(elemwise);

    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    relu2->AddInputDesc(out_desc);
    relu2->AddOutputDesc(out_desc);
    AttrUtils::SetInt(addn, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr addn_node = graph->AddNode(addn);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(addn_node->GetName(), std::move(buffer));
    addn_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(addn_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
  }

  void BuildGraph2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr addn = std::make_shared<OpDesc>("addn", "AddN");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1","ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2","ReLU");
    SetPattern(addn, "ElemWise");
    SetPattern(elemwise, "ElemWise");

    SetTvmType(addn);
    SetTvmType(elemwise);

    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    relu2->AddInputDesc(out_desc);
    relu2->AddOutputDesc(out_desc);
    AttrUtils::SetInt(addn, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr addn_node = graph->AddNode(addn);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(addn_node->GetName(), std::move(buffer));
    addn_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(addn_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutControlAnchor(), relu1_node->GetInControlAnchor());
  }

  void BuildGraph3(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr addn = std::make_shared<OpDesc>("addn", "AddN");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1","ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2","ReLU");
    SetPattern(addn, "ElemWise");
    SetPattern(elemwise, "ElemWise");

    SetTvmType(addn);
    SetTvmType(elemwise);

    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddInputDesc(out_desc);
    addn->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    relu2->AddInputDesc(out_desc);
    relu2->AddOutputDesc(out_desc);
    AttrUtils::SetInt(addn, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr addn_node = graph->AddNode(addn);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(addn_node->GetName(), std::move(buffer));
    addn_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutControlAnchor(), addn_node->GetInControlAnchor());
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(addn_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutControlAnchor(), relu1_node->GetInControlAnchor());
  }
};

TEST_F(UB_FUSION_UT_ELT_ELT, addn_eltwise_relu) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr,
                                                                                      fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  auto_buffer_fusion_ptr_->Run(*graph_out);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "AddN" &&
        node->GetName() == "addnelem") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_ELT_ELT, addn_eltwise_ctrl_relu) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  auto_buffer_fusion_ptr_->Run(*graph_out);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "AddN" &&
        node->GetName() == "addnelem") {
      for (auto in_ctrl : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
        if (in_ctrl->GetOwnerNode()->GetName() == "relu1") {
          find = 1;
        }
      }

    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_ELT_ELT, ctrl_addn_eltwise_ctrl_relu) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph3(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  auto_buffer_fusion_ptr_->Run(*graph_out);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_ELT_ELT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetName() == "DATA0") {
      for (auto &c_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
        if (c_anchor->GetOwnerNode()->GetName() != "addnelem") {
          EXPECT_EQ(0, 1);
        }
      }
    }
    if (node->GetOpDesc()->GetType() == "AddN" &&
        node->GetName() == "addnelem") {
      for (auto &out_ctrl : node->GetInControlAnchor()->GetPeerOutControlAnchors()) {
        if (out_ctrl->GetOwnerNode()->GetName() != "DATA0") {
          EXPECT_EQ(0, 1);
        }
      }
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
