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

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class UB_FUSION_UT_CONV_STRIDED_QUANT : public testing::Test {
 public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

 protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }
  static void TearDownTestCase() { std::cout << "UB fusion TearDown" << std::endl; }

  virtual void SetUp() {
  }
  virtual void TearDown() {}
  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }
  void SetTvmType(ge::OpDescPtr opdef) { ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM)); }

  void BuildGrapStridedConv1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr read = std::make_shared<OpDesc>("read", "StridedRead");
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr write = std::make_shared<OpDesc>("write", "StridedWrite");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");

    SetPattern(read, "strided_read");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(write, "strided_write");
    SetPattern(quant, "quant");
    SetTvmType(read);
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(write);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    data3->AddOutputDesc(out_desc);
    read->AddInputDesc(out_desc);
    read->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    write->AddInputDesc(out_desc);
    write->AddOutputDesc(out_desc);
    AttrUtils::SetInt(read, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(write, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr read_node = graph->AddNode(read);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr write_node = graph->AddNode(write);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            read_node->GetName(), std::move(buffer));
    read_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                      tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        read_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(read_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(3));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0),
                        write_node->GetInDataAnchor(0));
  }

  void BuildGrapStridedConv2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr read = std::make_shared<OpDesc>("read", "StridedRead");
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr write = std::make_shared<OpDesc>("write", "StridedWrite");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");

    SetPattern(read, "strided_read");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(write, "strided_write");
    SetPattern(quant, "quant");
    SetTvmType(read);
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(write);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    data3->AddOutputDesc(out_desc);
    read->AddInputDesc(out_desc);
    read->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    write->AddInputDesc(out_desc);
    write->AddOutputDesc(out_desc);
    AttrUtils::SetInt(read, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(write, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr read_node = graph->AddNode(read);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr write_node = graph->AddNode(write);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            read_node->GetName(), std::move(buffer));
    read_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                      tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        read_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(read_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(3));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
                        write_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(write_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }

  void BuildGrapStridedConv3(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr write = std::make_shared<OpDesc>("write", "StridedWrite");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");

    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(write, "strided_write");
    SetPattern(square, "Square");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(write);
    SetTvmType(square);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    data->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    write->AddInputDesc(out_desc);
    write->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(write, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    NodePtr data_node = graph->AddNode(data);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr write_node = graph->AddNode(write);
    NodePtr square_node = graph->AddNode(square);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                      tbe_kernel_ptr);

    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
                        write_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(write_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
  }
};

TEST_F(UB_FUSION_UT_CONV_STRIDED_QUANT, strideconv1) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGrapStridedConv1(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
// BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  std::shared_ptr<GraphComm> graph_comm(new (std::nothrow) GraphComm("engineName"));
  graph_comm->Initialize();
  if ((graph_comm.get() == nullptr)) {
    EXPECT_EQ(true, false);
  }
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
// find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);

// create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "readconvdequantquantwrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_CONV_STRIDED_QUANT, strideconv2) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGrapStridedConv2(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
// BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  std::shared_ptr<GraphComm> graph_comm(new (std::nothrow) GraphComm("engineName"));
  graph_comm->Initialize();

  if ((graph_comm.get() == nullptr)) {
    EXPECT_EQ(true, false);
  }
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
// find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);

// create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "readconvdequantwrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_CONV_STRIDED_QUANT, strideconv3) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGrapStridedConv3(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
// BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  std::shared_ptr<GraphComm> graph_comm(new (std::nothrow) GraphComm("engineName"));
  graph_comm->Initialize();
  if ((graph_comm.get() == nullptr)) {
    EXPECT_EQ(true, false);
  }
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
// find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);

// create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_STRIDED_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantwrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
