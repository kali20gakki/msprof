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
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "common/sgt_slice_type.h"
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

class CONV2D_UB_SLICE_INFO_UNITTEST : public testing::Test {
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
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
  }

/************************
 *           x  filter  bias
 *            \    |    /
 *             \   |   /
 *                conv
 *                 |      other_input
 *   deq_scale一dequant  /
 *      (4)        |    /
 *               eltwise
 *                 |
 *             leakyrelu
 *                 |
 *               quant
 *************************/
  void BuildGraph1(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, FORMAT_NHWC, DT_FLOAT16);
    vector<int64_t> deq_dim = {4};
    GeShape deq_shape(dim);
    GeTensorDesc deq_scale(deq_shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(deq_scale);
    data3->AddOutputDesc(out_desc);
    data4->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(deq_scale);
    dequant->AddOutputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    string op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, op_slice_info);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  }

/************************
 *           x  filter  bias
 *            \    |    /
 *             \   |   /
 *                conv
 *                 |      other_input
 *   deq_scale一dequant  /
 *   (4,4,4,4)     |    /
 *               eltwise
 *                 |
 *             leakyrelu
 *                 |
 *               quant
 *************************/
  void BuildGraph2(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, FORMAT_NHWC, DT_FLOAT16);
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
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    string op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, op_slice_info);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  }

/************************
 *           x  filter  bias
 *            \    |    /
 *             \   |   /
 *                conv
 *                 |      other_input
 *   deq_scale一dequant  /
 *      (4)        |    /
 *               eltwise
 *                 |
 *              leakyrelu
 *                 |   \
 *               quant  output
 *************************/
  void BuildGraph3(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr eltwise1 = std::make_shared<OpDesc>("eltwise1", "AscendQuant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetPattern(eltwise1, "ElemWise");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    SetTvmType(eltwise1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, FORMAT_NHWC, DT_FLOAT16);
    vector<int64_t> deq_dim = {4};
    GeShape deq_shape(dim);
    GeTensorDesc deq_scale(deq_shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(deq_scale);
    data3->AddOutputDesc(out_desc);
    data4->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(deq_scale);
    dequant->AddOutputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    eltwise1->AddInputDesc(out_desc);
    eltwise1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    string op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, op_slice_info);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    NodePtr eltwise1_node = graph->AddNode(eltwise1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), eltwise1_node->GetInDataAnchor(0));
  }
    /************************
   *           x  filter  bias
   *            \    |    /
   *             \   |   /
   *                conv
   *                 |      other_input
   *   deq_scale一dequant  /
   *      (4)        |    /
   *               eltwise
   *                 |
   *              leakyrelu
   *                 |   \
   *               quant  output
   *************************/
  void BuildGraph4(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "Dequant");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr eltwise1 = std::make_shared<OpDesc>("eltwise1", "AscendQuant");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetPattern(eltwise1, "ElemWise");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    SetTvmType(eltwise1);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, FORMAT_NHWC, DT_FLOAT16);
    vector<int64_t> deq_dim = {4};
    GeShape deq_shape(dim);
    GeTensorDesc deq_scale(deq_shape);
    data->AddOutputDesc(out_desc);
    data1->AddOutputDesc(out_desc);
    data2->AddOutputDesc(deq_scale);
    data3->AddOutputDesc(out_desc);
    data4->AddOutputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddInputDesc(out_desc);
    conv->AddOutputDesc(out_desc);
    dequant->AddInputDesc(out_desc);
    dequant->AddInputDesc(deq_scale);
    dequant->AddOutputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddInputDesc(out_desc);
    eltwise->AddOutputDesc(out_desc);
    relu->AddInputDesc(out_desc);
    relu->AddOutputDesc(out_desc);
    quant->AddInputDesc(out_desc);
    quant->AddOutputDesc(out_desc);
    eltwise1->AddInputDesc(out_desc);
    eltwise1->AddOutputDesc(out_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    AttrUtils::SetInt(conv, kThreadScopeId, 1);
    AttrUtils::SetInt(dequant, kThreadScopeId, 1);
    AttrUtils::SetInt(relu, kThreadScopeId, 1);
    AttrUtils::SetInt(eltwise1, kThreadScopeId, 1);

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
    relu->SetExtAttr(ffts::kAttrSgtStructInfo, tsmp_ptr);
    eltwise1->SetExtAttr(ffts::kAttrSgtStructInfo, tsmp_ptr);

    AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    string op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, op_slice_info);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    NodePtr eltwise1_node = graph->AddNode(eltwise1);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), eltwise1_node->GetInDataAnchor(0));
  }
};

TEST_F(CONV2D_UB_SLICE_INFO_UNITTEST, conv_dequant_eltwise_leakyrelu_quant_01) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph1(graph_out);
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
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion before" << endl;
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
  string single_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" && node->GetName() == "convdequanteltwisereluquant") {
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "single op slice info is : " << single_op_slice_info << endl;
      cerr << "fusion op slice info is : " << fusion_op_slice_info << endl;
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(CONV2D_UB_SLICE_INFO_UNITTEST, conv_dequant_eltwise_leakyrelu_quant_02) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out);
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
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion before" << endl;
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
  string single_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" && node->GetName() == "convdequanteltwisereluquant") {
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "single op slice info is : " << single_op_slice_info << endl;
      cerr << "fusion op slice info is : " << fusion_op_slice_info << endl;
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(CONV2D_UB_SLICE_INFO_UNITTEST, conv_dequant_eltwise_leakyrelu_quant_03) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph3(graph_out);
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
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion before" << endl;
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
  string single_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" && node->GetName() == "convdequanteltwisereluquant") {
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "single op slice info is : " << single_op_slice_info << endl;
      cerr << "fusion op slice info is : " << fusion_op_slice_info << endl;
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(CONV2D_UB_SLICE_INFO_UNITTEST, conv_dequant_eltwise_leakyrelu_quant_05) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph4(graph_out);
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
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion before" << endl;
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
  string single_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]},{\"idx\": 4, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "CONV2D_UB_SLICE_INFO_UNITTEST UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
  cerr << "id:" << id << endl;
  uint32_t scope_id = 0;
  cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
  if (node->GetOpDesc()->GetType() == "Conv2D" && node->GetName() == "convdequanteltwisereluquant") {
  AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
  cerr << "single op slice info is : " << single_op_slice_info << endl;
  cerr << "fusion op slice info is : " << fusion_op_slice_info << endl;
  find = 1;
  }
  if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
  cerr << "scope id : " << scope_id << endl;
  }
  id++;
  }
  EXPECT_EQ(find, 1);
}