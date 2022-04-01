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


#define private public
#define protected public
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_tag_no_const_folding_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_merge_conv2dbackpropinput_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_merge_weight_quant_fusion_pass.h"
#undef protected
#undef private

using namespace std;
using namespace fe;

static const char *RELUGRAD = "ReluGrad";
static const char *ATTR_REQUANTED  = "requanted";
static const char *ATTR_SCALE      = "scale";
static const string ATTR_NO_FOLDING = "no_need_constant_folding";

static const string OP_CONV2DBACKPRROPINPUT     = "Conv2DBackpropInput";
static const string OP_CONV                     = "Conv2D";
static const string OP_PAD                      = "Pad";
static const string OP_BIASADD                  = "BiasAdd";
static const string OP_CAST                     = "Cast";
static const string OP_QUANT                    = "AscendQuant";
static const string OP_ASCENDWEIGHTQUANT        = "AscendWeightQuant";
static const string OP_DEQUANT                  = "AscendDequant";
static const string OP_CONST                    = "Const";

class UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass : public testing::Test {
protected:
  void SetUp() {}
  void TearDown() {}

  ge::NodePtr AddNode(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc;
    tensor_desc.SetShape(ge::GeShape({1, 1, 1, 1}));
    tensor_desc.SetFormat(ge::FORMAT_NHWC);
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
  ge::NodePtr AddNodeShape2(ge::ComputeGraphPtr graph, const string &name, const string &type,
                      int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
    ge::GeTensorDesc tensor_desc;
    tensor_desc.SetShape(ge::GeShape({1, 1}));
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

  ge::NodePtr AddConst2Node(ge::ComputeGraphPtr graph, ge::NodePtr &node) {
    static uint32_t name_id = 0;
    float data[] = {0.1};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(name_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    node->AddLinkFrom(const_node);

    name_id++;
    return node;
  }
  ge::NodePtr AddConst2Node2(ge::ComputeGraphPtr graph, ge::NodePtr &node) {
    static uint32_t name_id = 0;
    float data[] = {0.1};
    ge::NodePtr const_node = AddNodeShape2(graph, "Const" + std::to_string(name_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    node->AddLinkFrom(const_node);

    name_id++;
    return node;
  }
  ge::NodePtr AddConst2Node(ge::ComputeGraphPtr graph, ge::NodePtr &node,
          int32_t in_anchor_index, float value = 1) {
    static uint32_t newname_id = 0;
    float data[] = {value};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }
  ge::NodePtr AddConst2NodeInt8(ge::ComputeGraphPtr graph, ge::NodePtr &node,
          int32_t in_anchor_index, int8_t value = 0) {
    static uint32_t newname_id = 0;
    int8_t data[] = {value};
    ge::NodePtr const_node = AddNode(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }
  ge::NodePtr AddConst2Node2(ge::ComputeGraphPtr graph, ge::NodePtr &node,
          int32_t in_anchor_index, float value = 1) {
    static uint32_t newname_id = 0;
    float data[] = {value};
    ge::NodePtr const_node = AddNodeShape2(graph, "Const" + std::to_string(newname_id), OP_CONST, 1, 0);
    ge::GeTensorDesc tensor_desc;
    ge::GeTensor tensor(tensor_desc, (uint8_t *) data, sizeof(data));
    ge::AttrUtils::SetTensor(const_node->GetOpDesc(), "value", tensor);

    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(in_anchor_index));
    newname_id++;
    return node;
  }

  ge::ComputeGraphPtr CreateSucessGraph() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");

    ge::NodePtr quant      = AddNode(graph, "quant", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2DBACKPRROPINPUT, 1, 3);
    ge::NodePtr biasadd    = AddNode(graph, "biasadd", OP_BIASADD, 1, 2);
    ge::NodePtr cast       = AddNode(graph, "cast", OP_CAST);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT);

    AddConst2Node(graph, conv, 0);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), biasadd->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast->GetOutDataAnchor(0), biasadd->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));

    return graph;
  }

  ge::ComputeGraphPtr CreateMergeSubSucessGraph() {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr quant      = AddNode(graph, "quant", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV, 1, 2);
    ge::NodePtr biasadd    = AddNode(graph, "biasadd", OP_BIASADD, 1, 2);
    ge::NodePtr cast       = AddNode(graph, "cast", OP_CAST);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT, 1, 2);

    AddConst2NodeInt8(graph, sub, 0);
    AddConst2NodeInt8(graph, sub, 1);
    AddConst2Node(graph, cast);
    AddConst2Node(graph, dequant, 1);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), biasadd->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast->GetOutDataAnchor(0), biasadd->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));

    return graph;
  }

  ge::ComputeGraphPtr CreateMergeSubSucessGraphNormal(vector<ge::NodePtr> &node_list) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    node_list.clear();
    ge::NodePtr quant      = AddNode(graph, "mul0", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2DBACKPRROPINPUT, 1, 3);
    ge::NodePtr biasadd    = AddNode(graph, "biasadd", OP_BIASADD, 1, 2);
    ge::NodePtr cast       = AddNode(graph, "cast", OP_CAST);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT, 1, 2);

    AddConst2NodeInt8(graph, sub, 0);
    AddConst2NodeInt8(graph, sub, 1);
    AddConst2Node(graph, cast);
    AddConst2Node(graph, conv, 0);
    AddConst2Node(graph, dequant, 1);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), biasadd->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast->GetOutDataAnchor(0), biasadd->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));

    for (auto node : graph->GetAllNodes()) {
      if (node->GetType() != OP_CONST && node->GetType() != OP_CAST) {
        node_list.push_back(node);
      }
    }

    return graph;
  }

  ge::ComputeGraphPtr CreateMergeSubSucessGraphShareSub(vector<ge::NodePtr> &node_list) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    node_list.clear();
    // shared sub
    ge::NodePtr quant      = AddNode(graph, "mul0", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2DBACKPRROPINPUT, 1, 3);
    ge::NodePtr biasadd    = AddNode(graph, "biasadd", OP_BIASADD, 1, 2);
    ge::NodePtr cast       = AddNode(graph, "cast", OP_CAST);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT, 1, 2);
    ge::NodePtr sub1       = AddNode(graph, "ascendweightquant1", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv1      = AddNode(graph, "conv1", OP_CONV, 1, 2);
    ge::NodePtr biasadd1   = AddNode(graph, "biasadd1", OP_BIASADD, 1, 2);
    ge::NodePtr cast1      = AddNode(graph, "cast1", OP_CAST);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", OP_DEQUANT, 1, 2);

    AddConst2NodeInt8(graph, sub, 0);
    AddConst2NodeInt8(graph, sub, 1);
    AddConst2Node(graph, cast);
    AddConst2Node(graph, conv, 0);
    AddConst2Node(graph, dequant, 1);
    AddConst2NodeInt8(graph, sub1, 0);
    AddConst2NodeInt8(graph, sub1, 1);
    AddConst2Node(graph, cast1);
    AddConst2Node(graph, dequant1, 1);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), biasadd->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast->GetOutDataAnchor(0), biasadd->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(sub1->GetOutDataAnchor(0), conv1->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), biasadd1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast1->GetOutDataAnchor(0), biasadd1->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));

    for (auto node : graph->GetAllNodes()) {
      if (node->GetType() != OP_CONST && node->GetType() != OP_CAST) {
        node_list.push_back(node);
      }
    }

    return graph;
  }
  ge::ComputeGraphPtr CreateMergeSubSucessGraphNoBias(vector<ge::NodePtr> &node_list) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    node_list.clear();
    ge::NodePtr quant      = AddNode(graph, "mul0", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2DBACKPRROPINPUT, 1, 3);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT, 1, 2);

    AddConst2NodeInt8(graph, sub, 0);
    AddConst2NodeInt8(graph, sub, 1);
    AddConst2Node(graph, conv, 0);
    AddConst2Node(graph, dequant, 1);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0),conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));

    for (auto node : graph->GetAllNodes()) {
      if (node->GetType() != OP_CONST && node->GetType() != OP_CAST) {
        node_list.push_back(node);
      }
    }

    return graph;
  }

  ge::ComputeGraphPtr CreateMergeSubSucessGraphShareSubNoBias(vector<ge::NodePtr> &node_list) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    node_list.clear();
    // shared sub
    ge::NodePtr quant      = AddNode(graph, "mul0", OP_QUANT);
    ge::NodePtr sub        = AddNode(graph, "ascendweightquant", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv       = AddNode(graph, "conv", OP_CONV2DBACKPRROPINPUT, 1, 3);
    ge::NodePtr biasadd    = AddNode(graph, "biasadd", OP_BIASADD, 1, 2);
    ge::NodePtr cast       = AddNode(graph, "cast", OP_CAST);
    ge::NodePtr dequant    = AddNode(graph, "dequant", OP_DEQUANT, 1, 2);
    ge::NodePtr sub1       = AddNode(graph, "ascendweightquant1", OP_ASCENDWEIGHTQUANT, 1, 2);
    ge::NodePtr conv1      = AddNode(graph, "conv1", OP_CONV, 1, 2);
    ge::NodePtr dequant1   = AddNode(graph, "dequant1", OP_DEQUANT, 1, 2);

    AddConst2NodeInt8(graph, sub, 0);
    AddConst2NodeInt8(graph, sub, 1);
    AddConst2Node(graph, cast);
    AddConst2NodeInt8(graph, conv, 0);
    AddConst2Node(graph, dequant, 1);
    AddConst2NodeInt8(graph, sub1, 0);
    AddConst2NodeInt8(graph, sub1, 1);
    AddConst2Node(graph, dequant1, 1);

    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(sub->GetOutDataAnchor(0), conv->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv->GetOutDataAnchor(0), biasadd->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(cast->GetOutDataAnchor(0), biasadd->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(biasadd->GetOutDataAnchor(0), dequant->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), conv1->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(sub1->GetOutDataAnchor(0), conv1->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv1->GetOutDataAnchor(0), dequant1->GetInDataAnchor(0));

    for (auto node : graph->GetAllNodes()) {
      if (node->GetType() != OP_CONST && node->GetType() != OP_CAST) {
        node_list.push_back(node);
      }
    }

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

  static void check_result(const ge::ComputeGraphPtr graph, const string attr_name, int value)
  {

    printf("start to check result...\n");

    bool ret;
    bool attr_value = false;

    for(ge::NodePtr node : graph->GetAllNodes()) {

      printf("nodeName: %s ", node->GetType().c_str());
      if (node->GetType() == RELU || node->GetType() == CAST || node->GetType() == CONSTANT)
        continue;
      ret = ge::AttrUtils::GetBool(node->GetOpDesc(), attr_name, attr_value);
      EXPECT_EQ(true, ret);
      EXPECT_EQ(value, attr_value);
    }

    return;
  }

};

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, pattern_success) {

  ge::ComputeGraphPtr graph = CreateSucessGraph();

  DumpGraph(graph, "before tag no const folding pattern0 fusion");

  fe::TfTagNoConstFolding pass;
  vector<fe::GraphPass *> passes = {&pass};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after tag no const folding pattern0 fusion");

  EXPECT_EQ(fe::SUCCESS, status);
  check_result(graph, ATTR_NO_FOLDING, true);
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, SetConstValueToAttr_suc1) {
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("add", "Add");;
  ge::Tensor const_tensor;
  ge::DataType dtype = ge::DT_INT32;
  std::string attr_name = "attr_name";
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  pass1.SetConstValueToAttr(op_desc, const_tensor, dtype, attr_name);
  std::vector<int32_t> const_data;
  ge::AttrUtils::GetListInt(op_desc, attr_name, const_data);
  EXPECT_EQ(0, const_data.size());
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, SetConstValueToAttr_suc2) {
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("add", "Add");;
  ge::Tensor const_tensor;
  ge::DataType dtype = ge::DT_INT64;
  std::string attr_name = "attr_name";
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  pass1.SetConstValueToAttr(op_desc, const_tensor, dtype, attr_name);
  std::vector<int32_t> const_data;
  ge::AttrUtils::GetListInt(op_desc, attr_name, const_data);
  EXPECT_EQ(0, const_data.size());
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, merge_sub_pattern_normal_success) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;

  std::vector<ge::NodePtr> node_list;
  ge::ComputeGraphPtr graph = CreateMergeSubSucessGraphNormal(node_list);

  DumpGraph(graph, "before merge_sub_pattern_success");

  fe::TfTagNoConstFolding pass;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  fe::TfMergeWeightQuantFusionPass pass2;

  // vector<fe::GraphPass *> passes = {&pass, &pass1};
  vector<fe::GraphPass *> passes = {&pass, &pass1, &pass2};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after merge_sub_pattern_success");
  for (auto node : node_list) {
    bool value = false;
    ge::AttrUtils::GetBool(node->GetOpDesc(), "no_need_constant_folding", value);
    EXPECT_EQ(true, value);
  }
  EXPECT_EQ(fe::SUCCESS, status);
  //check_result(graph, ATTR_NO_FOLDING, true);
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, merge_sub_pattern_normal_success_v200) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  std::vector<ge::NodePtr> node_list;
  ge::ComputeGraphPtr graph = CreateMergeSubSucessGraphNormal(node_list);

  DumpGraph(graph, "before merge_sub_pattern_success");

  fe::TfTagNoConstFolding pass;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  fe::TfMergeWeightQuantFusionPass pass2;

  // vector<fe::GraphPass *> passes = {&pass, &pass1};
  vector<fe::GraphPass *> passes = {&pass, &pass1, &pass2};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after merge_sub_pattern_success");
  for (auto node : node_list) {
    bool value = false;
    ge::AttrUtils::GetBool(node->GetOpDesc(), "no_need_constant_folding", value);
    EXPECT_EQ(true, value);
  }
  EXPECT_EQ(fe::SUCCESS, status);
  //check_result(graph, ATTR_NO_FOLDING, true);
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, merge_sub_pattern_sharesub_success) {

  std::vector<ge::NodePtr> node_list;
  ge::ComputeGraphPtr graph = CreateMergeSubSucessGraphShareSub(node_list);

  DumpGraph(graph, "before merge_sub_pattern_success");

  fe::TfTagNoConstFolding pass;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  fe::TfMergeWeightQuantFusionPass pass2;

  // vector<fe::GraphPass *> passes = {&pass, &pass1};
  vector<fe::GraphPass *> passes = {&pass, &pass1, &pass2};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after merge_sub_pattern_success");
  for (auto node : node_list) {
    bool value = false;
    ge::AttrUtils::GetBool(node->GetOpDesc(), "no_need_constant_folding", value);
    EXPECT_EQ(true, value);
  }

  EXPECT_EQ(fe::SUCCESS, status);
  //check_result(graph, ATTR_NO_FOLDING, true);
}
TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, merge_sub_pattern_nobias_success) {

  std::vector<ge::NodePtr> node_list;
  ge::ComputeGraphPtr graph = CreateMergeSubSucessGraphNoBias(node_list);

  DumpGraph(graph, "before merge_sub_pattern_success");

  fe::TfTagNoConstFolding pass;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  fe::TfMergeWeightQuantFusionPass pass2;

  // vector<fe::GraphPass *> passes = {&pass, &pass1};
  vector<fe::GraphPass *> passes = {&pass, &pass1, &pass2};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after merge_sub_pattern_success");
  for (auto node : node_list) {
    bool value = false;
    ge::AttrUtils::GetBool(node->GetOpDesc(), "no_need_constant_folding", value);
    EXPECT_EQ(true, value);
  }

  EXPECT_EQ(fe::SUCCESS, status);
  //check_result(graph, ATTR_NO_FOLDING, true);
}
TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, merge_sub_pattern_sharesub_nobias_success) {

  std::vector<ge::NodePtr> node_list;
  ge::ComputeGraphPtr graph = CreateMergeSubSucessGraphShareSubNoBias(node_list);

  DumpGraph(graph, "before merge_sub_pattern_success");

  fe::TfTagNoConstFolding pass;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  fe::TfMergeWeightQuantFusionPass pass2;

  // vector<fe::GraphPass *> passes = {&pass, &pass1};
  vector<fe::GraphPass *> passes = {&pass, &pass1, &pass2};
  Status status = fe::PassManager::Run(*graph, passes);

  DumpGraph(graph, "after merge_sub_pattern_success");
  for (auto node : node_list) {
    bool value = false;
    ge::AttrUtils::GetBool(node->GetOpDesc(), "no_need_constant_folding", value);
    EXPECT_EQ(true, value);
  }

  EXPECT_EQ(fe::SUCCESS, status);
  //check_result(graph, ATTR_NO_FOLDING, true);
}

TEST_F(UTEST_fusion_engine_tf_merge_conv2dbpi_fusion_pass, set_const_value_to_attr_test) {
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("add", "Add");
  ge::Tensor const_tensor;
  std::vector<uint8_t> data = {1,2,3,4,5,2,3,4,2,2,2,4,5,6,7,7,8,4,9,3,3,3,4,3,3,3,3};
  const_tensor.SetData(data);
  ge::DataType dtype = ge::DT_INT64;
  std::string attr_name;
  fe::TfMergeConv2DBackpropInputFusionPass pass1;
  pass1.SetConstValueToAttr(op_desc, const_tensor, dtype, attr_name);
}
