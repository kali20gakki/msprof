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
#include "graph_optimizer/fusion_common/fusion_pass_name.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_elemwise_quant_fusion_pass.h"
#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class TbeElemwiseQuantFusionUnitTest : public testing::Test {
 public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

 protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }
  static void TearDownTestCase() { std::cout << "UB fusion TearDown" << std::endl; }
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr_;
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr_;
  virtual void SetUp() {
    std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
    std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
    fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
    sub_graph_optimizer_ptr_ = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr_);
    sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  }

  virtual void TearDown() {

  }

  void SetPattern(ge::OpDescPtr opdef, const string &optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }
  void SetTvmType(ge::OpDescPtr opdef) {
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
  }

  // add - > quant
  void BuildGraph1(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr add = std::make_shared<OpDesc>("add", "Add");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");

    SetPattern(add, "ElemWise");
    SetPattern(quant, "quant");

    SetTvmType(add);
    SetTvmType(quant);

    AttrUtils::SetInt(add, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, 1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    data1->AddOutputDesc(tenosr_desc);
    add->AddInputDesc(tenosr_desc);
    add->AddInputDesc(tenosr_desc);
    add->AddOutputDesc(tenosr_desc);
    quant->AddInputDesc(tenosr_desc);
    quant->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr add_node = graph->AddNode(add);
    NodePtr quant_node = graph->AddNode(quant);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(add_node->GetName(), std::move(buffer));
    add_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(add_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  }

  // add - > quant
  void BuildGraph2(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr relu = std::make_shared<OpDesc>("add", "Relu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");

    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");

    SetTvmType(relu);
    SetTvmType(quant);

    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, 1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    relu->AddInputDesc(tenosr_desc);
    relu->AddOutputDesc(tenosr_desc);
    quant->AddInputDesc(tenosr_desc);
    quant->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(relu_node->GetName(), std::move(buffer));
    relu_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  }

  // eltwise - > quant
  void BuildGraph3(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");

    SetPattern(eltwise, "ElemWise");
    SetPattern(quant, "quant");

    SetTvmType(eltwise);
    SetTvmType(quant);

    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, 1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    data1->AddOutputDesc(tenosr_desc);
    eltwise->AddInputDesc(tenosr_desc);
    eltwise->AddInputDesc(tenosr_desc);
    eltwise->AddOutputDesc(tenosr_desc);
    quant->AddInputDesc(tenosr_desc);
    quant->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr quant_node = graph->AddNode(quant);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(eltwise_node->GetName(), std::move(buffer));
    eltwise_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  }
};

TEST_F(TbeElemwiseQuantFusionUnitTest, elemwise_quant_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph1(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

    std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 28. TbeElemwiseQuantFusionPass
  BufferFusionPassRunner *elemwise_quant_ub_pass = new (std::nothrow) BufferFusionPassRunner(
      ELEMWISE_QUANT_UB_PASS,
      []() -> BufferFusionPassBase * { return new (std::nothrow) TbeElemwiseQuantFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(ELEMWISE_QUANT_UB_PASS, AI_CORE_NAME, elemwise_quant_ub_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion after match" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  cerr << endl;
  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 3);
}

TEST_F(TbeElemwiseQuantFusionUnitTest, elemwise_quant_2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

    std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 28. TbeElemwiseQuantFusionPass
  BufferFusionPassRunner *elemwise_quant_ub_pass = new (std::nothrow) BufferFusionPassRunner(
          ELEMWISE_QUANT_UB_PASS,
          []() -> BufferFusionPassBase * { return new (std::nothrow) TbeElemwiseQuantFusionPass(); },
          cycle_detector);
  tbe_ub_fusion_pass->AddPass(ELEMWISE_QUANT_UB_PASS, AI_CORE_NAME, elemwise_quant_ub_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion after match" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
  uint32_t scope_id = 0;
  cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
  if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
    cerr << ", scope id : " << scope_id;
  }
    cerr << endl;
  }

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  cerr << endl;
  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 3);
}

TEST_F(TbeElemwiseQuantFusionUnitTest, elemwise_quant_3) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph3(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

    std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 28. TbeElemwiseQuantFusionPass
  BufferFusionPassRunner *elemwise_quant_ub_pass = new (std::nothrow) BufferFusionPassRunner(
          ELEMWISE_QUANT_UB_PASS,
          []() -> BufferFusionPassBase * { return new (std::nothrow) TbeElemwiseQuantFusionPass(); },
          cycle_detector);
  tbe_ub_fusion_pass->AddPass(ELEMWISE_QUANT_UB_PASS, AI_CORE_NAME, elemwise_quant_ub_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion after match" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph_out);
  cerr << endl;
  graph_out->Dump();
  cerr << endl;
  cerr << "TbeElemwiseQuantFusionUnitTest UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 4);
}