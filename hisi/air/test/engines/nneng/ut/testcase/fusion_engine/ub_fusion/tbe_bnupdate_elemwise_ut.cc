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
#include "graph_constructor.h"
#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class UB_FUSION_UT_BNUPDATE_ELEMWISE : public testing::Test {
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
  void SetTvmType(ge::OpDescPtr opdef) { ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM)); }

  void BuildGraphBNUpdateReluSuc(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(4),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(4),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateReluOneout(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateReluMultiout(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(1),
                        square_node4->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateAddReluSuc(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr add = std::make_shared<OpDesc>("add", "Add");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(add, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(add);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(add, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr add_node = graph->AddNode(add);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(add_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateAddReluOneout(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr add = std::make_shared<OpDesc>("add", "Add");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(add, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(add);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(add, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr add_node = graph->AddNode(add);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(add_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateAddReluMultiout(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr add = std::make_shared<OpDesc>("add", "Add");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(add, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(add);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddInputDesc(out_desc);
    add->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(add, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr add_node = graph->AddNode(add);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        add_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(add_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
  }

  void BuildGraphMultiBNUpdateReluSuc(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr bnupdate1 =
        std::make_shared<OpDesc>("bnupdate1", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(bnupdate1, "bn_update");
    SetPattern(relu1, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(bnupdate1);
    SetTvmType(relu1);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    bnupdate1->AddInputDesc(out_desc);
    bnupdate1->AddInputDesc(out_desc);
    bnupdate1->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(bnupdate1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr bnupdate_node1 = graph->AddNode(bnupdate1);
    NodePtr relu_node1 = graph->AddNode(relu1);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(4),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(4),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node1->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node1->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateReluFail(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(4),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateReluFail1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(3),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }

  void BuildGraphBNUpdateReluFail2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr bnupdate =
        std::make_shared<OpDesc>("bnupdate", "BNTrainingUpdate");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReluV2");
    OpDescPtr square = std::make_shared<OpDesc>("square", "Square");
    OpDescPtr square1 = std::make_shared<OpDesc>("square1", "Square");
    OpDescPtr square2 = std::make_shared<OpDesc>("square2", "Square");
    OpDescPtr square3 = std::make_shared<OpDesc>("square3", "Square");
    OpDescPtr square4 = std::make_shared<OpDesc>("square4", "Square");
    OpDescPtr square5 = std::make_shared<OpDesc>("square5", "Square");

    SetPattern(bnupdate, "bn_update");
    SetPattern(relu, "ElemWise");
    SetPattern(square, "Square");
    SetPattern(square1, "Square");
    SetPattern(square2, "Square");
    SetPattern(square3, "Square");
    SetPattern(square4, "Square");
    SetPattern(square5, "Square");
    SetTvmType(bnupdate);
    SetTvmType(relu);
    SetTvmType(square);
    SetTvmType(square1);
    SetTvmType(square2);
    SetTvmType(square3);
    SetTvmType(square4);
    SetTvmType(square5);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddInputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    bnupdate->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    square->AddInputDesc(out_desc);
    square->AddOutputDesc(out_desc);
    square1->AddInputDesc(out_desc);
    square1->AddOutputDesc(out_desc);
    square2->AddInputDesc(out_desc);
    square2->AddOutputDesc(out_desc);
    square3->AddInputDesc(out_desc);
    square3->AddOutputDesc(out_desc);
    square4->AddInputDesc(out_desc);
    square4->AddOutputDesc(out_desc);
    square5->AddInputDesc(out_desc);
    square5->AddOutputDesc(out_desc);
    AttrUtils::SetInt(bnupdate, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(square5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr bnupdate_node = graph->AddNode(bnupdate);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr square_node = graph->AddNode(square);
    NodePtr square_node1 = graph->AddNode(square1);
    NodePtr square_node2 = graph->AddNode(square2);
    NodePtr square_node3 = graph->AddNode(square3);
    NodePtr square_node4 = graph->AddNode(square4);
    NodePtr square_node5 = graph->AddNode(square5);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        bnupdate_node->GetName(), std::move(buffer));
    bnupdate_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL,
                                          tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        bnupdate_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(1),
                        square_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node4->GetInDataAnchor(0));
    GraphUtils::AddEdge(bnupdate_node->GetOutDataAnchor(2),
                        square_node5->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        square_node5->GetInDataAnchor(0));
  }
};

TEST_F(UB_FUSION_UT_BNUPDATE_ELEMWISE, bn_update_relu_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphBNUpdateReluSuc(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("enginName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_BNUPDATE_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_BNUPDATE_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "BNTrainingUpdate" &&
        node->GetName() == "bnupdaterelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_BNUPDATE_ELEMWISE, bn_update_add_relu_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphBNUpdateAddReluSuc(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("enginName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "BNTrainingUpdate" &&
        node->GetName() == "bnupdateaddrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_BNUPDATE_ELEMWISE, bn_update_add_relu_fusion_pass_oneout) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphBNUpdateAddReluOneout(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("enginName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "BNTrainingUpdate" &&
        node->GetName() == "bnupdate") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_BNUPDATE_ELEMWISE, bn_update_add_relu_fusion_pass_multiout) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphBNUpdateAddReluMultiout(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("enginName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_BNUPDATE_ADD_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "BNTrainingUpdate" &&
        node->GetName() == "bnupdate") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_BNUPDATE_ELEMWISE, bn_update_multi_relu_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphMultiBNUpdateReluSuc(graph_out, 1);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("enginName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_BNUPDATE_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_BNUPDATE_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "BNTrainingUpdate" &&
        node->GetName() == "bnupdaterelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

