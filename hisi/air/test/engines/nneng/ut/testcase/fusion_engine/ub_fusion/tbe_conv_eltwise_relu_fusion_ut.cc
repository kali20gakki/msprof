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

#include <map>
#include <memory>
#include <stdio.h>
#include "proto/om.pb.h"
#include "gtest/gtest.h"

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
#include "common/aicore_util_types.h"

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;
static const string STREAM_LABEL = "_stream_label";

class UB_FUSION_UT_CONV_ELT_RELU : public testing::Test {
public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }
  static void TearDownTestCase() {
    std::cout << "UB fusion TearDown" << std::endl;
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {}
  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }
  void SetTvmType(ge::OpDescPtr opdef) {
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
  }
  void BuildGraph(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");

    SetPattern(conv, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraphForL2Fusion(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    SetPattern(conv, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));

    //elementwise l2 Info
    L2FusionInfoPtr elementwise_l2_info_ptr = std::make_shared<TaskL2FusionInfo_t>();
    //data
    uint64_t L2_mirror_addr=0;          // preload or swap source address
    uint32_t L2_data_section_size=123;    // every data size
    uint8_t L2_preload=0;               // 1 - preload from mirror_addr, 0 - no preload
    uint8_t modified=1;                 // 1 - data will be modified by kernel, 0 - no modified
    uint8_t priority=1;                 // data priority
    int8_t prev_L2_page_offset_base=-1;  // remap source section offset
    uint8_t L2_page_offset_base=0;      // remap destination section offset
    uint8_t L2_load_to_ddr=0;           // 1 - need load out, 0 - no need
    rtSmData_t tmp_data={L2_mirror_addr,L2_data_section_size,L2_preload,modified,priority,prev_L2_page_offset_base,L2_page_offset_base,L2_page_offset_base,L2_load_to_ddr};
    tmp_data.reserved[2]={0};

      elementwise_l2_info_ptr->l2_info.l2ctrl.data[0]=tmp_data;
      elementwise_l2_info_ptr->l2_info.l2ctrl.size=60;
      elementwise_l2_info_ptr->node_name="elem";
    L2FusionData_t elem_output={0,123,2};
      elementwise_l2_info_ptr->output[0]=elem_output;

    (void)ge::AttrUtils::SetBool(conv_node->GetOpDesc(), NEED_RE_PRECOMPILE, true);
    elemwise_node->GetOpDesc()->SetExtAttr(
            ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, elementwise_l2_info_ptr);

    //relu l2 Info
     L2FusionInfoPtr relu_l2_info_ptr = std::make_shared<TaskL2FusionInfo_t>();
    //data
    uint64_t L2_mirror_addr1=0;          // preload or swap source address
    uint32_t L2_data_section_size1=456;    // every data size
    uint8_t L2_preload1=0;               // 1 - preload from mirror_addr, 0 - no preload
    uint8_t modified1=1;                 // 1 - data will be modified by kernel, 0 - no modified
    uint8_t priority1=1;                 // data priority
    int8_t prev_L2_page_offset_base1=-1;  // remap source section offset
    uint8_t L2_page_offset_base1=11;      // remap destination section offset
    uint8_t L2_load_to_ddr1=0;           // 1 - need load out, 0 - no need
    rtSmData_t tmp_data1={L2_mirror_addr1,L2_data_section_size1,L2_preload1,modified1,priority1,prev_L2_page_offset_base1,L2_page_offset_base1,L2_page_offset_base1,L2_load_to_ddr1};
    tmp_data.reserved[2]={0};

      relu_l2_info_ptr->l2_info.l2ctrl.data[0]=tmp_data;
      relu_l2_info_ptr->l2_info.l2ctrl.size=60;
      relu_l2_info_ptr->node_name="relu";
    L2FusionData_t relu_output={1,234,2};
      relu_l2_info_ptr->output[0]=relu_output;

    (void)ge::AttrUtils::SetBool(relu_node->GetOpDesc(), NEED_RE_PRECOMPILE, true);
      relu_node->GetOpDesc()->SetExtAttr(
            ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, relu_l2_info_ptr);


    //relu2 Info
     L2FusionInfoPtr relu2_l2_info_ptr = std::make_shared<TaskL2FusionInfo_t>();
    //data
    uint64_t L2_mirror_addr2=0;          // preload or swap source address
    uint32_t L2_data_section_size2=789;    // every data size
    uint8_t L2_preload2=0;               // 1 - preload from mirror_addr, 0 - no preload
    uint8_t modified2=1;                 // 1 - data will be modified by kernel, 0 - no modified
    uint8_t priority2=1;                 // data priority
    int8_t prev_L2_page_offset_base2=-1;  // remap source section offset
    uint8_t L2_page_offset_base2=21;      // remap destination section offset
    uint8_t L2_load_to_ddr2=0;           // 1 - need load out, 0 - no need
    rtSmData_t tmp_data2={L2_mirror_addr2,L2_data_section_size2,L2_preload2,modified2,priority2,prev_L2_page_offset_base2,L2_page_offset_base2,L2_page_offset_base2,L2_load_to_ddr2};
    tmp_data.reserved[2]={0};

      relu2_l2_info_ptr->l2_info.l2ctrl.data[0]=tmp_data;
      relu2_l2_info_ptr->l2_info.l2ctrl.size=60;
      relu2_l2_info_ptr->node_name="relu1";
    L2FusionData_t conv_output={2,567,2};
      relu2_l2_info_ptr->output[0]=conv_output;

    (void)ge::AttrUtils::SetBool(relu1_node->GetOpDesc(), NEED_RE_PRECOMPILE, true);
    relu1_node->GetOpDesc()->SetExtAttr(
            ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, relu2_l2_info_ptr);

  }
  void BuildGraphForL2Fusion1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr elemwise1 = std::make_shared<OpDesc>("elem1", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");

    SetPattern(conv, "Convolution");
    SetPattern(elemwise1, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetTvmType(conv);
    SetTvmType(elemwise);
    SetTvmType(elemwise1);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    elemwise1->AddInputDesc(out_desc);
    elemwise1->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);

    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr elemwise1_node = graph->AddNode(elemwise1);
    NodePtr relu_node = graph->AddNode(relu);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        elemwise1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise1_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));

    //elementwise l2 Info
    L2FusionInfoPtr elementwise_l2_info_ptr = std::make_shared<TaskL2FusionInfo_t>();
    //data
    uint64_t L2_mirror_addr=0;          // preload or swap source address
    uint32_t L2_data_section_size=123;    // every data size
    uint8_t L2_preload=0;               // 1 - preload from mirror_addr, 0 - no preload
    uint8_t modified=1;                 // 1 - data will be modified by kernel, 0 - no modified
    uint8_t priority=1;                 // data priority
    int8_t prev_L2_page_offset_base=-1;  // remap source section offset
    uint8_t L2_page_offset_base=0;      // remap destination section offset
    uint8_t L2_load_to_ddr=0;           // 1 - need load out, 0 - no need
    rtSmData_t tmp_data={L2_mirror_addr,L2_data_section_size,L2_preload,modified,priority,prev_L2_page_offset_base,L2_page_offset_base,L2_page_offset_base,L2_load_to_ddr};
    tmp_data.reserved[2]={0};

    elementwise_l2_info_ptr->l2_info.l2ctrl.data[0]=tmp_data;
    elementwise_l2_info_ptr->l2_info.l2ctrl.size=60;
    elementwise_l2_info_ptr->node_name="elem";
    L2FusionData_t elem_output={0,123,2};
    elementwise_l2_info_ptr->output[0]=elem_output;

    (void)ge::AttrUtils::SetBool(elemwise1_node->GetOpDesc(), NEED_RE_PRECOMPILE, true);
      elemwise1_node->GetOpDesc()->SetExtAttr(
            ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, elementwise_l2_info_ptr);

  }
  void BuildGraph2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");
    OpDescPtr netout_op = std::make_shared<OpDesc>("netoutput", "NetOutput");

    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(relu1, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
    SetTvmType(relu1);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    netout_op->AddInputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr netout_node = graph->AddNode(netout_op);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        netout_node->GetInDataAnchor(0));
  }

  void BuildGraph3(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");

    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(conv1, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(elemwise, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(relu, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(relu1, STREAM_LABEL, "stream1");

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraph4(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");

    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(conv1, STREAM_LABEL, "stream1");
    ge::AttrUtils::SetStr(elemwise, STREAM_LABEL, "stream2");
    ge::AttrUtils::SetStr(relu, STREAM_LABEL, "stream1");

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraph5(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");

    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(relu, STREAM_LABEL, "stream1");

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraph6(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "ReLU");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Convolution");

    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
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
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, STREAM_LABEL, "stream1");

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraphConvReluQuant(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "Quant");

    SetPattern(conv, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(relu);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }
  void BuildGraphConvLeakyReluQuant1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "Quant");

    SetPattern(conv, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(relu);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetFloat(relu, "negative_slope", 0);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }
  void BuildGraphConvLeakyReluQuant2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "Quant");

    SetPattern(conv, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(relu);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetFloat(relu, "negative_slope", 0.1);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }
  void BuildGraphConvEltReluQuant1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "Quant");

    SetPattern(conv, "Convolution");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
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
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }
  void BuildGraphConvEltReluQuant2(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Convolution");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "EltwiseNoFusion");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "Quant");

    SetPattern(conv, "Convolution");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
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
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        quant_node->GetInDataAnchor(0));
  }

  void BuildGraphdoubleConvEltElt(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Conv2D");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim = {4, 4, 4, 4};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    out_desc.SetOriginFormat(ge::FORMAT_NCHW);
    vector<int64_t> dim1 = {8, 8, 8, 8};
    GeShape shape1(dim1);
    GeTensorDesc out_desc1(shape1);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    conv1->AddInputDesc(out_desc1);
    conv1->AddInputDesc(out_desc1);
    conv1->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv1, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    AttrUtils::SetListInt(conv1, "ub_atomic_params", params);
    AttrUtils::SetBool(conv1, "Aipp_Conv_Flag", true);
    conv1->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv_node1 = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    ge::OpKernelBinPtr tbe_kernel_ptr1 = std::make_shared<ge::OpKernelBin>(
        conv_node1->GetName(), std::move(buffer));
    conv_node1->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr1);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node1->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node1->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraphdoubleConvEltElt_1(ComputeGraphPtr graph, int32_t reluflag) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Conv2D");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    SetPattern(conv, "Convolution");
    SetPattern(conv1, "Convolution");
    SetPattern(relu, "ElemWise");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(conv1);
    SetTvmType(elemwise);
    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim = {4, 4, 4, 4};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    out_desc.SetOriginFormat(ge::FORMAT_NHWC);
    vector<int64_t> dim1 = {8, 8, 8, 8};
    GeShape shape1(dim1);
    GeTensorDesc out_desc1(shape1);
    out_desc1.SetOriginFormat(ge::FORMAT_HWCN);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc1);
    conv->AddInputDesc(out_desc1);
    conv->AddOutputDesc(out_desc);
    conv1->AddInputDesc(out_desc);
    conv1->AddInputDesc(out_desc);
    conv1->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    relu1->AddInputDesc(out_desc);
    relu1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv1, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    AttrUtils::SetListInt(conv1, "ub_atomic_params", params);
    AttrUtils::SetBool(conv1, "Aipp_Conv_Flag", true);
    conv1->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr conv_node1 = graph->AddNode(conv1);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    ge::OpKernelBinPtr tbe_kernel_ptr1 = std::make_shared<ge::OpKernelBin>(
        conv_node1->GetName(), std::move(buffer));
    conv_node1->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr1);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node1->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_node1->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                        relu1_node->GetInDataAnchor(0));
  }

  void BuildGraphConvElt(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");

    SetPattern(conv, "Convolution");
    SetPattern(elemwise, "ElemWise");
    SetTvmType(conv);
    SetTvmType(elemwise);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape in_shape(dim);
    GeTensorDesc out_desc(in_shape);

    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddInputDesc(out_desc);
    elemwise->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetStr(conv, ge::ATTR_NAME_SESSION_GRAPH_ID, "_0_1_2_3");
    std::vector<int64_t> params = {0, 0, 0, 0, 0, 1, 0, 1};
    AttrUtils::SetListInt(conv, "ub_atomic_params", params);
    // AttrUtils::SetBool(conv, "Aipp_Conv_Flag", true);
    conv->SetWorkspaceBytes({0});
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
        conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                        conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                        elemwise_node->GetInDataAnchor(1));
  }
};

/************************
 *
 *          op    conv
 *           |     |
 *           eltiwse
 *              |
 *             op
 *
 *************************
 *conv eltw ubfusion
 *************************/
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_data_eltwise_relu) {
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
      string session_graph_id = "";
      ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
      EXPECT_EQ(session_graph_id, "_0_1_2_3");
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_data_eltwise_relu_l2_fusion) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphForL2Fusion1(graph_out, 1);
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
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.is_init_ = false;
    map<string, string> options;
    string soc_version = "Ascend910A";
    config.Initialize(options, soc_version);
    Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;

  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemelem1relu") {
        find = 1;
        L2FusionInfoPtr l2_info;
        l2_info = node->GetOpDesc()->TryGetExtAttr(
                ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, l2_info);
        if(l2_info == nullptr){
            cerr<<node->GetName()<<" does not has l2_info" << endl;
        } else {
            cerr<<" l2_info->node_name:"<< l2_info->node_name << endl;
            cerr<<" l2_info->output[0].l2Index:"<< l2_info->output[0].l2Index << endl;
            cerr<<" l2_info->output[0].l2Addr:"<< l2_info->output[0].l2Addr << endl;
            cerr<<" l2_info->output[0].l2PageNum:"<< l2_info->output[0].l2PageNum << endl;
        }
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_conv_eltwise_relu) {
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_conv_eltwise_relu3) {
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
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_conv_eltwise_relu4) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph4(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 0);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_conv_eltwise_relu5) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph5(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 0);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_conv_eltwise_relu6) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph6(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convelemrelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 0);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_relu_quant_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvReluQuant(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convreluquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_leakyrelu_quant_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvLeakyReluQuant1(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convreluquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_leakyrelu_quant_fusion_pass_no_fusion) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvLeakyReluQuant2(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "convreluquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_eltwise_relu_quant_fusion_pass) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvEltReluQuant1(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "conveltwisereluquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_eltwise_relu_quant_fusion_pass_no_fusion) {

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvEltReluQuant2(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_RELU_QUANT UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Convolution" &&
        node->GetName() == "conveltwisereluquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 0);
}

/************************
 *
 *          conv   conv
 *           |     |
 *           eltiwse
 *              |
 *            eltwise
 *
 *************************
 *conv eltw ubfusion
 *************************/
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, double_conv_data_eltwise_relu) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphdoubleConvEltElt(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        (node->GetName() == "conv1elemrelu" || node->GetName() == "convelemrelu")) {
      find = 1;
      string session_graph_id = "";
      ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
      EXPECT_EQ(session_graph_id, "_0_1_2_3");
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

/************************
 *
 *          conv   conv
 *           |     |
 *           eltiwse
 *              |
 *            eltwise
 *
 *************************
 *conv eltw ubfusion
 *************************/
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, double_conv_data_eltwise_relu_1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphdoubleConvEltElt_1(graph_out, 1);
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion before" << endl;
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
  cerr << "UB_FUSION_UT_CONV_ELT_RELU UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        (node->GetName() == "conv1elemrelu" || node->GetName() == "convelemrelu")) {
      find = 1;
      string session_graph_id = "";
      ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
      EXPECT_EQ(session_graph_id, "_0_1_2_3");
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_UT_CONV_ELT_RELU, converage) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr aipp = std::make_shared<ge::OpDesc>("aipp", "Aipp");
  ge::AttrUtils::SetInt(aipp, SCOPE_ID_ATTR, 1);
  graph->AddNode(aipp);

  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(
      "engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr,
                                     fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
}

/************************
 *
 *           conv2d
 *              |
 *           eltiwse
 *              |
 *
 *************************
 *conv eltw ubfusion
 *************************/
TEST_F(UB_FUSION_UT_CONV_ELT_RELU, conv_data_eltwise) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphConvElt(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr =
      std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr =
      std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr,
                                     fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_UT_CONV_ELT UB fusion before" << endl;
  for (auto& node : graph_out->GetDirectNode()) {
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
  cerr << "UB_FUSION_UT_CONV_ELT UB fusion result" << endl;
  for (auto& node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName()
         << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convelem") {
      find = 1;
      string session_graph_id = "";
      ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID,
                            session_graph_id);
      EXPECT_EQ(session_graph_id, "_0_1_2_3");
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }

  EXPECT_EQ(find, 1);
}
