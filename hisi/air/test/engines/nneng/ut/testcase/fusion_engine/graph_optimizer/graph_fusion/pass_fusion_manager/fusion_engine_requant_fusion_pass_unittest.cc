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


#define protected public
#define private public
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_not_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_not_requant_fusion_pass.h"
#undef protected
#undef private

using namespace std;
using namespace fe;

static const char *RELU2 = "Relu";
static const char *RELUGRAD = "ReluGrad";
static const char *ATTR_REQUANTED  = "_requanted";
static const char *ATTR_SCALE      = "scale";
static const char *ATTR_RELU_FLAG  = "relu_flag";
static const string OP_CONST        = "Const";
static const string OP_CONV2D        = "Conv2D";
const static std::string QUANT_BIAS_OPTIMIZATION = "QuantBiasOptimization";

class UTEST_fusion_engine_requant_fusion_pass : public testing::Test {
protected:
  void SetUp() {}
  void TearDown() {}

  ge::NodePtr AddNode(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc(ge::GeShape({7}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(name, type);
    for (int32_t i = 0; i < out_anchors_num; i++) {
      opdesc->AddOutputDesc(tensor_desc);
    }
    for (int32_t i = 0; i < in_anchors_num; i++) {
      opdesc->AddInputDesc(tensor_desc);
    }
    ge::NodePtr node = graph->AddNode(opdesc);
    return node;
  }
  ge::NodePtr AddNodeAligned(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc(ge::GeShape({16, 32, 16, 16}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    tensor_desc.SetOriginShape(ge::GeShape({16, 32, 16, 16}));
    tensor_desc.SetOriginDataType(ge::DT_FLOAT);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(name, type);
    for (int32_t i = 0; i < out_anchors_num; i++) {
      opdesc->AddOutputDesc(tensor_desc);
    }
    for (int32_t i = 0; i < in_anchors_num; i++) {
      opdesc->AddInputDesc(tensor_desc);
    }
    ge::NodePtr node = graph->AddNode(opdesc);
    return node;
  }
  ge::NodePtr AddNodeONE(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(name, type);
    for (int32_t i = 0; i < out_anchors_num; i++) {
      opdesc->AddOutputDesc(tensor_desc);
    }
    for (int32_t i = 0; i < in_anchors_num; i++) {
      opdesc->AddInputDesc(tensor_desc);
    }
    ge::NodePtr node = graph->AddNode(opdesc);
    return node;
  }

  void SetWeightsFloat(ge::NodePtr node, float w) {
    float data[] = {w, w, w, w, w, w, w};
    ge::GeTensorDesc tensor_desc(ge::GeShape({7}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }

  void SetWeightsUint64(ge::NodePtr node, uint64_t w) {
    uint64_t data[] = {w};
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_UINT64);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }
  void SetWeightsUint64(ge::NodePtr node) {
    uint64_t data[] = {1};
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_UINT64);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }
  void SetWeightsInt32(ge::NodePtr node) {
    int32_t data[] = {1};
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_INT32);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }
  void SetWeightsInt8(ge::NodePtr node) {
    int8_t data[] = {1};
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_INT8);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }
  void SetWeightsInt4(ge::NodePtr node) {
    int8_t data[] = {1};
    ge::GeTensorDesc tensor_desc(ge::GeShape({1}), ge::FORMAT_NCHW, ge::DT_INT4);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }

  void SetWeightsFloat16(ge::NodePtr node, float w) {
    float data[] = {w, w, w, w, w, w, w};
    ge::GeTensorDesc tensor_desc(ge::GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::GeTensorPtr tensor = std::make_shared<ge::GeTensor>(tensor_desc, (uint8_t *) data, sizeof(data));
    vector<ge::GeTensorPtr> weights = {tensor};
    ge::OpDescUtils::SetWeights(node, weights);
    return;
  }

  ge::NodePtr AddConst2NodeInt8(ge::ComputeGraphPtr graph, ge::NodePtr &node,
                                int32_t in_anchor_index, int8_t value = 0) {
    static uint32_t newname_id = 0;
    int8_t data[] = {value};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc(ge::GeShape({1,1,1,1}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }
  ge::NodePtr AddConst2NodeInt4(ge::ComputeGraphPtr graph, ge::NodePtr &node,
                                int32_t in_anchor_index, int8_t value = 0) {
    static uint32_t newname_id = 0;
    int8_t data[] = {value};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc(ge::GeShape({1,1,1,1}), ge::FORMAT_NCHW, ge::DT_INT4);
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }

  ge::NodePtr AddConst2NodeInt32(ge::ComputeGraphPtr graph, ge::NodePtr &node,
                                int32_t in_anchor_index, int32_t value = 0) {
    static uint32_t newname_id = 0;
    int32_t data[] = {value};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }

  ge::ComputeGraphPtr CreateSucessGraph(uint32_t high_performance = 0) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, LEAKY_RELU);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    if (high_performance)
    {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
      SetWeightsUint64(dequant, 0x00001103392BCD31);
    } else {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PRECISION);
      SetWeightsUint64(dequant, 0.2);
    }
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), "offset", 0);
    ge::AttrUtils::SetFloat(relu2_node->GetOpDesc(), "negative_slope", 0);

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphWithInt4(uint32_t high_performance = 0) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, LEAKY_RELU);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    //add descriptor
    auto quant_out_desc = quant->GetOpDesc()->MutableOutputDesc(0);
    quant_out_desc->SetOriginDataType(ge::DT_INT4);
    quant_out_desc->SetDataType(ge::DT_INT4);
    auto conv_in_desc0 = conv->GetOpDesc()->MutableInputDesc(0);
    conv_in_desc0->SetOriginDataType(ge::DT_INT4);
    conv_in_desc0->SetDataType(ge::DT_INT4);
    auto conv_in_desc1 = conv->GetOpDesc()->MutableInputDesc(1);
    conv_in_desc1->SetOriginDataType(ge::DT_INT4);
    conv_in_desc1->SetDataType(ge::DT_INT4);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt4(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    if (high_performance)
    {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
      SetWeightsUint64(dequant, 0x00001103392BCD31);
    } else {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PRECISION);
      SetWeightsUint64(dequant, 0.2);
    }
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), "offset", 0);
    ge::AttrUtils::SetFloat(relu2_node->GetOpDesc(), "negative_slope", 0);

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphBiasS9() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu      = AddNodeONE(graph, "relu", "AAA");
    ge::NodePtr input      = AddNodeONE(graph, "input", ASCEND_QUANT);
    ge::NodePtr weight     = AddNodeONE(graph, "weight", CONSTANT);
    ge::NodePtr bias       = AddNode(graph, "bias", CONSTANT);
    ge::NodePtr hostop     = AddNodeONE(graph, "host", "QuantBiasOptimization");
    ge::NodePtr conv       = AddNodeONE(graph, "conv", CONV2D, 1, 3);
    ge::NodePtr dequant    = AddNodeONE(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNodeONE(graph, RELU2, LEAKY_RELU);
    ge::NodePtr quant      = AddNodeONE(graph, "quant0", ASCEND_QUANT);

    SetWeightsUint64(dequant, 0x00001103392BCD31);
    ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
    SetWeightsInt32(bias);
    SetWeightsInt32(weight);
    ge::AttrUtils::SetFloat(input->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(conv->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(input->GetOpDesc(), "offset", 0);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), "offset", 0);
    ge::AttrUtils::SetFloat(conv->GetOpDesc(), "offset", 0);
    ge::AttrUtils::SetFloat(relu2_node->GetOpDesc(), "negative_slope", 0);

    ge::GraphUtils::AddEdge(relu->GetOutDataAnchor(0), input->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(input->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(weight->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(bias->GetOutDataAnchor(0), hostop->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(hostop->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraph3Quant(int relu_num, bool same_scale) {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT, 3, 1);
    ge::NodePtr quant0     = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::NodePtr quant1     = AddNode(graph, "quant1", ASCEND_QUANT);
    ge::NodePtr quant2     = AddNode(graph, "quant2", ASCEND_QUANT);

    if (relu_num == 1) {
      relu0 = AddNode(graph, "relu", RELU2, 3, 1);
    }

    if (relu_num == 3) {
      relu0 = AddNode(graph, "relu0", RELU2);
      relu1 = AddNode(graph, "relu1", RELU2);
      relu2 = AddNode(graph, "relu2", RELU2);
    }
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant0->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant1->GetOpDesc(), ATTR_SCALE, 0.1);
    if (same_scale)
      ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.1);
    else
      ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.01);

    if (relu_num == 1) {
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(1), quant1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(2), quant2->GetInDataAnchor(0));
    } if (relu_num == 3) {
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), relu1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), relu2->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu2->GetOutDataAnchor(0), quant2->GetInDataAnchor(0));
    } else {
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), quant1->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), quant2->GetInDataAnchor(0));
    }


    return graph;
  }

  ge::ComputeGraphPtr CreateGraphNoRequant() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT, 3, 1);
    ge::NodePtr quant0     = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::NodePtr quant1     = AddNode(graph, "quant1", ASCEND_QUANT);
    ge::NodePtr quant2     = AddNode(graph, "quant2", ASCEND_QUANT);

    relu0 = AddNode(graph, "relu0", RELU2);
    relu1 = AddNode(graph, "relu1", RELU2);
    relu2 = AddNode(graph, "relu2", RELU2);


    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant0->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant1->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), relu1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), quant2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant1->GetInDataAnchor(0));

    return graph;
  }

  ge::ComputeGraphPtr CreateGraphNoRequant_LeakReluFailed() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT, 3, 1);
    ge::NodePtr quant0     = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::NodePtr quant1     = AddNode(graph, "quant1", ASCEND_QUANT);
    ge::NodePtr quant2     = AddNode(graph, "quant2", ASCEND_QUANT);

    relu0 = AddNode(graph, "relu0", RELU2);
    relu1 = AddNode(graph, "relu1", RELU2);
    relu2 = AddNode(graph, "relu2", LEAKY_RELU);

    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant0->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant1->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(relu2->GetOpDesc(), "negative_slope", 0.1);

    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), relu1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), relu2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2->GetOutDataAnchor(0), quant2->GetInDataAnchor(0));

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphWithLeakyRelu(float negative_slope) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, LEAKY_RELU);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(relu2_node->GetOpDesc(), "negative_slope", negative_slope);

    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraph3QuantWithLeakyRelu(int relu_num, bool same_scale, float negative_slope) {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT, 3, 1);
    ge::NodePtr quant0     = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::NodePtr quant1     = AddNode(graph, "quant1", ASCEND_QUANT);
    ge::NodePtr quant2     = AddNode(graph, "quant2", ASCEND_QUANT);

    if (relu_num == 1) {
      relu0 = AddNode(graph, "relu", LEAKY_RELU, 3, 1);
      ge::AttrUtils::SetFloat(relu0->GetOpDesc(), "negative_slope",negative_slope);
    }

    if (relu_num == 3) {
      relu0 = AddNode(graph, "relu0", LEAKY_RELU);
      relu1 = AddNode(graph, "relu1", LEAKY_RELU);
      relu2 = AddNode(graph, "relu2", LEAKY_RELU);
      ge::AttrUtils::SetFloat(relu0->GetOpDesc(), "negative_slope",negative_slope);
      ge::AttrUtils::SetFloat(relu1->GetOpDesc(), "negative_slope",negative_slope);
      ge::AttrUtils::SetFloat(relu2->GetOpDesc(), "negative_slope",negative_slope);
    }

    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant0->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant1->GetOpDesc(), ATTR_SCALE, 0.1);
    if (same_scale)
      ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.1);
    else
      ge::AttrUtils::SetFloat(quant2->GetOpDesc(), ATTR_SCALE, 0.01);

    if (relu_num == 1) {
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(1), quant1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(2), quant2->GetInDataAnchor(0));
    } if (relu_num == 3) {
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), relu1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), relu2->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu2->GetOutDataAnchor(0), quant2->GetInDataAnchor(0));
    } else {
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), quant0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(1), quant1->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(2), quant2->GetInDataAnchor(0));
    }
    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphPattern1WithLeakyRelu(int relu_num, bool front_relu, float negative_slope) {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;

    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNode(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNode(graph, "concat0", "ConcatD", 1, 3);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    if (relu_num == 1) {
      relu0      = AddNode(graph, "relu0", LEAKY_RELU, 1, 3);
      ge::AttrUtils::SetFloat(relu0->GetOpDesc(), "negative_slope",negative_slope);
    }

    if (relu_num == 3) {
      relu0      = AddNode(graph, "relu0", LEAKY_RELU);
      relu1      = AddNode(graph, "relu1", LEAKY_RELU);
      relu2      = AddNode(graph, "relu2", LEAKY_RELU);
      ge::AttrUtils::SetFloat(relu0->GetOpDesc(), "negative_slope",negative_slope);
      ge::AttrUtils::SetFloat(relu1->GetOpDesc(), "negative_slope",negative_slope);
      ge::AttrUtils::SetFloat(relu2->GetOpDesc(), "negative_slope",negative_slope);
    }

    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    if (front_relu) {
      if (relu_num == 1) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
      }
      if (relu_num == 3) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), relu2->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(relu2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
      }
      ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    } else {
      if (relu_num == 0) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
        ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
      } else {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
        ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
      }
    }

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphPattern1BackLeakyRelu(float negative_slope) {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;

    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNode(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNode(graph, "concat0", "ConcatD", 1, 3);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    relu0      = AddNode(graph, "relu0", LEAKY_RELU, 1, 3);
    relu1      = AddNode(graph, "relu1", LEAKY_RELU);
    ge::AttrUtils::SetFloat(relu0->GetOpDesc(), "negative_slope",
    negative_slope);
    ge::AttrUtils::SetFloat(relu1->GetOpDesc(), "negative_slope",
    negative_slope);

    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphPattern1Concat() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNodeAligned(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNodeAligned(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNodeAligned(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNodeAligned(graph, "concat0", "ConcatD", 1, 3);
    ge::NodePtr quant      = AddNodeAligned(graph, "quant0", ASCEND_QUANT);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), dequant2->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    ge::AttrUtils::SetInt(concat->GetOpDesc(), "concat_dim", 1);
    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphPattern1Concat_v200() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNodeAligned(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNodeAligned(graph, "concat0", "ConcatV2", 1, 4);
    ge::NodePtr quant      = AddNodeAligned(graph, "quant0", ASCEND_QUANT);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    AddConst2NodeInt32(graph, concat, 3, 0);
    SetWeightsUint64(dequant0, 0x0000000123456789);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv1->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv2->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetStr(dequant0->GetOpDesc(), "quant_mode", STR_QUANT_HIGH_PERFORMANCE);

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphPattern1Concat_v200_fail() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", "AnyOp");
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNodeAligned(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNodeAligned(graph, "concat0", "ConcatV2", 1, 4);
    ge::NodePtr quant      = AddNodeAligned(graph, "quant0", ASCEND_QUANT);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    AddConst2NodeInt32(graph, concat, 3, 0);
    SetWeightsUint64(dequant0, 0x0000000123456789);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv1->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetFloat(conv2->GetOpDesc(), "offset", -128);
    ge::AttrUtils::SetStr(dequant0->GetOpDesc(), "quant_mode", STR_QUANT_HIGH_PERFORMANCE);

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphNoRelu() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    SetWeightsUint64(dequant, 0.00000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphPattern1(int relu_num, bool front_relu) {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;

    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNode(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNode(graph, "concat0", "ConcatD", 1, 3);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    if (relu_num == 1) {
      relu0      = AddNode(graph, "relu0", RELU2, 1, 3);
    }

    if (relu_num == 3) {
      relu0      = AddNode(graph, "relu0", RELU2);
      relu1      = AddNode(graph, "relu1", RELU2);
      relu2      = AddNode(graph, "relu2", RELU2);
    }
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), dequant2->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    if (front_relu) {
      if (relu_num == 1) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
      }
      if (relu_num == 3) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), relu2->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(relu2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
      }
      ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    } else {
      if (relu_num == 0) {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
        ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
      } else {
        ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
        ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
        ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
      }
    }

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphPattern1BackRelu() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNode(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNode(graph, "concat0", "ConcatD", 1, 3);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    relu0      = AddNode(graph, "relu0", RELU2, 1, 3);
    relu1      = AddNode(graph, "relu1", RELU2);
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), dequant2->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphPattern1Relu6() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu6;

    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    relu6      = AddNode(graph, "relu", "Relu6");

    SetWeightsUint64(dequant0, 0.2);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), relu6->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu6->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraphPattern1Relu6Quant() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr data   = AddNode(graph, "data", "DATA");
    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu6      = AddNode(graph, "relu", "Relu6");
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), relu6->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu6->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    SetWeightsUint64(dequant0, 0x0000000212345678);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 14.5);
    return graph;
  }
  ge::ComputeGraphPtr CreateSucessGraph_FP16() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, LEAKY_RELU);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);

    SetWeightsFloat16(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);
    ge::AttrUtils::SetFloat(relu2_node->GetOpDesc(), "negative_slope", 0);

    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name)
  {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for(ge::NodePtr node : graph->GetAllNodes()) {
      printf("node name = %s.\n", node->GetName().c_str());
      for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
        for (ge::InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          printf("    node name = %s[%d], out data node name = %s[%d].\n",
                 node->GetName().c_str(),
                 anchor->GetIdx(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                 peer_in_anchor->GetIdx());
        }
      }
      if (node->GetOutControlAnchor() != nullptr) {
        for (ge::InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
          printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str());
        }
      }
    }

    return;
  }

  ge::ComputeGraphPtr CreateSucessGraphNotRequantV200(uint32_t high_performance = 0) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "DATA");
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, RELU2);
    ge::NodePtr other_node = AddNode(graph, "abc", "others");
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), other_node->GetInDataAnchor(0));
    if (high_performance)
    {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
      SetWeightsUint64(dequant, 0x00001103392BCD31);
    } else {
      ge::AttrUtils::SetStr(dequant->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PRECISION);
      SetWeightsUint64(dequant, 0.2);
    }
    return graph;
  }
  ge::ComputeGraphPtr CreateSucessConcatGraphPattern1() {

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr relu0;
    ge::NodePtr relu1;
    ge::NodePtr relu2;
    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr conv2       = AddNode(graph, "conv2", OP_CONV2D, 1, 3);
    ge::NodePtr host_op2    = AddNode(graph, "host2", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNodeAligned(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr dequant1   = AddNodeAligned(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr dequant2   = AddNodeAligned(graph, "dequant2", ASCEND_DEQUANT);
    ge::NodePtr concat     = AddNodeAligned(graph, "concat0", "ConcatV2", 1, 3);
    ge::NodePtr quant      = AddNodeAligned(graph, "quant0", ASCEND_QUANT);
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv2->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op2->GetOutDataAnchor(0), conv2->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv2->GetOutDataAnchor(0), dequant2->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    AddConst2NodeInt8(graph, conv2, 1);
    AddConst2NodeInt32(graph, host_op2, 0);
    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    SetWeightsUint64(dequant2, 0.000001);
    SetWeightsInt32(concat);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(dequant2->GetOutDataAnchor(0), concat->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

    return graph;
  }
  static void check_result(const ge::ComputeGraphPtr graph, bool has_value, int value)
  {

    printf("start to check requant fusion result...\n");

    bool ret;
    int attr_value = 0;

    for(ge::NodePtr node : graph->GetAllNodes()) {

      if (node->GetType() == ASCEND_DEQUANT || node->GetType() == ASCEND_QUANT){
        ret = ge::AttrUtils::GetInt(node->GetOpDesc(), ATTR_REQUANTED, attr_value);
        EXPECT_EQ(has_value, ret);
        EXPECT_EQ(value, attr_value);
      }
    }

    return;
  }

  static void check_result_v200(const ge::ComputeGraphPtr graph, vector<std::string> node_type)
  {

    printf("Start to check requant fusion result...\n");

    int index = 0;
    for(ge::NodePtr node : graph->GetAllNodes()) {
      EXPECT_EQ(node_type[index], node->GetType());
      index++;
    }

    return;
  }

  static void check_n_flag(const ge::ComputeGraphPtr graph, vector<uint32_t> NFlagValue)
  {
    printf("Start to check requant graph result...\n");

    int idx = 0;

    for(ge::NodePtr node : graph->GetAllNodes()) {
      if (node->GetType() == ATTR_REQUANTED){
        vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(node);
        ge::GeTensorPtr scale_input = weights[0];
        int       scale_size     = scale_input->GetData().size() / sizeof(uint64_t);
        uint64_t *scale_org_data  = (uint64_t *)(scale_input->GetData().data());
        for (int i = 0; i < scale_size; i++) {
          uint64_t scale_data = scale_org_data[i] & 0x0000001000000000;
          EXPECT_EQ(NFlagValue[idx], scale_data);
        }

        idx++;
      }
    }

    return;
  }

  static void check_relu_flag(const ge::ComputeGraphPtr graph, vector<bool> has_relu)
  {
    printf("start to check requant graph result...\n");

    bool ret;
    bool relu_flag = false;
    int idx = 0;

    for(ge::NodePtr node : graph->GetAllNodes()) {
      if (node->GetType() == ASCEND_DEQUANT){
        ret = ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
        EXPECT_EQ(true, ret);
        EXPECT_EQ(has_relu[idx], relu_flag);
        idx++;
      }
    }
    return;
  }
};

TEST_F(UTEST_fusion_engine_requant_fusion_pass, IsConstToAttrInput_suc) {

  ge::ComputeGraphPtr graph = CreateSucessConcatGraphPattern1();
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  fe::V100RequantFusionPass pass;
  ge::NodePtr concat_node = graph->FindNode("concat0");
  ge::NodePtr const_node = graph->FindNode("dequant0");
  Status ret = pass.IsConstToAttrInput(concat_node, const_node);
  EXPECT_EQ(false, ret);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_single_quant_withleakyrelu_negtiveslope) {

  ge::ComputeGraphPtr graph = CreateSucessGraphWithLeakyRelu(0.1);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_single_quant_withleakyrelu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphWithLeakyRelu(0);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_single_quant) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, deal_with_cube_nodes_success) {
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  ge::ComputeGraphPtr graph = CreateSucessGraph();
  ge::NodePtr node_ = graph->FindNode("conv");
  vector<ge::NodePtr> cube_nodes;
  cube_nodes.push_back(node_);
  bool del_bias_flag = true;
  bool no_bias_s9_flag = false;
  fe::V200RequantFusionPass pass;
  pass.DealWithCubeNodes(*graph, cube_nodes, del_bias_flag, no_bias_s9_flag);
  EXPECT_EQ(del_bias_flag, true);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_biass9) {
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  ge::ComputeGraphPtr graph = CreateSucessGraphBiasS9();

  DumpGraph(graph, "before requant pattern0 fusion");

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);

//  vector<bool> has_relu = {true};
//  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_without_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(0, true);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_failed_mul_quant_without_relu_scale_not_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(0, false);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, false, 0);

}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_single_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(1, true);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, false, 0);

}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_mul_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(3, true);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_single_leaky_relu_negtiveslope_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3QuantWithLeakyRelu(1, true, 0.1);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, false, 0);

}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_single_leaky_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3QuantWithLeakyRelu(1, true, 0);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, false, 0);

}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_mul_leaky_relu_negtiveslope_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3QuantWithLeakyRelu(3, true, 0.1);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_mul_quant_with_mul_leaky_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3QuantWithLeakyRelu(3, true, 0);

  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_no_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNoRelu();

  DumpGraph(graph, "before requant pattern0 no relu fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 no relu fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_mul_front_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(3, true);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_mul_front_leaky_relu_negtiveslope) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1WithLeakyRelu(3, true, 0.1);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_mul_front_leaky_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1WithLeakyRelu(3, true, 0);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_Single_back_leaky_relu_negtiveslope) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1WithLeakyRelu(1, false, 0.1);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_Single_back_leaky_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1WithLeakyRelu(1, false, 0);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}


TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_no_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(0, false);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_Single_back_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(1, false);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_Single_front_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(1, true);

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  vector<bool> has_relu = {false, true, false};

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_mul_dequant_Single_front_relu_back_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1BackRelu();

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_relu6) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1Relu6();

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 0);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_failed) {

  ge::ComputeGraphPtr graph = CreateGraphNoRequant();

  DumpGraph(graph, "before not_requant_pattern0_failed");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 0);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_Leaky_relu_failed) {

  ge::ComputeGraphPtr graph = CreateGraphNoRequant_LeakReluFailed();

  DumpGraph(graph, "before not_requant_pattern0_Leaky_relu_failed");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 0);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, no_requant_after_requant) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass0;
  fe::V100NotRequantFusionPass pass1;
  vector<fe::GraphPass *> passes = {&pass0, &pass1};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "AscendRequant"};
//  check_result_v200(graph, node_type);

  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success_mul_quant_without_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(0, true);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "AscendRequant"};
//  check_result_v200(graph, node_type);

  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_failed_mul_quant_without_relu_scale_not_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(0, false);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", ASCEND_DEQUANT, ASCEND_QUANT, ASCEND_QUANT, ASCEND_QUANT};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {1};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success_mul_quant_with_single_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(1, true);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);

}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success_mul_quant_with_mul_relu_scale_equal) {

  ge::ComputeGraphPtr graph = CreateSucessGraph3Quant(3, true);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "AscendRequant", "AscendRequant", "AscendRequant"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success_no_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNoRelu();

  DumpGraph(graph, "before requant pattern0 no relu fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 no relu fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "AscendRequant"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(3, true);

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "Const", "Const", "AscendRequant", "AscendRequant", "AscendRequant", "ConcatD"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success_mul_dequant_no_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(0, false);

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "Const", "Const", "AscendRequant", "AscendRequant", "AscendRequant", "ConcatD"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {false, false, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success_mul_dequant_Single_back_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(1, false);

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "Const", "Const", "AscendRequant", "AscendRequant", "AscendRequant", "ConcatD"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success_mul_dequant_Single_front_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1(1, true);

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "Const", "Const", "AscendRequant", "AscendRequant", "AscendRequant", "ConcatD"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {false, true, false};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success_mul_dequant_Single_front_relu_back_relu) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1BackRelu();

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "Const", "Const", "AscendRequant", "AscendRequant", "AscendRequant", "ConcatD"};
//  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0, 0, 0};
  check_n_flag(graph, NFlagValue);

  vector<bool> has_relu = {true, true, true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNotRequantV200();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);

  vector<std::string> node_type = {"Const", "DATA", ASCEND_DEQUANT, "others", "RequantHostCpuOpV2"};
  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, no_requant_V200_after_requant) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNotRequantV200();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  fe::V200NotRequantFusionPass pass1;
  vector<fe::GraphPass *> passes = {&pass0, &pass1};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  vector<std::string> node_type = {"Const", "DATA", ASCEND_DEQUANT, "others", "RequantHostCpuOpV2"};
  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_high_performance_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph(1);

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
//  vector<std::string> node_type = {"Const", "AscendRequant"};
//  check_result_v200(graph, node_type);

  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, no_requant_V200_high_performance_after_requant) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNotRequantV200(1);

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  fe::V200NotRequantFusionPass pass1;
  vector<fe::GraphPass *> passes = {&pass0, &pass1};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  vector<std::string> node_type = {"Const", "DATA", ASCEND_DEQUANT, "others", "RequantHostCpuOpV2"};
  check_result_v200(graph, node_type);

  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, Requant_V100_fp16) {

  ge::ComputeGraphPtr graph = CreateSucessGraph_FP16();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100RequantFusionPass pass0;
  fe::V100NotRequantFusionPass pass1;
  vector<fe::GraphPass *> passes = {&pass0, &pass1};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  vector<std::string> node_type = {"Const", ASCEND_DEQUANT, ASCEND_QUANT};
  check_result_v200(graph, node_type);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_cancat) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1Concat();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  vector<fe::GraphPass *> passes = {&pass0};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_cancatv2) {

  ge::ComputeGraphPtr graph = CreateSucessConcatGraphPattern1();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  vector<fe::GraphPass *> passes = {&pass0};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_cancatv2_v200) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1Concat_v200();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  vector<fe::GraphPass *> passes = {&pass0};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after notrequant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_cancatv2_v200_fail) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1Concat_v200_fail();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass0;
  vector<fe::GraphPass *> passes = {&pass0};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after notrequant pattern0 fusion");

  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern1_success_relu6_quant) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1Relu6Quant();

  DumpGraph(graph, "before requant pattern1 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetAllNodes()) {
    printf("node %s \ttype is %s.\n", node->GetName().c_str(), node->GetType().c_str());
  }
  vector<std::string> node_type = {"Const", "DATA", ASCEND_DEQUANT, "Relu6D", ASCEND_QUANT, "RequantHostCpuOpV2"};
  check_result_v200(graph, node_type);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_single_quant_with_int4_v200) {
  ge::ComputeGraphPtr graph = CreateSucessGraphWithInt4();
  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 fusion");
  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);
  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_success_single_quant_with_int4_v100) {
  ge::ComputeGraphPtr graph = CreateSucessGraphWithInt4();
  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 fusion");
  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);
  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_failed_single_quant_with_int4_v200) {
  ge::ComputeGraphPtr graph = CreateSucessGraphWithInt4(1);
  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 fusion");
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_int4_v210) {
  ge::ComputeGraphPtr graph = CreateSucessGraphWithInt4();
  DumpGraph(graph, "before requant pattern0 fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V210;
  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 fusion");
  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, true, 1);
  vector<bool> has_relu = {true};
  check_relu_flag(graph, has_relu);
  for (auto node : graph->GetDirectNode()) {
    if (node->GetType() == "AscendRequant") {
      ge::DataType out_datatype = node->GetOpDesc()->GetOutputDesc(0).GetDataType();
      EXPECT_EQ(out_datatype, ge::DT_INT4);
    }
  }
}

TEST_F(UTEST_fusion_engine_requant_fusion_pass, requant_pattern0_V210_success_no_relu) {
  ge::ComputeGraphPtr graph = CreateSucessGraphNoRelu();
  DumpGraph(graph, "before requant pattern0 no relu fusion");
  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V210;
  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);
  DumpGraph(graph, "after requant pattern0 no relu fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  vector<uint32_t> NFlagValue = {0};
  check_n_flag(graph, NFlagValue);
  vector<bool> has_relu = {false};
  check_relu_flag(graph, has_relu);
}
