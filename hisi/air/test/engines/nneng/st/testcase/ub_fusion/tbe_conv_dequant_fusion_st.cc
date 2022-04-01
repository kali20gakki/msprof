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
#include "common/sgt_slice_type.h"
#include "common/util/op_info_util.h"
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

class UB_FUSION_ST_CONV_DEQUANT : public testing::Test {
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

  void SetTvmType(ge::OpDescPtr opdef) { 
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM)); }
/************************
 *
 *          x    filter
 *          |      |
 *            conv
 *              |
 *  deq_scale一dequant
 *                |       
 *  deq_scale1一dequant1
 *************************/
  void BuildGraph(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", "Dequant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(dequant1, "dequant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(dequant1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    data3->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    dequant1->AddInputDesc(out_desc);
    dequant1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(
        OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
        dequant1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
        dequant1_node->GetInDataAnchor(1));
  }

/************************
 *
 *          x    filter
 *          |      |
 *            conv
 *              |
 *  deq_scale一dequant
 *                |
 *  deq_scale1一dequant1
 *************************/
  void BuildGraphSgtSlice(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", "Dequant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(dequant1, "dequant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(dequant1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    data3->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    dequant1->AddInputDesc(out_desc);
    dequant1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv, kThreadScopeId, 1);
    AttrUtils::SetInt(dequant, kThreadScopeId, 1);
    ffts::ThreadSliceMap subgraphInfo;
    vector<vector<vector<ffts::DimRange>>> inputTensorSlice;
    vector<vector<vector<ffts::DimRange>>> oriInputTensorSlice;
    vector<vector<vector<ffts::DimRange>>> outputTensorSlice;
    vector<vector<vector<ffts::DimRange>>> oriOutputTensorSlice;
    vector<vector<vector<int64_t>>> oriInputTensorShape;
    vector<vector<vector<int64_t>>> oriOutputTensorShape;
    vector<vector<int64_t>> oriThreadShape;
    vector<int64_t> oriInputShape;
    vector<vector<int64_t>> oriOutputThreadShape;
    vector<int64_t> oriOutputShape;

    for (size_t i = 0; i < 2; i++) {
      vector<int64_t> vec1 = {0, 288, 0, 32, 0, 16, 0, 16};
      vector<ffts::DimRange> vdr1;
      for (size_t j = 0; j < vec1.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec1[j];
        dr.higher = vec1[j + 1];
        vdr1.push_back(dr);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> threadSlice;
      threadSlice.push_back(vdr1);
      vector<int64_t> vec2 = {0, 288, 0, 32, 0, 16, 0, 16};
      vector<ffts::DimRange> vdr2;
      for (size_t j = 0; j < vec2.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec2[j];
        dr.higher = vec2[j + 1];
        vdr2.push_back(dr);
        oriInputShape.push_back(dr.higher - dr.lower);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> oriThreadSlice;
      oriThreadSlice.push_back(vdr2);
      oriThreadShape.push_back(oriInputShape);
      inputTensorSlice.push_back(threadSlice);
      oriInputTensorSlice.push_back(oriThreadSlice);
      oriInputTensorShape.push_back(oriThreadShape);
    }

    for (size_t i = 0; i < 2; i++) {
      vector<int64_t> vec3 = {0, 288, 0, 32, 0, 16, 0, 16};
      vector<ffts::DimRange> vdr3;
      for (size_t j = 0; j < vec3.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec3[j];
        dr.higher = vec3[j + 1];
        vdr3.push_back(dr);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> threadSlice;
      threadSlice.push_back(vdr3);
      vector<int64_t> vec4 = {0, 288, 0, 32, 0, 16, 0, 16};
      vector<ffts::DimRange> vdr4;
      for (size_t j = 0; j < vec4.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec4[j];
        dr.higher = vec4[j + 1];
        vdr4.push_back(dr);
        oriOutputShape.push_back(dr.higher - dr.lower);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> oriThreadSlice;
      oriThreadSlice.push_back(vdr4);
      oriOutputThreadShape.push_back(oriOutputShape);
      outputTensorSlice.push_back(threadSlice);
      oriOutputTensorSlice.push_back(oriThreadSlice);
      oriOutputTensorShape.push_back(oriOutputThreadShape);
    }
    subgraphInfo.input_tensor_slice = inputTensorSlice;
    subgraphInfo.ori_input_tensor_slice = oriInputTensorSlice;
    subgraphInfo.output_tensor_slice = outputTensorSlice;
    subgraphInfo.ori_output_tensor_slice = oriOutputTensorSlice;
    subgraphInfo.ori_input_tensor_shape = oriInputTensorShape;
    subgraphInfo.ori_output_tensor_shape = oriOutputTensorShape;
    subgraphInfo.slice_instance_num = 2;
    ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(subgraphInfo);
    conv->SetExtAttr(ffts::kAttrSgtStructInfo, tsmp_ptr);
    dequant->SetExtAttr(ffts::kAttrSgtStructInfo, tsmp_ptr);
    AttrUtils::SetInt(dequant1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(
            OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
                        dequant1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
                        dequant1_node->GetInDataAnchor(1));
  }

/************************
 *
 *          x    filter
 *          |      |
 *            conv一一一bias
 *              |
 *  deq_scale一dequant
 *                |       
 *  deq_scale1一dequant1
 *************************/
  void BuildGraph1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", "Dequant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(dequant1, "dequant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(dequant1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    data3->AddOutputDesc(out_desc);
    data4->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    dequant1->AddInputDesc(out_desc);
    dequant1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(
        OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
        dequant1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0),
        dequant1_node->GetInDataAnchor(1));
  }
/************************
 *
 *          x    filter
 *          |      |
 *            conv
 *              |
 *  deq_scale一dequant一conv
 *                |       
 *            dequant1
 *
 *************************/
  void BuildGraph2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", "Dequant");
    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(dequant1, "dequant");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(dequant);
    SetTvmType(dequant1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    conv1->AddInputDesc(out_desc);
    conv1->AddInputDesc(out_desc);
    conv1->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddOutputDesc(out_desc);
    dequant1->AddInputDesc(out_desc);
    dequant1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
        dequant_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),
        dequant1_node->GetInDataAnchor(0));
  }
};

TEST_F(UB_FUSION_ST_CONV_DEQUANT, conv_dequant) {
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
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = 
    std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion before" << endl;
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
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);
  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convdequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_CONV_DEQUANT, conv_dequant_sgt_slice) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphSgtSlice(graph_out, 1);
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
  uint32_t id = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion before" << endl;
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
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);
  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
  cerr << "id:" << id << endl;
  uint32_t scope_id = 0;
  cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
  if (node->GetOpDesc()->GetType() == "Convolution" &&
  node->GetName() == "convdequant") {
  find = 1;
  }
  if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
  cerr << "scope id : " << scope_id << endl;
  }
  id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_CONV_DEQUANT, conv_bias_dequant) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph1(graph_out, 1);
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
  uint32_t id = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion before" << endl;
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
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);
  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convdequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_CONV_DEQUANT, conv_conv_dequant) {
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
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = 
    std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion before" << endl;
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
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph_out);
  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_CONV_DEQUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() != "convdequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}
