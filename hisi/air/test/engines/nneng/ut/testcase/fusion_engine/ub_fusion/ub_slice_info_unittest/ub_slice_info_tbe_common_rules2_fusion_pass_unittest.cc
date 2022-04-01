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
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph_optimizer/op_setter/op_setter.h"
#include "ops_store/ops_kernel_manager.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

using OpSetterPtr = std::shared_ptr<OpSetter>;
class COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST : public testing::Test {
public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }

  static void TearDownTestCase() { std::cout << "UB fusion TearDown" << std::endl; }

  void SetUp()
  {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
    FEOpsStoreInfo tbe_custom {
            6,
            "tbe-custom",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_slice_op_info/slice_success",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
    fe_ops_kernel_info_store_ptr_->Initialize(options);
  }

  virtual void TearDown() {}

  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }

  void SetTvmType(ge::OpDescPtr opdef) {
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
  }

  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;


  /************************
   *          x
   *          \
   *          read  filter  bias
   *            \    |    /
   *             \   |   /
   *                conv
   *                 |      other_input
   *   deq_scale一dequant  /
   *                 |    /
   *               eltwise
   *                 |    \
   *             leakyrelu \
   *                 |    end2
   *               quant
   *                 |
   *               write
   *                 |
   *               end1
   *************************/
  // {"_fusion_op_slice_info":{"l1FusionEnable":2,"minTbeL1Space":0,"reduceMaps":[],
  // "splitMaps":[{"inputList":[{"axis":[0],"headOverLap":[-1],"idx":0,"tailOverLap":[-1]},{"axis":[0],"headOverLap":[],"idx":4,"tailOverLap":[]}],
  //               "outputList":[{"axis":[0],"idx":0},{"axis":[0],"idx":1}]}]}}

  void BuildGraphForTbeCommonRules2FusionPassOKCase1(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr read = std::make_shared<OpDesc>("read", "StridedRead");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr write = std::make_shared<OpDesc>("write", "StridedWrite");
    OpDescPtr end1 = std::make_shared<OpDesc>("end1", "End");
    OpDescPtr end2 = std::make_shared<OpDesc>("end2", "End");
    SetPattern(read, "strided_read");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetPattern(write, "strided_write");
    SetTvmType(read);
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    SetTvmType(write);
    ge::AttrUtils::SetInt(read, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(relu, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(quant, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(write, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    // add descriptor
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_bias(GeShape({512}), ge::FORMAT_NCHW, ge::DT_INT32);
    conv_tensor_desc_bias.SetOriginShape(GeShape({512}));
    conv_tensor_desc_bias.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc deq_scale_desc(GeShape({1, 16, 1, 1, 32}), ge::FORMAT_NC1HWC0, ge::DT_UINT64);
    deq_scale_desc.SetOriginShape(GeShape({512}));
    deq_scale_desc.SetOriginFormat(ge::FORMAT_NCHW);

    data->AddOutputDesc(conv_tensor_desc);
    data1->AddOutputDesc(conv_tensor_desc_weight);
    data2->AddOutputDesc(conv_tensor_desc_bias);
    data3->AddOutputDesc(conv_tensor_desc);
    data4->AddOutputDesc(conv_tensor_desc);
    read->AddInputDesc(conv_tensor_desc);
    read->AddOutputDesc(conv_tensor_desc_weight);
    conv->AddInputDesc(conv_tensor_desc);
    conv->AddInputDesc(conv_tensor_desc_weight);
    conv->AddInputDesc(conv_tensor_desc_bias);
    conv->AddOutputDesc(conv_tensor_desc);
    dequant->AddInputDesc(conv_tensor_desc);
    dequant->AddInputDesc(deq_scale_desc);
    dequant->AddOutputDesc(conv_tensor_desc);
    eltwise->AddInputDesc(conv_tensor_desc);
    eltwise->AddInputDesc(conv_tensor_desc);
    eltwise->AddOutputDesc(conv_tensor_desc);
    relu->AddInputDesc(conv_tensor_desc);
    relu->AddOutputDesc(conv_tensor_desc);
    quant->AddInputDesc(conv_tensor_desc);
    quant->AddOutputDesc(conv_tensor_desc);
    write->AddInputDesc(conv_tensor_desc);
    write->AddOutputDesc(conv_tensor_desc);
    end1->AddInputDesc(conv_tensor_desc);
    end1->AddOutputDesc(conv_tensor_desc);
    end2->AddInputDesc(conv_tensor_desc);
    end2->AddOutputDesc(conv_tensor_desc);

    string conv_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, conv_op_slice_info);
    string dequant_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 1, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(dequant, OP_SLICE_INFO, dequant_op_slice_info);
    string quant_op_slice_info = dequant_op_slice_info;
    AttrUtils::SetStr(quant, OP_SLICE_INFO, quant_op_slice_info);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr read_node = graph->AddNode(read);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    NodePtr write_node = graph->AddNode(write);
    NodePtr end1_node = graph->AddNode(end1);
    NodePtr end2_node = graph->AddNode(end2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(read_node->GetName(), std::move(buffer));
    read_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), read_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(read_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), write_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(write_node->GetOutDataAnchor(0), end1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), end2_node->GetInDataAnchor(0));
  }
  /************************
   * Ref: (StrideRead) + conv2_d + (dequant) + ele-wise*N + (quant) + (StrideWrite)
   *           x    filter  bias
   *            \    |    /
   *             \   |   /
   *                conv
   *                 |      other_input
   *   deq_scale一dequant  /
   *              /  |    /
   *             / eltwise
   *         end2    |    other_input
   *              eltwise /
   *                |    /
   *              eltwise
   *                |
   *             leakyrelu
   *                 |
   *               quant
   *                 |
   *               end1
   *************************/
  // {"_fusion_op_slice_info":{"l1FusionEnable":2,"minTbeL1Space":0,"reduceMaps":[],"splitMaps":[
  // {"inputList":[{"axis":[3],"headOverLap":[0],"idx":0,"tailOverLap":[0]},{"axis":[3],"headOverLap":[],"idx":4,"tailOverLap":[]},
  // {"axis":[3],"headOverLap":[],"idx":5,"tailOverLap":[]}],"outputList":[{"axis":[3],"idx":0},{"axis":[3],"idx":1}]},
  // {"inputList":[{"axis":[2],"headOverLap":[0],"idx":0,"tailOverLap":[0]},{"axis":[2],"headOverLap":[],"idx":4,"tailOverLap":[]},
  // {"axis":[2],"headOverLap":[],"idx":5,"tailOverLap":[]}],"outputList":[{"axis":[2],"idx":0},{"axis":[2],"idx":1}]},
  // {"inputList":[{"axis":[0],"headOverLap":[-1],"idx":0,"tailOverLap":[-1]},{"axis":[0],"headOverLap":[],"idx":4,"tailOverLap":[]},
  // {"axis":[0],"headOverLap":[],"idx":5,"tailOverLap":[]}],"outputList":[{"axis":[0],"idx":0},{"axis":[0],"idx":1}]}]}}
  void BuildGraphForTbeCommonRules2FusionPassOKCase2(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr data5 = std::make_shared<OpDesc>("DATA5", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr eltwise1 = std::make_shared<OpDesc>("eltwise1", "Eltwise");
    OpDescPtr eltwise2 = std::make_shared<OpDesc>("eltwise2", "Eltwise");
    OpDescPtr eltwise3 = std::make_shared<OpDesc>("eltwise3", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr end1 = std::make_shared<OpDesc>("end1", "End");
    OpDescPtr end2 = std::make_shared<OpDesc>("end2", "End");
    SetPattern(conv, "Convolution");
    SetPattern(dequant, "dequant");
    SetPattern(eltwise1, "ElemWise");
    SetPattern(eltwise2, "ElemWise");
    SetPattern(eltwise3, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(dequant);
    SetTvmType(eltwise1);
    SetTvmType(eltwise2);
    SetTvmType(eltwise3);
    SetTvmType(relu);
    SetTvmType(quant);
    ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(eltwise1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(eltwise2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(eltwise3, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(relu, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(quant, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    // add descriptor
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_bias(GeShape({512}), ge::FORMAT_NCHW, ge::DT_INT32);
    conv_tensor_desc_bias.SetOriginShape(GeShape({512}));
    conv_tensor_desc_bias.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc deq_scale_desc(GeShape({1, 16, 1, 1, 32}), ge::FORMAT_NC1HWC0, ge::DT_UINT64);
    deq_scale_desc.SetOriginShape(GeShape({512}));
    deq_scale_desc.SetOriginFormat(ge::FORMAT_NCHW);

    data->AddOutputDesc(conv_tensor_desc);
    data1->AddOutputDesc(conv_tensor_desc_weight);
    data2->AddOutputDesc(conv_tensor_desc_bias);
    data3->AddOutputDesc(conv_tensor_desc);
    data4->AddOutputDesc(conv_tensor_desc);
    data5->AddOutputDesc(conv_tensor_desc);
    conv->AddInputDesc(conv_tensor_desc);
    conv->AddInputDesc(conv_tensor_desc_weight);
    conv->AddInputDesc(conv_tensor_desc_bias);
    conv->AddOutputDesc(conv_tensor_desc);
    dequant->AddInputDesc(conv_tensor_desc);
    dequant->AddInputDesc(deq_scale_desc);
    dequant->AddOutputDesc(conv_tensor_desc);
    eltwise1->AddInputDesc(conv_tensor_desc);
    eltwise1->AddInputDesc(conv_tensor_desc);
    eltwise1->AddOutputDesc(conv_tensor_desc);
    eltwise2->AddInputDesc(conv_tensor_desc);
    eltwise2->AddOutputDesc(conv_tensor_desc);
    eltwise3->AddInputDesc(conv_tensor_desc);
    eltwise3->AddInputDesc(conv_tensor_desc);
    eltwise3->AddOutputDesc(conv_tensor_desc);
    relu->AddInputDesc(conv_tensor_desc);
    relu->AddOutputDesc(conv_tensor_desc);
    quant->AddInputDesc(conv_tensor_desc);
    quant->AddOutputDesc(conv_tensor_desc);
    end1->AddInputDesc(conv_tensor_desc);
    end1->AddOutputDesc(conv_tensor_desc);
    end2->AddInputDesc(conv_tensor_desc);
    end2->AddOutputDesc(conv_tensor_desc);
    string conv_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, conv_op_slice_info);
    string dequant_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 1, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(dequant, OP_SLICE_INFO, dequant_op_slice_info);
    string quant_op_slice_info = dequant_op_slice_info;
    AttrUtils::SetStr(quant, OP_SLICE_INFO, quant_op_slice_info);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr data4_node = graph->AddNode(data4);
    NodePtr data5_node = graph->AddNode(data5);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr dequant_node = graph->AddNode(dequant);
    NodePtr eltwise1_node = graph->AddNode(eltwise1);
    NodePtr eltwise2_node = graph->AddNode(eltwise2);
    NodePtr eltwise3_node = graph->AddNode(eltwise3);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    NodePtr end1_node = graph->AddNode(end1);
    NodePtr end2_node = graph->AddNode(end2);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), eltwise1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data4_node->GetOutDataAnchor(0), eltwise1_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise1_node->GetOutDataAnchor(0), eltwise2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(eltwise2_node->GetOutDataAnchor(0), eltwise3_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data5_node->GetOutDataAnchor(0), eltwise3_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise3_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), end1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), end2_node->GetInDataAnchor(0));
  }

  /************************
   * Ref: (StrideRead) + conv2_d + ele-wise*N + (StrideWrite)
   *           x    filter  bias
   *            \    |    /
   *             \   |   /
   *                conv
   *                 |  other_input
   *                 |    /
   *               eltwise
   *                 |
   *             leakyrelu ---- end2
   *                 |
   *               quant
   *                 |
   *               end1
   *************************/
  void BuildGraphForTbeCommonRules2FusionPassOKCase3  (ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr eltwise = std::make_shared<OpDesc>("eltwise", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "LeakyRelu");
    OpDescPtr quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr end1 = std::make_shared<OpDesc>("end1", "End");
    OpDescPtr end2 = std::make_shared<OpDesc>("end2", "End");
    SetPattern(conv, "Convolution");
    SetPattern(eltwise, "ElemWise");
    SetPattern(relu, "ElemWise");
    SetPattern(quant, "quant");
    SetTvmType(conv);
    SetTvmType(eltwise);
    SetTvmType(relu);
    SetTvmType(quant);
    ge::AttrUtils::SetInt(data, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(data1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(data2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(data3, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(relu, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(quant, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    // add descriptor
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_bias(GeShape({512}), ge::FORMAT_NCHW, ge::DT_INT32);
    conv_tensor_desc_bias.SetOriginShape(GeShape({512}));
    conv_tensor_desc_bias.SetOriginFormat(ge::FORMAT_NCHW);

    data->AddOutputDesc(conv_tensor_desc);
    data1->AddOutputDesc(conv_tensor_desc_weight);
    data2->AddOutputDesc(conv_tensor_desc_bias);
    data3->AddOutputDesc(conv_tensor_desc_bias);
    conv->AddInputDesc(conv_tensor_desc);
    conv->AddInputDesc(conv_tensor_desc_weight);
    conv->AddInputDesc(conv_tensor_desc_bias);
    conv->AddOutputDesc(conv_tensor_desc);
    eltwise->AddInputDesc(conv_tensor_desc);
    eltwise->AddInputDesc(conv_tensor_desc);
    eltwise->AddOutputDesc(conv_tensor_desc);
    relu->AddInputDesc(conv_tensor_desc);
    relu->AddOutputDesc(conv_tensor_desc);
    quant->AddInputDesc(conv_tensor_desc);
    quant->AddOutputDesc(conv_tensor_desc);
    end1->AddInputDesc(conv_tensor_desc);
    end1->AddOutputDesc(conv_tensor_desc);
    end2->AddInputDesc(conv_tensor_desc);
    end2->AddOutputDesc(conv_tensor_desc);
    AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(eltwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    string conv_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [0], \"tailOverLap\": [0]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}, {\"inputList\": [{\"idx\": 1, \"axis\": [1], \"headOverLap\": [-1], \"tailOverLap\": [-1]}, {\"idx\": 2, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [1]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 2, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(conv, OP_SLICE_INFO, conv_op_slice_info);
    string quant_op_slice_info = "{\"_op_slice_info\": {\"splitMaps\": [{\"inputList\": [{\"idx\": 0, \"axis\": [0], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [0]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [2], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [2]}]}, {\"inputList\": [{\"idx\": 0, \"axis\": [3], \"headOverLap\": [-1], \"tailOverLap\": [-1]}], \"outputList\": [{\"idx\": 0, \"axis\": [3]}]}], \"reduceMaps\": [], \"l1FusionEnable\": 1, \"minTbeL1Space\": 0}}";
    AttrUtils::SetStr(quant, OP_SLICE_INFO, quant_op_slice_info);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr data3_node = graph->AddNode(data3);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr eltwise_node = graph->AddNode(eltwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr quant_node = graph->AddNode(quant);
    NodePtr end1_node = graph->AddNode(end1);
    NodePtr end2_node = graph->AddNode(end2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
    conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data3_node->GetOutDataAnchor(0), eltwise_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(eltwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), end1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), end2_node->GetInDataAnchor(0));
  }
};

TEST_F(COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST, pass_case1) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphForTbeCommonRules2FusionPassOKCase1(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // set op slice info for each op
  OpSetterPtr op_setter_ptr = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_setter_ptr->SetOpInfo(*(graph_out.get()));
  EXPECT_EQ(fe::SUCCESS, ret);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

  uint32_t id = 0;
  string op_slice_info;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case1 UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
    cerr << "op slice info is :   " << endl << op_slice_info << endl;
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

  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case1 UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr  << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" && node->GetName() == "readconvdequanteltwisereluquantwrite") {
      AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
      cerr << "op slice info is :   " << endl << op_slice_info << endl;
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "fusion op slice info is :   " << endl << fusion_op_slice_info << endl;
      find = 1;
    }
  }
  EXPECT_EQ(find, 1);
}
TEST_F(COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST, pass_case2) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphForTbeCommonRules2FusionPassOKCase2(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // set op slice info for each op
  OpSetterPtr op_setter_ptr = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_setter_ptr->SetOpInfo(*(graph_out.get()));
  EXPECT_EQ(fe::SUCCESS, ret);

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;
  string op_slice_info;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case2 UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
    cerr << "op slice info is :   " << endl << op_slice_info << endl;
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

  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case2 UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr  <<  "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D") {
      AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
      cerr << "op slice info is :   " << endl << op_slice_info << endl;
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "fusion op slice info is :   " << endl << fusion_op_slice_info << endl;
      find = 1;
    }
  }
  EXPECT_EQ(find, 1);
}

TEST_F(COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST, pass_case3) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraphForTbeCommonRules2FusionPassOKCase3(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // set op slice info for each op
  OpSetterPtr op_setter_ptr = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_setter_ptr->SetOpInfo(*(graph_out.get()));
  EXPECT_EQ(fe::SUCCESS, ret);

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;
  string op_slice_info;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case3 UB fusion before" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
    cerr << "op slice info is :   " << endl << op_slice_info << endl;
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

  string fusion_op_slice_info;
  int find = 0;
  cerr << endl;
  cerr << "COMMON_RULES2_UB_FUSION_SLICE_INFO_UNITTEST::pass_case3 UB fusion result" << endl;
  for (auto &node : graph_out->GetDirectNode()) {
    cerr  <<  "name: " << node->GetName() << ", type:" << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D") {
      AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
      cerr << "op slice info is :   " << endl << op_slice_info << endl;
      AttrUtils::GetStr(node->GetOpDesc(), FUSION_OP_SLICE_INFO, fusion_op_slice_info);
      cerr << "fusion op slice info is :   " << endl << fusion_op_slice_info << endl;
      find = 1;
    }
  }
  EXPECT_EQ(find, 1);
}