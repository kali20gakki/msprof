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
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_not_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_not_requant_fusion_pass.h"
#undef protected
#undef private

using namespace std;
using namespace fe;

static const char *RELU2 = "Relu";
static const char *RELUGRAD = "ReluGrad";
static const char *ATTR_REQUANTED  = "_requanted";
static const char *ATTR_SCALE      = "scale";
static const string OP_CONST        = "Const";
static const string OP_CONV2D        = "Conv2D";
const static std::string QUANT_BIAS_OPTIMIZATION = "QuantBiasOptimization";

class STEST_fusion_engine_requant_fusion_pass : public testing::Test {
protected:
  void SetUp() {}
  void TearDown() {}

  ge::NodePtr AddNode(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc;
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

  ge::ComputeGraphPtr CreateSucessGraph() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, RELU2);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), quant->GetInDataAnchor(0));
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    SetWeightsUint64(dequant, 0.2);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    return graph;
  }

  ge::ComputeGraphPtr CreateSucessGraphPattern1() {

    Status ret;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data       = AddNode(graph, "data", "Data");
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2D, 1, 3);
    ge::NodePtr conv1       = AddNode(graph, "conv1", OP_CONV2D, 1, 3);
    ge::NodePtr host_op    = AddNode(graph, "host", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr host_op1    = AddNode(graph, "host1", QUANT_BIAS_OPTIMIZATION);
    ge::NodePtr dequant0   = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu0      = AddNode(graph, "relu0", RELU2);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", ASCEND_DEQUANT);
    ge::NodePtr relu1      = AddNode(graph, "relu1", RELU2);
    ge::NodePtr concat     = AddNode(graph, "concat0", "ConcatD", 1, 2);
    ge::NodePtr quant      = AddNode(graph, "quant0", ASCEND_QUANT);
    AddConst2NodeInt8(graph, conv, 1);
    AddConst2NodeInt32(graph, host_op, 0);
    AddConst2NodeInt8(graph, conv1, 1);
    AddConst2NodeInt32(graph, host_op1, 0);
    SetWeightsUint64(dequant0, 0.2);
    SetWeightsUint64(dequant1, 0.000001);
    ge::AttrUtils::SetFloat(quant->GetOpDesc(), ATTR_SCALE, 0.1);

    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant0->GetOutDataAnchor(0), relu0->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu0->GetOutDataAnchor(0), concat->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(host_op1->GetOutDataAnchor(0), conv1->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), concat->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(concat->GetOutDataAnchor(0), quant->GetInDataAnchor(0));

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

  ge::ComputeGraphPtr CreateSucessGraphNotRequantV200() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr data    = AddNode(graph, "data", "DATA");
    ge::NodePtr dequant    = AddNode(graph, "dequant0", ASCEND_DEQUANT);
    ge::NodePtr relu2_node = AddNode(graph, RELU2, RELU2);
    ge::GraphUtils::AddEdge(data->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    SetWeightsUint64(dequant, 0.2);
    return graph;
  }

  static void check_result(const ge::ComputeGraphPtr graph, int value)
  {

    printf("start to check requant fusion result...\n");

    bool ret;
    int attr_value = 0;

    for(ge::NodePtr node : graph->GetAllNodes()) {

      if (node->GetType() == ASCEND_DEQUANT || node->GetType() == ASCEND_QUANT){
        ret = ge::AttrUtils::GetInt(node->GetOpDesc(), ATTR_REQUANTED, attr_value);
        EXPECT_EQ(true, ret);
        EXPECT_EQ(value, attr_value);
      }
    }

    return;
  }

};

TEST_F(STEST_fusion_engine_requant_fusion_pass, requant_pattern0_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before requant pattern0 fusion");

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 1);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, requant_pattern1_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1();

  DumpGraph(graph, "before requant pattern1 fusion");

  fe::V100RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 1);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  fe::V100NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 0);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, requant_pattern0_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before requant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 1);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, requant_pattern1_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraphPattern1();

  DumpGraph(graph, "before requant pattern1 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200RequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after requant pattern1 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 1);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, not_requant_pattern0_V200_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraphNotRequantV200();

  DumpGraph(graph, "before notrequant pattern0 fusion");

  Configuration::Instance(fe::AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;

  fe::V200NotRequantFusionPass pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, 0);
}

TEST_F(STEST_fusion_engine_requant_fusion_pass, deal_with_cube_nodes_success) {
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