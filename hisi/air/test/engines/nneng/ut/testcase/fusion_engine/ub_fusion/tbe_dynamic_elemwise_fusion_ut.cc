/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_broadcast_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_reduce_fusion_pass.h"
#include "graph_constructor.h"
#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;
namespace fe_ut {
class TbeDynamicElemwiseFusionUTest : public testing::Test {
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
    sub_graph_optimizer_ptr_ = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr,
                                                              fusion_priority_mgr_ptr_);
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

  void CheckResult(ComputeGraphPtr &graph_out) {
    int fusion_nodes_count = 0;
    for (auto node: graph_out->GetDirectNode()) {
      if (node->GetName() == "depthwise" ||
          node->GetName() == "dequant" ||
          node->GetName() == "add" ||
          node->GetName() == "relu6" ||
          node->GetName() == "mul1" ||
          node->GetName() == "mul2" ||
          node->GetName() == "quant") {
        int64_t scope_id = -1;
        (void) ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id);
        fusion_nodes_count++;
      }
    }
    EXPECT_GT(fusion_nodes_count, 5);
  }

  void CheckNotMatchedResult(ComputeGraphPtr &graph_out) {
    int fusion_nodes_count = 0;
    for (auto node: graph_out->GetDirectNode()) {
      if (node->GetName() == "depthwise" ||
          node->GetName() == "dequant" ||
          node->GetName() == "add" ||
          node->GetName() == "relu6" ||
          node->GetName() == "mul1" ||
          node->GetName() == "mul2" ||
          node->GetName() == "quant") {
        int64_t scope_id = -1;
        (void) ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id);
        ASSERT_EQ(scope_id, -1);
        fusion_nodes_count++;
      }
    }
    EXPECT_EQ(fusion_nodes_count, 7);
  }

  // elemwise(D) - > broadcast(D) -> elemwise(D)
  void BuildGraph1(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr add = std::make_shared<OpDesc>("add", "Add");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");

    SetPattern(elemwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(add, "Broadcast");

    SetTvmType(add);
    SetTvmType(elemwise);
    SetTvmType(relu);

    AttrUtils::SetInt(add, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    data1->AddOutputDesc(tenosr_desc);
    add->AddInputDesc(tenosr_desc);
    add->AddInputDesc(tenosr_desc);
    add->AddOutputDesc(tenosr_desc);
    elemwise->AddInputDesc(tenosr_desc);
    elemwise->AddOutputDesc(tenosr_desc);
    relu->AddInputDesc(tenosr_desc);
    relu->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr add_node = graph->AddNode(add);
    NodePtr relu_node = graph->AddNode(relu);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(elemwise_node->GetName(), std::move(buffer));
    elemwise_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(add_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  }

  // elemwise(D) - > elemwise(D) -> elemwise(D) - > elemwise(D) -> elemwise(D)
  void BuildGraph2(ComputeGraphPtr graph, int strategy = 0) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr elemwise3 = std::make_shared<OpDesc>("elem3", "Eltwise");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(elemwise3, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");

    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(elemwise3);
    SetTvmType(relu1);
    SetTvmType(relu2);

    switch (strategy) {
      case 1:
        ge::AttrUtils::SetBool(elemwise3, "_is_op_dynamic_impl", true);
        break;
      case 2:
        ge::AttrUtils::SetBool(elemwise1, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(elemwise2, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(elemwise3, "_is_op_dynamic_impl", false);
        ge::AttrUtils::SetBool(relu1, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(relu2, "_is_op_dynamic_impl", true);
        break;
      default:
        ge::AttrUtils::SetBool(elemwise1, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(elemwise2, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(elemwise3, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(relu1, "_is_op_dynamic_impl", true);
        ge::AttrUtils::SetBool(relu2, "_is_op_dynamic_impl", true);
    }

    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    elemwise1->AddInputDesc(tenosr_desc);
    elemwise2->AddInputDesc(tenosr_desc);
    elemwise3->AddInputDesc(tenosr_desc);
    elemwise1->AddOutputDesc(tenosr_desc);
    elemwise2->AddOutputDesc(tenosr_desc);
    elemwise3->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr elemwise1_node = graph->AddNode(elemwise1);
    NodePtr elemwise2_node = graph->AddNode(elemwise2);
    NodePtr elemwise3_node = graph->AddNode(elemwise3);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(elemwise1_node->GetName(), std::move(buffer));
    elemwise1_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    vector<char> buffer1(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr1 = std::make_shared<ge::OpKernelBin>(relu1_node->GetName(), std::move(buffer1));
    relu1_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr1);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise1_node->GetOutDataAnchor(0), elemwise2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise2_node->GetOutDataAnchor(0), elemwise3_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise3_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
  }

  // elemwise(D) - > elemwise(D) -> elemwise(S) - > elemwise(D) -> elemwise(D)
  void BuildGraph3(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr elemwise3 = std::make_shared<OpDesc>("elem3", "Eltwise");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(elemwise3, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");

    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(elemwise3);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    vector<int64_t> static_dim = {4, 4, 1, 4};
    GeShape static_shape(static_dim);
    GeTensorDesc static_tenosr_desc(static_shape);

    data->AddOutputDesc(tenosr_desc);
    elemwise1->AddInputDesc(tenosr_desc);
    elemwise2->AddInputDesc(tenosr_desc);
    elemwise3->AddInputDesc(static_tenosr_desc);
    elemwise1->AddOutputDesc(tenosr_desc);
    elemwise2->AddOutputDesc(tenosr_desc);
    elemwise3->AddOutputDesc(static_tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr elemwise1_node = graph->AddNode(elemwise1);
    NodePtr elemwise2_node = graph->AddNode(elemwise2);
    NodePtr elemwise3_node = graph->AddNode(elemwise3);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(elemwise1_node->GetName(), std::move(buffer));
    elemwise1_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    vector<char> buffer1(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr1 = std::make_shared<ge::OpKernelBin>(relu1_node->GetName(), std::move(buffer1));
    relu1_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr1);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise1_node->GetOutDataAnchor(0), elemwise2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise2_node->GetOutDataAnchor(0), elemwise3_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise3_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
  }

  // elemwise(D) -> elemwise(D) - > reduce(D) -> relu(D) -> relu(D)
  void BuildGraph11(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    reduce->AddInputDesc(tenosr_desc);
    reduce->AddOutputDesc(tenosr_desc);
    elemwise1->AddInputDesc(tenosr_desc);
    elemwise1->AddOutputDesc(tenosr_desc);
    elemwise2->AddInputDesc(tenosr_desc);
    elemwise2->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr elemwise_node1 = graph->AddNode(elemwise1);
    NodePtr elemwise_node2 = graph->AddNode(elemwise2);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(elemwise_node1->GetName(), std::move(buffer));
    elemwise_node1->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node1->GetOutDataAnchor(0), elemwise_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node2->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reduce_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
  }

  // reduce(D) -> relu(D) -> relu(D)
  void BuildGraph12(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    reduce->AddInputDesc(tenosr_desc);
    reduce->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(reduce_node->GetName(), std::move(buffer));
    reduce_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reduce_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
  }

  // elemwise(D) -> elemwise(D) -> reduce(D)
  void BuildGraph13(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, -1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    reduce->AddInputDesc(tenosr_desc);
    reduce->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(relu_node1->GetName(), std::move(buffer));
    relu_node1->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node2->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
  }

  // elemwise(S) -> elemwise(D) - > reduce(D) -> relu(D) -> relu(S)
  void BuildGraph14(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dyn_dim = {4, 4, -1, 4};
    GeShape dyn_shape(dyn_dim);
    GeTensorDesc dyn_tenosr_desc(dyn_shape);

    vector<int64_t> static_dim = {4, 4, 1, 4};
    GeShape static_shape(static_dim);
    GeTensorDesc static_tenosr_desc(static_shape);

    data->AddOutputDesc(static_tenosr_desc);
    reduce->AddInputDesc(dyn_tenosr_desc);
    reduce->AddOutputDesc(dyn_tenosr_desc);
    elemwise1->AddInputDesc(static_tenosr_desc);
    elemwise1->AddOutputDesc(static_tenosr_desc);
    elemwise2->AddInputDesc(dyn_tenosr_desc);
    elemwise2->AddOutputDesc(dyn_tenosr_desc);
    relu1->AddInputDesc(dyn_tenosr_desc);
    relu1->AddOutputDesc(dyn_tenosr_desc);
    relu2->AddInputDesc(static_tenosr_desc);
    relu2->AddOutputDesc(static_tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr elemwise_node1 = graph->AddNode(elemwise1);
    NodePtr elemwise_node2 = graph->AddNode(elemwise2);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(elemwise_node2->GetName(), std::move(buffer));
    elemwise_node2->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node1->GetOutDataAnchor(0), elemwise_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node2->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reduce_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
  }

  // elemwise(D) -> elemwise(S) - > reduce(D) -> relu(S) -> relu(D)
  void BuildGraph15(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dyn_dim = {4, 4, -1, 4};
    GeShape dyn_shape(dyn_dim);
    GeTensorDesc dyn_tenosr_desc(dyn_shape);

    vector<int64_t> static_dim = {4, 4, 1, 4};
    GeShape static_shape(static_dim);
    GeTensorDesc static_tenosr_desc(static_shape);

    data->AddOutputDesc(static_tenosr_desc);
    reduce->AddInputDesc(dyn_tenosr_desc);
    reduce->AddOutputDesc(dyn_tenosr_desc);
    elemwise1->AddInputDesc(dyn_tenosr_desc);
    elemwise1->AddOutputDesc(dyn_tenosr_desc);
    elemwise2->AddInputDesc(static_tenosr_desc);
    elemwise2->AddOutputDesc(static_tenosr_desc);
    relu1->AddInputDesc(static_tenosr_desc);
    relu1->AddOutputDesc(static_tenosr_desc);
    relu2->AddInputDesc(dyn_tenosr_desc);
    relu2->AddOutputDesc(dyn_tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr elemwise_node1 = graph->AddNode(elemwise1);
    NodePtr elemwise_node2 = graph->AddNode(elemwise2);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node1->GetOutDataAnchor(0), elemwise_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node2->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reduce_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
  }

  // elemwise(D) -> elemwise(D) - > reduce(S) -> relu(D) -> relu(D)
  void BuildGraph16(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr elemwise2 = std::make_shared<OpDesc>("elem2", "Eltwise");
    OpDescPtr reduce = std::make_shared<OpDesc>("reduce", "Reduce");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "ReLU");

    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise2, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(relu2, "ElemWise");
    SetPattern(reduce, "CommReduce");

    SetTvmType(reduce);
    SetTvmType(elemwise1);
    SetTvmType(elemwise2);
    SetTvmType(relu1);
    SetTvmType(relu2);

    AttrUtils::SetInt(reduce, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dyn_dim = {4, 4, -1, 4};
    GeShape dyn_shape(dyn_dim);
    GeTensorDesc dyn_tenosr_desc(dyn_shape);

    vector<int64_t> static_dim = {4, 4, 1, 4};
    GeShape static_shape(static_dim);
    GeTensorDesc static_tenosr_desc(static_shape);

    data->AddOutputDesc(static_tenosr_desc);
    reduce->AddInputDesc(static_tenosr_desc);
    reduce->AddOutputDesc(static_tenosr_desc);
    elemwise1->AddInputDesc(dyn_tenosr_desc);
    elemwise1->AddOutputDesc(dyn_tenosr_desc);
    elemwise2->AddInputDesc(dyn_tenosr_desc);
    elemwise2->AddOutputDesc(dyn_tenosr_desc);
    relu1->AddInputDesc(dyn_tenosr_desc);
    relu1->AddOutputDesc(dyn_tenosr_desc);
    relu2->AddInputDesc(dyn_tenosr_desc);
    relu2->AddOutputDesc(dyn_tenosr_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr reduce_node = graph->AddNode(reduce);
    NodePtr elemwise_node1 = graph->AddNode(elemwise1);
    NodePtr elemwise_node2 = graph->AddNode(elemwise2);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr relu_node2 = graph->AddNode(relu2);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), elemwise_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node1->GetOutDataAnchor(0), elemwise_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node2->GetOutDataAnchor(0), reduce_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reduce_node->GetOutDataAnchor(0), relu_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0), relu_node2->GetInDataAnchor(0));
  }
};

TEST_F(TbeDynamicElemwiseFusionUTest, broad_cast_elemwise_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph1(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeDynamicElemwiseFusionUTest UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "TbeDynamicElemwiseFusionUTest UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 3);
}

TEST_F(TbeDynamicElemwiseFusionUTest, broad_cast_elemwise_2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeDynamicElemwiseFusionUTest UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "TbeDynamicElemwiseFusionUTest UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 2);
}

TEST_F(TbeDynamicElemwiseFusionUTest, broad_cast_elemwise_3) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph3(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeDynamicElemwiseFusionUTest UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "TbeDynamicElemwiseFusionUTest UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 4);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph11(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 2);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph12(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 2);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_3) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph13(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 2);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_4) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph14(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 4);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_5) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph15(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 6);
}

TEST_F(TbeDynamicElemwiseFusionUTest, reduce_elemwise_6) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph16(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 6);
}

TEST_F(TbeDynamicElemwiseFusionUTest, check_dynamic_impl_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeDynamicElemwiseFusionUTest UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "TbeDynamicElemwiseFusionUTest UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 4);
}

TEST_F(TbeDynamicElemwiseFusionUTest, check_dynamic_impl_2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out, 2);
  graph_out->TopologicalSorting();
  graph_out->Dump();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_reduce = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_REDUCE_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseReduceFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dynamic_elemwise_reduce, UB_FUSION);

  // 27. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *dynamic_elemwise_broadcast = new(std::nothrow) BufferFusionPassRunner(
      DYNAMIC_ELEMWISE_BROADCAST_UB_PASS,
      []() -> BufferFusionPassBase * { return new(std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dynamic_elemwise_broadcast, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);

  cerr << "TbeDynamicElemwiseFusionUTest UB fusion after match" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
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
  cerr << "TbeDynamicElemwiseFusionUTest UB fusion result" << endl;
  for (auto &node: graph_out->GetDirectNode()) {
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType();
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << ", scope id : " << scope_id;
    }
    cerr << endl;
  }
  EXPECT_EQ(graph_out->GetDirectNodesSize(), 4);
}

const string kPatternConv = "conv2d";
const string kPatternDWConv = "DepthwiseConvolution";
const string kPatternAdd = "add";
const string kPatternRelu6 = "relu6";
const string kPatternMul1 = "mul1";
const string kPatternMul2 = "mul2";
const string kPatternDeq = "dequant";
const string kPatternQuant = "quant";
const string kPatternOtherInput = "otherInput";
const string kPatternOtherInput1 = "otherInput1";
const string kPatternOtherInput2 = "otherInput2";
const string kPatternOtherInput3 = "otherInput3";
const string kPatternOtherInputd = "otherInputd";
const string kPatternOtherOutput = "otherOutput";
const string kPatternOtherOutputd = "otherOutputd";
const string kPatternOtherOutput1 = "otherOutput1";

class Conv2dAddRelu6MulMulFusionPass : public BufferFusionPassBase {
 public:
  Conv2dAddRelu6MulMulFusionPass() {}

  ~Conv2dAddRelu6MulMulFusionPass() {}

  vector<BufferFusionPattern *> DefinePatterns() override {
    vector<BufferFusionPattern *> patterns;
    string pattern_name1 = "DepthwiseconvDequantAddRelu6MulMulQuantFusionPass";
    BufferFusionPattern *pattern1 = new(std::nothrow) BufferFusionPattern(pattern_name1);
    FE_LOGD("Start to define %s pass pattern.", pattern_name1.c_str());
    /* define pattern
     *       otherinput1--\
     *                     \
     *  Depthwiseconv-->  deuqnt   -->    add   -->  relu6  -->   mul1    -->    mul2  -->  quant
     *        otherOutputd | otherinput2--/          otherInputd /   otherinput3-/  \-->otheroutput1
     *                     |------------------------------------/
     */
    pattern1->AddOpDesc(kPatternDWConv, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternDeq, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternAdd, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternRelu6, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternQuant, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInputd, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput1, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput2, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput3, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutputd, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutput1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .SetHead({kPatternDWConv})
        .SetOutputs(kPatternDWConv, {kPatternDeq})
        .SetOutputs(kPatternOtherInput1, {kPatternDeq})
        .SetOutputs(kPatternDeq, {kPatternAdd, kPatternOtherOutputd}, TBE_OUTPUT_BRANCH_MULTI)
        .SetOutputs(kPatternOtherInput2, {kPatternAdd})
        .SetOutputs(kPatternAdd, {kPatternRelu6}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternRelu6, {kPatternMul1}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInputd, {kPatternMul1})
        .SetOutputs(kPatternMul1, {kPatternMul2}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInput3, {kPatternMul2})
        .SetOutputs(kPatternMul2, {kPatternQuant, kPatternOtherOutput1}, TBE_OUTPUT_BRANCH_MULTI);
    patterns.push_back(pattern1);
    FE_LOGD("End to define %s pass pattern.", pattern_name1.c_str());
    return patterns;
  }
};

class Conv2dAddRelu6MulMulFusionPass1 : public BufferFusionPassBase {
 public:
  Conv2dAddRelu6MulMulFusionPass1() {}

  ~Conv2dAddRelu6MulMulFusionPass1() {}

  vector<BufferFusionPattern *> DefinePatterns() override {
    vector<BufferFusionPattern *> patterns;
    string pattern_name1 = "DepthwiseconvDequantAddRelu6MulMulQuantFusionPass";
    BufferFusionPattern *pattern1 = new(std::nothrow) BufferFusionPattern(pattern_name1);
    FE_LOGD("Start to define %s pass pattern.", pattern_name1.c_str());
    /* define pattern
     *       otherinput1--\
     *                     \
     *  Depthwiseconv-->  deuqnt   -->    add   -->  relu6  -->   mul1    -->    mul2  -->  quant
     *        otherOutputd | otherinput2--/          otherInputd /   otherinput3-/  \-->otheroutput1
     *                     |------------------------------------/
     */
    pattern1->AddOpDesc(kPatternDWConv, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternDeq, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternAdd, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternRelu6, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternQuant, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInputd, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput1, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput2, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput3, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutputd, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutput1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .SetHead({kPatternDWConv})
        .SetOutputs(kPatternDWConv, {kPatternDeq})
        .SetOutputs(kPatternOtherInput1, {kPatternDeq})
        .SetOutputs(kPatternDeq, {kPatternAdd, kPatternOtherOutputd}, TBE_OUTPUT_BRANCH_MULTI)
        .SetOutputs(kPatternOtherInput2, {kPatternAdd})
        .SetOutputs(kPatternAdd, {kPatternRelu6}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternRelu6, {kPatternMul1}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInputd, {kPatternMul1})
        .SetOutputs(kPatternMul1, {kPatternMul2}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInput3, {kPatternMul2})
        .SetOutputs(kPatternMul2, {kPatternQuant, kPatternOtherOutput1}, TBE_OUTPUT_BRANCH_MULTI, false, true);
    patterns.push_back(pattern1);
    FE_LOGD("End to define %s pass pattern.", pattern_name1.c_str());
    return patterns;
  }
};


class Conv2dAddRelu6MulMulFusionPass2 : public BufferFusionPassBase {
 public:
  Conv2dAddRelu6MulMulFusionPass2() {}

  ~Conv2dAddRelu6MulMulFusionPass2() {}

  vector<BufferFusionPattern *> DefinePatterns() override {
    vector<BufferFusionPattern *> patterns;
    string pattern_name1 = "DepthwiseconvDequantAddRelu6MulMulQuantFusionPass";
    BufferFusionPattern *pattern1 = new(std::nothrow) BufferFusionPattern(pattern_name1);
    FE_LOGD("Start to define %s pass pattern.", pattern_name1.c_str());
    /* define pattern
     *       otherinput1--\
     *                     \
     *  Depthwiseconv-->  deuqnt   -->    add   -->  relu6  -->   mul1    -->    mul2  -->  quant
     *        otherOutputd | otherinput2--/          otherInputd /   otherinput3-/  \-->otheroutput1
     *                     |------------------------------------/
     */
    pattern1->AddOpDesc(kPatternDWConv, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternDeq, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternAdd, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternRelu6, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternQuant, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInputd, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput1, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput2, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput3, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutputd, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutput1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .SetHead({kPatternDWConv})
        .SetOutputs(kPatternDWConv, {kPatternDeq})
        .SetOutputs(kPatternOtherInput1, {kPatternDeq})
        .SetOutputs(kPatternDeq, {kPatternAdd, kPatternOtherOutputd}, TBE_OUTPUT_BRANCH_MULTI)
        .SetOutputs(kPatternOtherInput2, {kPatternAdd})
        .SetOutputs(kPatternAdd, {kPatternRelu6}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternRelu6, {kPatternMul1}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInputd, {kPatternMul1})
        .SetOutputs(kPatternMul1, {kPatternMul2}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInput3, {kPatternMul2})
        .SetOutputs(kPatternMul2, {kPatternQuant, kPatternOtherOutput1}, TBE_OUTPUT_BRANCH_MULTI, false, true);
    patterns.push_back(pattern1);
    FE_LOGD("End to define %s pass pattern.", pattern_name1.c_str());
    return patterns;
  }
};


class Conv2dAddRelu6MulMulFusionPass3 : public BufferFusionPassBase {
 public:
  Conv2dAddRelu6MulMulFusionPass3() {}

  ~Conv2dAddRelu6MulMulFusionPass3() {}

  vector<BufferFusionPattern *> DefinePatterns() override {
    vector<BufferFusionPattern *> patterns;
    string pattern_name1 = "DepthwiseconvDequantAddRelu6MulMulQuantFusionPass";
    BufferFusionPattern *pattern1 = new(std::nothrow) BufferFusionPattern(pattern_name1);
    FE_LOGD("Start to define %s pass pattern.", pattern_name1.c_str());
    /* define pattern
     *       otherinput1--\
     *                     \
     *  Depthwiseconv-->  deuqnt   -->    add   -->  relu6  -->   mul1    -->    mul2  -->  quant
     *        otherOutputd | otherinput2--/          otherInputd /   otherinput3-/  \-->otheroutput1
     *                     |------------------------------------/
     */
    pattern1->AddOpDesc(kPatternDWConv, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternDeq, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternAdd, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternRelu6, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternMul2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternQuant, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInputd, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput1, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput2, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherInput3, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutputd, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
        .AddOpDesc(kPatternOtherOutput1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
        .SetHead({kPatternDWConv})
        .SetOutputs(kPatternDWConv, {kPatternDeq})
        .SetOutputs(kPatternOtherInput1, {kPatternDeq})
        .SetOutputs(kPatternDeq, {kPatternAdd, kPatternOtherOutputd}, TBE_OUTPUT_BRANCH_MULTI)
        .SetOutputs(kPatternOtherInput2, {kPatternAdd})
        .SetOutputs(kPatternAdd, {kPatternRelu6}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternRelu6, {kPatternMul1}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInputd, {kPatternMul1})
        .SetOutputs(kPatternMul1, {kPatternMul2}, TBE_OUTPUT_BRANCH_SINGLE)
        .SetOutputs(kPatternOtherInput3, {kPatternMul2})
        .SetOutputs(kPatternMul2, {kPatternQuant, kPatternOtherOutput1}, TBE_OUTPUT_BRANCH_MULTI);
    patterns.push_back(pattern1);
    FE_LOGD("End to define %s pass pattern.", pattern_name1.c_str());
    return patterns;
  }
};

void BuildGraphInfiniteLoop(ComputeGraphPtr graph) {
  ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                        original_shape);
  string ADD = "Add";
  test.AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_CONV, "depthwise", "DepthWiseConv2D", 2, 1)
      .SetInputs({"Data_1", "Data_2"})
      .AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_DEQUANT, "dequant", "AscendDequant", 2, 1)
      .SetInputs({"depthwise:0", "Data_3"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
      .SetInputs({"dequant:0", "Data4"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
      .SetInputs({"add:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul1", "Mul", 2, 1)
      .SetInputs({"dequant:0", "relu6:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul2", "Mul", 2, 1)
      .SetInputs({"mul1:0", "Data5"})
      .AddOpDesc(EN_IMPL_HW_TBE, "Opaque", "out", "NetOutput", 1, 0)
      .SetInputs({"mul2:0"});

  test.DumpGraph(graph);
}

void BuildGraphInfiniteLoop2(ComputeGraphPtr graph) {
  ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                        original_shape);
  string ADD = "Add";
  test.AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_CONV, "depthwise", "DepthWiseConv2D", 2, 1)
      .SetInputs({"Data_1", "Data_2"})
      .AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_DEQUANT, "dequant", "AscendDequant", 2, 1)
      .SetInputs({"depthwise:0", "Data_3"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
      .SetInputs({"dequant:0", "Data4"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
      .SetInputs({"add:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul1", "Mul", 2, 1)
      .SetInputs({"dequant:0", "relu6:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul2", "Mul", 2, 1)
      .SetInputs({"mul1:0", "Data5"})
      .AddOpDesc(EN_IMPL_HW_TBE, "Opaque", "out", "NetOutput", 1, 0)
      .SetInputs({"mul2:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "Opaque", "avgpool", "AvgPool", 1, 0)
      .SetInputs({"mul2:0"});
  test.DumpGraph(graph);
}

void BuildGraphInfiniteLoop3(ComputeGraphPtr graph) {
  ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                        original_shape);
  string ADD = "Add";
  test.AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_CONV, "depthwise", "DepthWiseConv2D", 2, 1)
      .SetInputs({"Data_1", "Data_2"})
      .AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_DEQUANT, "dequant", "AscendDequant", 2, 1)
      .SetInputs({"depthwise:0", "Data_3"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
      .SetInputs({"dequant:0", "Data4"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
      .SetInputs({"add:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul1", "Mul", 2, 1)
      .SetInputs({"dequant:0", "relu6:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul2", "Mul", 2, 1)
      .SetInputs({"mul1:0", "Data5"})
      .AddOpDesc(EN_IMPL_HW_TBE, "Opaque", "out", "NetOutput", 1, 0)
      .SetInputs({"mul2:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 0)
      .SetInputs({"mul2:0"});
  test.DumpGraph(graph);
}

void BuildGraphInfiniteLoop4(ComputeGraphPtr graph) {
  ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                        original_shape);
  string ADD = "Add";
  test.AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_CONV, "depthwise", "DepthWiseConv2D", 2, 1)
      .SetInputs({"Data_1", "Data_2"})
      .AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_DEQUANT, "dequant", "AscendDequant", 2, 1)
      .SetInputs({"depthwise:0", "Data_3"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
      .SetInputs({"dequant:0", "Data4"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
      .SetInputs({"add:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul1", "Mul", 2, 1)
      .SetInputs({"dequant:0", "relu6:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul2", "Mul", 2, 1)
      .SetInputs({"mul1:0", "Data5"})
      .AddOpDesc(EN_IMPL_HW_TBE, "Opaque", "out", "NetOutput", 1, 0)
      .SetInputs({"mul2:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 0)
      .SetInputs({"mul2:0"});
  test.DumpGraph(graph);
}

void BuildGraphInfiniteLoop5(ComputeGraphPtr graph) {
  ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                        original_shape);
  string ADD = "Add";
  test.AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_CONV, "depthwise", "DepthWiseConv2D", 2, 1)
      .SetInputs({"Data_1", "Data_2"})
      .AddOpDesc(EN_IMPL_HW_TBE, OP_PATTERN_DEQUANT, "dequant", "AscendDequant", 2, 1)
      .SetInputs({"depthwise:0", "Data_3"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
      .SetInputs({"dequant:0", "Data4"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
      .SetInputs({"add:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul1", "Mul", 2, 1)
      .SetInputs({"dequant:0", "relu6:0"})
      .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul2", "Mul", 2, 1)
      .SetInputs({"mul1:0", "Data5"})
      .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 0)
      .SetInputs({"mul2:0"});
  test.DumpGraph(graph);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop2(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_3) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop3(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_4) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop2(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass1(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_5) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop3(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass1(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_6) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop5(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass1(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckNotMatchedResult(graph_out);
}

TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_7) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop5(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}


TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_8) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop5(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass2(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}

/* Although the mul2 has only one output node where pattern needs
 * multioutput), but it's the second last
 * node of this pattern. So we will ignore this check.
 * The grap will be matched. */
TEST_F(TbeDynamicElemwiseFusionUTest, test_infinite_loop_9) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphInfiniteLoop5(graph_out);
  graph_out->TopologicalSorting();

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass = std::make_shared<PassManager>(
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr());
  // 26. TbeDynamicElemwiseReduceFusionPass
  BufferFusionPassRunner *test_pass = new(std::nothrow) BufferFusionPassRunner(
      "Conv2DAddRelu6MulMulFusionPass",
      []() -> BufferFusionPassBase * { return new(std::nothrow) Conv2dAddRelu6MulMulFusionPass3(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("Conv2DAddRelu6MulMulFusionPass", AI_CORE_NAME,
                              test_pass, UB_FUSION);

  // find sub-graphs that match UB fusion pattern
  tbe_ub_fusion_pass->Run(*graph_out);
  CheckResult(graph_out);
}
}