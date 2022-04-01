/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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
#include <string>
#include <vector>

#define protected public
#define private public
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_pre_quant_pass.h"
#include "platform_info.h"
#undef protected
#undef private
using namespace std;
using namespace ge;
using namespace fe;
namespace fe {
const std::string GRAPH_NAME = "test_graph";
const std::string ATTR_OFFSET = "offset";
const std::string ATTR_SCALE = "scale";

class fixpipe_pass_ut : public testing::Test {
 public:
 protected:
  void SetUp() {}
  void TearDown() {}
  static void DumpTestGraph(ge::ComputeGraphPtr graph) {
    std::cout << "fixpipe_test : graph_name = " << graph->GetName() << std::endl;
    for (const auto &node : graph->GetAllNodes()) {
      std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType() << std::endl;
      for (const auto &anchor : node->GetAllOutDataAnchors()) {
        for (const auto &peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          if (peer_in_anchor != nullptr && peer_in_anchor->GetOwnerNode() != nullptr) {
            std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType()
                      << " outdatanode name = " << peer_in_anchor->GetOwnerNode()->GetName()
                      << " type = " << peer_in_anchor->GetOwnerNode()->GetType() << std::endl;
          }
        }
      }
      auto out_control_anchor = node->GetOutControlAnchor();
      if (out_control_anchor != nullptr) {
        for (const auto &peer_in_anchor : out_control_anchor->GetPeerInControlAnchors()) {
          if (peer_in_anchor != nullptr && peer_in_anchor->GetOwnerNode() != nullptr) {
            std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType()
                      << " outcontrolnode name = " << peer_in_anchor->GetOwnerNode()->GetName()
                      << " type = " << peer_in_anchor->GetOwnerNode()->GetType() << std::endl;
          }
        }
      }
    }
  }
  static void PrintPass(FixPipePassInfo &m_pass) {
    std::cout << "    fixpipe_test matchedpass passid = " << std::dec << m_pass.pass_index << std::endl;
    for (auto node : m_pass.m_opnodes) {
      std::cout << "  fixpipe_test node_name = " << node.GetNode()->GetName()
                << "  type = " << node.GetNode()->GetType() << std::endl;
    }
  }

  static void CreateEdges(ge::NodePtr inputnode, ge::NodePtr outnode, size_t out_index, size_t in_index) {
    if (inputnode->GetOutDataAnchor(out_index) == nullptr) {
      std::cout << "fixpipe_test :inputnode outdataanchor = null" << std::endl;
      return;
    }
    if (outnode->GetInDataAnchor(in_index) == nullptr) {
      std::cout << "fixpipe_test :outputnode indataanchor = null" << std::endl;
      return;
    }
    if (inputnode->GetOutControlAnchor() == nullptr) {
      std::cout << "fixpipe_test :inputnode outcontrolanchor = null" << std::endl;
      return;
    }
    if (outnode->GetInControlAnchor() == nullptr) {
      std::cout << "fixpipe_test :inputnode incontrolanchor = null" << std::endl;
      return;
    }
    ge::GraphUtils::AddEdge(inputnode->GetOutDataAnchor(out_index), outnode->GetInDataAnchor(in_index));
    ge::GraphUtils::AddEdge(inputnode->GetOutControlAnchor(), outnode->GetInControlAnchor());
  }

  static ge::NodePtr CreateConstNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_tensor_desc.SetDataType(ge::DT_FLOAT);
    ge::GeTensorPtr dequant_input = std::make_shared<ge::GeTensor>();
    std::vector<float> inputv01;
    for (int i = 0; i != 3; i++) {
      for (int j = 0; j != 4; j++) {
        for (int k = 0; k != 5; k++) {
          for (int m = 0; m != 6; m++) {
            float dst = i * j * k * m;
            inputv01.push_back(dst);
          }
        }
      }
    }
    std::cout << "fixpipe_test inputv01 size = " << inputv01.size() << std::endl;
    dequant_input->SetData(reinterpret_cast<uint8_t *>(inputv01.data()), 1440);
    ge::OpDescPtr const_opdesc = std::make_shared<ge::OpDesc>(opname, "Const");
    ge::AttrUtils::SetTensor(const_opdesc, ge::ATTR_NAME_WEIGHTS, dequant_input);
    const_opdesc->AddOutputDesc(conv_tensor_desc);
    const_opdesc->AddInputDesc(conv_tensor_desc);
    auto const_node = graph->AddNode(const_opdesc);
    return const_node;
  }

  static ge::NodePtr CreateDequantNode(ge::ComputeGraphPtr graph, std::string opname, bool hasattr = false,
                                       float scale = 0.0, float offset = 0.0) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::GeTensorPtr dequant_input = std::make_shared<ge::GeTensor>();
    std::vector<float> inputv01;
    for (int i = 0; i != 3; i++) {
      for (int j = 0; j != 4; j++) {
        for (int k = 0; k != 5; k++) {
          for (int m = 0; m != 6; m++) {
            float dst = i * j * k * m;
            inputv01.push_back(dst);
          }
        }
      }
    }
    std::cout << "fixpipe_test inputv01 size = " << inputv01.size() << std::endl;
    dequant_input->SetData(reinterpret_cast<uint8_t *>(inputv01.data()), 1440);
    ge::OpDescPtr const_opdesc = std::make_shared<OpDesc>(opname + "_constinput", "Const");
    ge::AttrUtils::SetTensor(const_opdesc, ge::ATTR_NAME_WEIGHTS, dequant_input);
    const_opdesc->AddOutputDesc(conv_tensor_desc);
    const_opdesc->AddInputDesc(conv_tensor_desc);
    auto const_node = graph->AddNode(const_opdesc);

    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendDequant");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    if (hasattr) {
      ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
      ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    }
    auto node = graph->AddNode(opdesc);
    CreateEdges(const_node, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateDequant2Node(ge::ComputeGraphPtr graph, std::string opname, bool hasattr = false,
                                        float scale = 0.0, float offset = 0.0) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_tensor_desc.SetDataType(ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<OpDesc>(opname, "AscendDequant");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    if (hasattr) {
      ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
      ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    }
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateQuantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendQuant");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    float offset = 8.0;
    float scale = 4.0;
    ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
    ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateRequantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendRequant");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_constinput");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateCastNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Cast");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateAntiQuantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendAntiQuant");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    float offset = 8.0;
    float scale = 4.0;
    ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
    ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_inputconstnode");
    CreateEdges(constnode, node, 0, 0);
    return node;
  }

  static ge::NodePtr CreateAddNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto antinode = CreateAntiQuantNode(graph, opname + "_inputantinode");
    CreateEdges(antinode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateAdd2Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_inputconstnode");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateAdd3Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_FRACTAL_NZ);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateAdd4Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_ND);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateSubNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Sub");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto antinode = CreateAntiQuantNode(graph, opname + "_inputantinode");
    CreateEdges(antinode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Relu");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreatePReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "PRelu");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_constinput");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreatePRelu2Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "PRelu");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateLReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "LeakyRelu");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    float negative_slope = 8.0f;
    ge::AttrUtils::SetFloat(opdesc, "negative_slope", negative_slope);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateRelu6Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Relu6");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateTransFormNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_ND);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "TransData");
    opdesc->AddInputDesc(conv_tensor_desc);
    conv_tensor_desc.SetFormat(ge::FORMAT_ND);
    opdesc->AddOutputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateCubeNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3,1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Conv2D");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateOtherNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc conv_tensor_desc(GeShape({3,1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "MaxPool");
    opdesc->AddOutputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    opdesc->AddInputDesc(conv_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateNodeSimpleFactoryTemplate(ge::ComputeGraphPtr graph, std::string opname,
                                                    std::string opuinttype) {
    if (opuinttype == "A") {
      return CreateCubeNode(graph, opname);
    } else if (opuinttype == "B") {
      return CreateDequantNode(graph, opname);
    } else if (opuinttype == "C") {
      return CreateReluNode(graph, opname);
    } else if (opuinttype == "D") {
      return CreateAddNode(graph, opname);
    } else if (opuinttype == "E") {
      return CreateQuantNode(graph, opname);
    } else if (opuinttype == "F") {
      return CreateTransFormNode(graph, opname);
    } else {
      return CreateOtherNode(graph, opname);
    }
  }

  static ge::NodePtr CreateTestCase01Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateNodeSimpleFactoryTemplate(graph, "1", "A");
    auto node2 = CreateNodeSimpleFactoryTemplate(graph, "2", "G");
    auto node3 = CreateNodeSimpleFactoryTemplate(graph, "3", "G");
    auto node4 = CreateNodeSimpleFactoryTemplate(graph, "4", "A");
    auto node5 = CreateNodeSimpleFactoryTemplate(graph, "5", "C");
    auto node6 = CreateNodeSimpleFactoryTemplate(graph, "6", "E");
    auto node7 = CreateNodeSimpleFactoryTemplate(graph, "7", "F");
    auto node8 = CreateNodeSimpleFactoryTemplate(graph, "8", "C");

    auto node31 = CreateNodeSimpleFactoryTemplate(graph, "31", "B");
    auto node51 = CreateNodeSimpleFactoryTemplate(graph, "51", "B");
    auto node61 = CreateNodeSimpleFactoryTemplate(graph, "61", "E");
    auto node71 = CreateNodeSimpleFactoryTemplate(graph, "71", "D");
    auto node72 = CreateNodeSimpleFactoryTemplate(graph, "72", "D");
    auto node73 = CreateNodeSimpleFactoryTemplate(graph, "73", "A");

    auto node711 = CreateNodeSimpleFactoryTemplate(graph, "711", "B");
    auto node712 = CreateNodeSimpleFactoryTemplate(graph, "712", "C");
    auto node713 = CreateNodeSimpleFactoryTemplate(graph, "713", "D");
    auto node714 = CreateNodeSimpleFactoryTemplate(graph, "714", "D");

    auto node7131 = CreateNodeSimpleFactoryTemplate(graph, "7131", "C");
    auto node7132 = CreateNodeSimpleFactoryTemplate(graph, "7132", "F");
    auto node7141 = CreateNodeSimpleFactoryTemplate(graph, "7141", "E");
    auto node7142 = CreateNodeSimpleFactoryTemplate(graph, "7142", "G");

    auto node71311 = CreateNodeSimpleFactoryTemplate(graph, "71311", "E");
    auto node71321 = CreateNodeSimpleFactoryTemplate(graph, "71321", "G");

    auto node713111 = CreateNodeSimpleFactoryTemplate(graph, "713111", "F");
    auto node713211 = CreateNodeSimpleFactoryTemplate(graph, "71211", "G");
    auto node7131111 = CreateNodeSimpleFactoryTemplate(graph, "7131111", "G");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node1, node4, 0, 0);
    CreateEdges(node1, node5, 0, 0);
    CreateEdges(node1, node6, 0, 0);
    CreateEdges(node1, node7, 0, 0);
    CreateEdges(node1, node8, 0, 0);

    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node5, node51, 0, 0);
    CreateEdges(node6, node61, 0, 0);
    CreateEdges(node7, node71, 0, 0);
    CreateEdges(node7, node72, 0, 0);
    CreateEdges(node7, node73, 0, 0);

    CreateEdges(node71, node711, 0, 0);
    CreateEdges(node71, node712, 0, 0);
    CreateEdges(node71, node713, 0, 0);
    CreateEdges(node71, node714, 0, 0);

    CreateEdges(node713, node7131, 0, 0);
    CreateEdges(node713, node7132, 0, 0);

    CreateEdges(node714, node7141, 0, 0);
    CreateEdges(node714, node7142, 0, 0);

    CreateEdges(node7131, node71311, 0, 0);
    CreateEdges(node7132, node71321, 0, 0);

    CreateEdges(node7131, node71311, 0, 0);
    CreateEdges(node7132, node71321, 0, 0);

    CreateEdges(node71311, node713111, 0, 0);
    CreateEdges(node713111, node7131111, 0, 0);
    CreateEdges(node71321, node713211, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase04Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase05Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase06Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase07Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateCastNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase08Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePReluNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase09Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateLReluNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase10Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRelu6Node(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase11Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateReluNode(graph, "2");
    CreateEdges(node1, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase12Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePReluNode(graph, "2");
    auto node3 = CreateAddNode(graph, "3");
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node3, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase13Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateLReluNode(graph, "2");
    auto node3 = CreateAddNode(graph, "3");
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node3, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase14Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRelu6Node(graph, "2");
    auto node3 = CreateAddNode(graph, "3");
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node3, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase15Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateReluNode(graph, "2");
    auto node3 = CreateAddNode(graph, "3");
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node3, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase16Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    auto node3 = CreateAddNode(graph, "3");
    CreateEdges(node1, node3, 0, 0);
    CreateEdges(node3, node2, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase17Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreatePReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase18Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateLReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase19Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase20Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateRelu6Node(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase21Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    auto node3 = CreatePReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase22Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    auto node3 = CreateLReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase23Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase24Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    auto node3 = CreateRelu6Node(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase25Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    auto node3 = CreatePReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase26Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    auto node3 = CreateLReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase27Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase28Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateQuantNode(graph, "2");
    auto node3 = CreateRelu6Node(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase29Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateCastNode(graph, "2");
    auto node3 = CreatePReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase30Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateCastNode(graph, "2");
    auto node3 = CreateLReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase31Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateCastNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase32Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateCastNode(graph, "2");
    auto node3 = CreateRelu6Node(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase33Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAddNode(graph, "2");
    auto node3 = CreatePReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node3, node4, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase34Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAddNode(graph, "2");
    auto node3 = CreateLReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node3, node4, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase35Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAddNode(graph, "2");
    auto node3 = CreateRelu6Node(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node3, node4, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase36Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateReluNode(graph, "2");
    auto node21 = CreateAdd2Node(graph, "21");
    auto node211 = CreateLReluNode(graph, "211");
    auto node2111 = CreateOtherNode(graph, "2111");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);
    CreateEdges(node211, node2111, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase37Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateReluNode(graph, "2");
    auto node21 = CreateAdd2Node(graph, "21");
    auto node211 = CreateLReluNode(graph, "211");
    auto node2111 = CreateTransFormNode(graph, "2111");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);
    CreateEdges(node211, node2111, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase38Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateReluNode(graph, "2");
    auto node21 = CreateAdd3Node(graph, "21");
    auto node211 = CreateLReluNode(graph, "211");
    auto node2111 = CreateTransFormNode(graph, "2111");
    auto node21111 = CreateCastNode(graph, "21111");

    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    auto node4 = CreateConstNode(graph, "4");
    auto node41 = CreateCastNode(graph, "41");
    auto node411 = CreateAdd4Node(graph, "411");
    auto node4111 = CreateTransFormNode(graph, "4111");
    auto node41111 = CreateCastNode(graph, "41111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);
    CreateEdges(node211, node2111, 0, 0);
    CreateEdges(node2111, node21111, 0, 0);

    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node21, 0, 1);

    CreateEdges(node4, node41, 0, 0);
    CreateEdges(node41, node411, 0, 0);
    CreateEdges(node411, node4111, 0, 0);
    CreateEdges(node4111, node41111, 0, 0);
    CreateEdges(node211, node411, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase39Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAdd3Node(graph, "2");
    auto node21 = CreateAdd3Node(graph, "21");
    auto node211 = CreateTransFormNode(graph, "211");
    auto node2111 = CreateCastNode(graph, "2111");

    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    auto node4 = CreateConstNode(graph, "4");
    auto node41 = CreateCastNode(graph, "41");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);
    CreateEdges(node211, node2111, 0, 0);

    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node2, 0, 1);

    CreateEdges(node4, node41, 0, 0);
    CreateEdges(node41, node21, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase40Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAdd3Node(graph, "2");
    auto node21 = CreateTransFormNode(graph, "21");
    auto node211 = CreateCastNode(graph, "211");

    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    auto node4 = CreateConstNode(graph, "4");
    auto node41 = CreateCastNode(graph, "41");
    auto node411 = CreateAdd4Node(graph, "411");
    auto node4111 = CreateTransFormNode(graph, "4111");
    auto node41111 = CreateCastNode(graph, "41111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);

    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node2, 0, 1);

    CreateEdges(node4, node41, 0, 0);
    CreateEdges(node41, node411, 0, 0);
    CreateEdges(node411, node4111, 0, 0);
    CreateEdges(node4111, node41111, 0, 0);
    CreateEdges(node2, node411, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase41Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateAdd3Node(graph, "2");
    auto node21 = CreateOtherNode(graph, "21");
    auto node211 = CreateCastNode(graph, "211");

    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    auto node4 = CreateConstNode(graph, "4");
    auto node41 = CreateCastNode(graph, "41");
    auto node411 = CreateAdd4Node(graph, "411");
    auto node4111 = CreateTransFormNode(graph, "4111");
    auto node41111 = CreateCastNode(graph, "41111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node21, 0, 0);
    CreateEdges(node21, node211, 0, 0);

    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node2, 0, 1);

    CreateEdges(node4, node41, 0, 0);
    CreateEdges(node41, node411, 0, 0);
    CreateEdges(node411, node4111, 0, 0);
    CreateEdges(node4111, node41111, 0, 0);
    CreateEdges(node2, node411, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase42Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase43Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase44Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateTransFormNode(graph, "3111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase45Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateCastNode(graph, "3111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase46Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase47Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateCastNode(graph, "3111");
    auto node31111 = CreateTransFormNode(graph, "31111");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node31111, 0, 0);
    CreateEdges(node31111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase48Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase49Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase50Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateCastNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateTransFormNode(graph, "3111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase51Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateCastNode(graph, "3111");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase52Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");

    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase53Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreatePRelu2Node(graph, "2");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateCastNode(graph, "3111");
    auto node31111 = CreateTransFormNode(graph, "31111");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node31111, 0, 0);
    CreateEdges(node31111, node2, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase54Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2");
    auto node4 = CreatePRelu2Node(graph, "4");
    auto node3 = CreateConstNode(graph, "3");
    auto node31 = CreateTransFormNode(graph, "31");
    auto node311 = CreateTransFormNode(graph, "311");
    auto node3111 = CreateCastNode(graph, "3111");
    auto node31111 = CreateTransFormNode(graph, "31111");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node311, node3111, 0, 0);
    CreateEdges(node3111, node31111, 0, 0);
    CreateEdges(node31111, node2, 0, 1);
    CreateEdges(node31111, node4, 0, 1);
    return node1;
  }

  static ge::NodePtr CreateTestCase55Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateRequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    auto node4 = CreateReluNode(graph, "4");
    auto node5 = CreateQuantNode(graph, "5");
    auto node6 = CreateTransFormNode(graph, "6");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node3, node4, 0, 0);
    CreateEdges(node4, node5, 0, 0);
    CreateEdges(node5, node6, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase56Graph(ge::ComputeGraphPtr graph) {
    auto node_cube1 = CreateCubeNode(graph, "cube1");
    auto node_const1 = CreateConstNode(graph, "const1");
    auto node_transdata11 = CreateTransFormNode(graph, "transdata11");
    auto node_dequant11 = CreateDequant2Node(graph, "dequant11");
    auto node_leakeyrelu111 = CreateLReluNode(graph, "leakyrelu111");
    auto node_quant1111 = CreateQuantNode(graph, "quant1111");
    auto node_cube2 = CreateCubeNode(graph, "cube2");
    auto node_const2 = CreateConstNode(graph, "const2");
    auto node_transdata21 = CreateTransFormNode(graph, "transdata21");
    auto node_const3 = CreateConstNode(graph, "const3");
    auto node_transdata31 = CreateTransFormNode(graph, "transdata31");
    auto node_dequant21 = CreateDequant2Node(graph, "dequant21");
    auto node_relu211 = CreateReluNode(graph, "relu211");
    auto node_transdata2111 = CreateTransFormNode(graph, "transdata2111");
    auto node_cast21111 = CreateCastNode(graph, "cast21111");
    CreateEdges(node_cube1, node_dequant11, 0, 0);
    CreateEdges(node_const1, node_transdata11, 0, 0);
    CreateEdges(node_transdata11, node_dequant11, 0, 1);
    CreateEdges(node_dequant11, node_leakeyrelu111, 0, 0);
    CreateEdges(node_leakeyrelu111, node_quant1111, 0, 0);
    CreateEdges(node_quant1111, node_cube2, 0, 0);
    CreateEdges(node_cube2, node_dequant21, 0, 0);
    CreateEdges(node_const3, node_transdata31, 0, 0);
    CreateEdges(node_transdata31, node_dequant21, 0, 1);
    CreateEdges(node_dequant21, node_relu211, 0, 0);    
    CreateEdges(node_relu211, node_transdata2111, 0, 0);
    CreateEdges(node_transdata2111, node_cast21111, 0, 0);
    return node_cube1;
  }

  static ge::NodePtr CreateTestCase57Graph(ge::ComputeGraphPtr graph) {
    auto node_cube1 = CreateCubeNode(graph, "cube1");
    auto node_const1 = CreateConstNode(graph, "const1");
    auto node_transdata11 = CreateTransFormNode(graph, "transdata11");
    auto node_dequant11 = CreateDequant2Node(graph, "dequant11");
    auto node_leakeyrelu111 = CreateLReluNode(graph, "leakyrelu111");
    auto node_quant1111 = CreateQuantNode(graph, "quant1111");
    auto node_cube2 = CreateCubeNode(graph, "cube2");
    CreateEdges(node_cube1, node_dequant11, 0, 0);
    CreateEdges(node_const1, node_transdata11, 0, 0);
    CreateEdges(node_transdata11, node_dequant11, 0, 1);
    CreateEdges(node_dequant11, node_leakeyrelu111, 0, 0);
    CreateEdges(node_leakeyrelu111, node_quant1111, 0, 0);
    CreateEdges(node_quant1111, node_cube2, 0, 0);
    return node_cube1;
  }

  static ge::NodePtr CreateTestCase58Graph(ge::ComputeGraphPtr graph) {
    auto node_cube1 = CreateCubeNode(graph, "cube1");
    auto node_const1 = CreateConstNode(graph, "const1");
    auto node_transdata11 = CreateTransFormNode(graph, "transdata11");
    auto node_dequant11 = CreateDequant2Node(graph, "dequant11");
    auto node_leakeyrelu111 = CreateLReluNode(graph, "leakyrelu111");
    auto node_quant1111 = CreateQuantNode(graph, "quant1111");
    auto node_cube2 = CreateCubeNode(graph, "cube2");
    auto node_quant1112 = CreateQuantNode(graph, "quant1112");
    CreateEdges(node_cube1, node_dequant11, 0, 0);
    CreateEdges(node_const1, node_transdata11, 0, 0);
    CreateEdges(node_transdata11, node_dequant11, 0, 1);
    CreateEdges(node_dequant11, node_leakeyrelu111, 0, 0);
    CreateEdges(node_leakeyrelu111, node_quant1111, 0, 0);
    CreateEdges(node_quant1111, node_cube2, 0, 0);
    CreateEdges(node_leakeyrelu111, node_quant1112, 0, 0);
    return node_cube1;
  }

  static ge::NodePtr CreateTestCase59Graph(ge::ComputeGraphPtr graph) {
    auto node_cube1 = CreateCubeNode(graph, "cube1");
    auto node_const1 = CreateConstNode(graph, "const1");
    auto node_transdata11 = CreateTransFormNode(graph, "transdata11");
    auto node_dequant11 = CreateDequant2Node(graph, "dequant11");
    auto node_leakeyrelu111 = CreateLReluNode(graph, "leakyrelu111");
    auto node_quant1111 = CreateQuantNode(graph, "quant1111");
    auto node_cube2 = CreateCubeNode(graph, "cube2");
    auto node_quant21 = CreateQuantNode(graph, "node_quant21");
    auto node_relu211 = CreateReluNode(graph, "node_relu211");
    auto node_add2111 = CreateAddNode(graph, "add2111");
    auto node_add2112 = CreateAddNode(graph, "add2112");
    CreateEdges(node_cube1, node_dequant11, 0, 0);
    CreateEdges(node_const1, node_transdata11, 0, 0);
    CreateEdges(node_transdata11, node_dequant11, 0, 1);
    CreateEdges(node_dequant11, node_leakeyrelu111, 0, 0);
    CreateEdges(node_leakeyrelu111, node_quant1111, 0, 0);
    CreateEdges(node_quant1111, node_cube2, 0, 0);
    CreateEdges(node_cube2, node_quant21, 0, 0);
    CreateEdges(node_quant21,node_relu211, 0, 0);
    CreateEdges(node_relu211,node_add2111, 0, 0);
    CreateEdges(node_relu211,node_add2112, 0, 0);
    return node_cube2;
  }

  static ge::NodePtr CreateTestCase60Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2", true, 1.1, 2.0);
    auto node4 = CreateLReluNode(graph, "4");
    auto node3 = CreateConstNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node2, 0, 1);
    CreateEdges(node2, node4, 0, 0);
    return node1;
  }

  static ge::NodePtr CreateTestCase61Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequant2Node(graph, "2", true, 1.1, 2.0);
    auto node3 = CreateConstNode(graph, "3");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node3, node2, 0, 1);
    return node1;
  }
};

TEST_F(fixpipe_pass_ut, ReadConfig01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  for (auto unit : m_testpass.m_idxtonodetypes_) {
    std::cout << "fixpipe_test unit = : " << unit.GetName() << std::endl;
    for (auto nodes : unit.GetNode()) {
      std::cout << "  opnodes = " << nodes.first;
    }
    for (auto index : unit.GetDependsUnitsIndex()) {
      std::cout << " index = " << index << " is unit = " << m_testpass.m_idxtonodetypes_[index].GetName() << std::endl;
    }
  }
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, IsCubeOpType01) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto conv_node = CreateCubeNode(graph, "conv2d");
  bool ret = m_testpass.IsCubeOpType(conv_node);
  EXPECT_EQ(true, ret);
  auto relu_node = CreateRelu6Node(graph, "relu");
  ret = m_testpass.IsCubeOpType(relu_node);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, JudgeCachePass01) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  bool ret = m_testpass.ReadConfig("Ascend320");
  auto conv_node = CreateTestCase01Graph(graph);
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  stack<uint32_t> index;
  uint32_t ret_index;
  ret = m_testpass.JudgeCachePass(cur_head_info, index, ret_index);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, JudgeCachePass02) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);  
  bool ret = m_testpass.ReadConfig("Ascend320");
  auto conv_node = CreateOtherNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  stack<uint32_t> index;
  index.push(0);
  uint32_t ret_index;
  ret = m_testpass.JudgeCachePass(cur_head_info, index, ret_index);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, PreCachePass01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateOtherNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  FixPipePassInfo cur_pass;
  ret = m_testpass.PreCachePass(cur_pass, node_info);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, PreCachePass02) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  cur_pass.unit_index = 0;
  cur_pass.m_opnodes.push_back(head_node);
  ret = m_testpass.PreCachePass(cur_pass, node_info);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, PreMatchAcorrdingToPass01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  cur_pass.unit_index = 1;
  cur_pass.m_opnodes.push_back(head_node);
  ret = m_testpass.PreMatchAcorrdingToPass(cur_pass, node_info);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, PreMatchAcorrdingToPass02) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  cur_pass.unit_index = 0;
  cur_pass.m_opnodes.push_back(head_node);
  ret = m_testpass.PreMatchAcorrdingToPass(cur_pass, node_info);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, NeedToCutPass01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  cur_pass.unit_index = 1;
  cur_pass.m_opnodes.push_back(head_node);
  ret = m_testpass.NeedToCutPass(cur_pass);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, NeedToCutPass02) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  auto othernode = CreateOtherNode(graph, "othernode");
  CreateEdges(cnode, node, 0, 0);
  CreateEdges(node, othernode, 0, 0);

  cur_pass.unit_index = 1;
  cur_pass.m_opnodes.push_back(head_node);
  cur_pass.m_opnodes.push_back(node_info);
  ret = m_testpass.NeedToCutPass(cur_pass);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, IsInWhitelist01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  auto othernode = CreateOtherNode(graph, "othernode");
  CreateEdges(cnode, node, 0, 0);
  CreateEdges(node, othernode, 0, 0);

  cur_pass.unit_index = 1;
  ret = m_testpass.IsInWhitelist(node_info);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, IsInWhitelist02) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  auto othernode = CreateOtherNode(graph, "othernode");
  CreateEdges(cnode, node, 0, 0);
  CreateEdges(node, othernode, 0, 0);
  FixPipeNodeInfo otnode_info(othernode);
  cur_pass.unit_index = 1;
  ret = m_testpass.IsInWhitelist(otnode_info);
  EXPECT_EQ(false, ret);
}

TEST_F(fixpipe_pass_ut, JudgeIsMatch01) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  auto othernode = CreateOtherNode(graph, "othernode");
  CreateEdges(cnode, node, 0, 0);
  CreateEdges(node, othernode, 0, 0);
  stack<uint32_t> index;
  index.push(0);
  uint32_t ret_index;
  cur_pass.unit_index = 1;
  Status ret1 = m_testpass.JudgeIsMatch(node_info, index, ret_index);
  EXPECT_EQ(SUCCESS, ret1);
}

TEST_F(fixpipe_pass_ut, JudgeIsMatch02) {
  FixPipePass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto node = CreateQuantNode(graph, "testnode");
  DumpTestGraph(graph);
  FixPipeNodeInfo node_info(node);
  auto cnode = CreateCubeNode(graph, "cube");
  FixPipeNodeInfo head_node(cnode);
  FixPipePassInfo cur_pass;
  auto othernode = CreateOtherNode(graph, "othernode");
  FixPipeNodeInfo othernode_info(othernode);
  CreateEdges(cnode, node, 0, 0);
  CreateEdges(node, othernode, 0, 0);
  stack<uint32_t> index;
  index.push(0);
  uint32_t ret_index;
  cur_pass.unit_index = 1;
  Status ret1 = m_testpass.JudgeIsMatch(othernode_info, index, ret_index);
  EXPECT_EQ(FAILED, ret1);
}

TEST_F(fixpipe_pass_ut, AddInput04) {
  FixPipePass m_testpass;
 
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput04" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase04Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput04" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput04" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput04" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput05) {
  FixPipePass m_testpass;
 
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput05" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase05Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput05" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput05" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput05" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput06) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput06" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase06Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput06" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput06" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput06" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput07) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput07" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase07Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput07" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput07" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput07" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput08) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput08" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase08Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput08" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput08" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput08" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput09) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput09" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase09Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput09" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput09" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput09" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput10) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput10" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase10Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput10" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput10" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput10" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput11) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput11" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase11Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput11" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput11" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput11" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput12) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput12" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase12Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput12" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput12" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput12" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput13) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput13" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase13Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput13" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput13" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput13" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput14) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput14" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase14Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput14" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput14" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput14" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput15) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput15" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase15Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput15" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput15" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput15" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput16) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput16" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase16Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput16" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput16" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput16" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput17) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput17" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase17Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput17" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput17" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput17" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput18) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput18" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase18Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput18" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput18" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput18" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput19) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput19" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase19Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput19" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput19" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput19" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput20) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput20" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase20Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput20" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput20" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput20" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput21) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput21" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase21Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput21" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput21" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput21" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput22) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput22" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase22Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput22" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput22" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput22" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput23) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput23" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase23Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput23" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput23" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput23" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput24) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput24" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase24Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput24" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput24" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput24" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput25) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput25" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase25Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput25" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput25" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput25" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput26) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput26" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase26Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput26" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput26" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput26" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput27) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput27" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase27Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput27" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput27" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput27" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput28) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput28" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase28Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput28" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput28" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput28" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput29) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput29" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase29Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput29" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput29" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput29" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput30) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput30" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase30Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput30" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput30" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput30" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput31) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput31" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase31Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput31" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput31" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput31" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput32) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput32" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase32Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput32" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput32" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput32" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput33) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput33" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase33Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput33" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput33" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput33" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput34) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput34" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase34Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput34" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput34" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput34" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput35) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput35" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase35Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput35" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput35" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput35" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput42) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput42" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase42Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput42" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput42" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput42" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput43) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput43" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase43Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput43" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput43" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput43" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput44) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput44" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase44Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput44" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput44" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput44" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput45) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput45" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase45Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput45" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput45" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput45" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput46) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput46" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase46Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput46" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput46" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput46" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput47) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput47" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase47Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput47" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput47" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput47" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput48) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput48" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase48Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput48" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput48" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput48" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput49) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput49" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase49Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput49" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput49" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput49" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput50) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput50" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase50Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput50" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput50" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput50" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput51) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput51" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase51Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput51" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput51" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput51" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput52) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput52" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase52Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput52" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput52" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput52" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput53) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput53" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase53Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput53" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput53" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput53" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, AddInput54) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : AddInput54" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase54Graph(graph);
  std::cout << "fixpipe_test : beforefusion AddInput54" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison AddInput54" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end AddInput54" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass01) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass01" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase01Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass01" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass01" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass01" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass04) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass04" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase04Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass04" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass04" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass04" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass05) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass05" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase05Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass05" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass05" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass05" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass36) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass36" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase36Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass36" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass36" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass36" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass37) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass37" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase37Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass37" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass37" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass37" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass38) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass38" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase38Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass38" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass38" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass38" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass39) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass39" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase39Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass39" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass39" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass39" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass40) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass40" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase40Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass40" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass40" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass40" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass41) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass41" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase41Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass41" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass41" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass41" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass55) {
  FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass55" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase55Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass55" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass55" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass55" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass56) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass56" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  FixPipePreQuantPass m_pretestpass;
  m_pretestpass.ReadConfig("Ascend320");
  ge::NodePtr conv_node = CreateTestCase56Graph(graph);
  std::cout << "fixpipe_test  before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_pretestpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after prepass GenraterPass56" << std::endl;
  DumpTestGraph(graph);
  passes.clear();
  passes.push_back(&m_testpass);
  status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after fixpipepass GenraterPass56" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass57) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass57" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  FixPipePreQuantPass m_pretestpass;
  m_pretestpass.ReadConfig("Ascend320");
  ge::NodePtr conv_node = CreateTestCase57Graph(graph);
  std::cout << "fixpipe_test  before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_pretestpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after prepass GenraterPass57" << std::endl;
  DumpTestGraph(graph);
  passes.clear();
  passes.push_back(&m_testpass);
  status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after fixpipepass GenraterPass57" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass57_1) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass57_1" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  FixPipePreQuantPass m_pretestpass;
  m_pretestpass.ReadConfig("Ascend320");
  ge::NodePtr conv_node = CreateTestCase57Graph(graph);
  std::cout << "fixpipe_test  before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_pretestpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after prepass GenraterPass57_1" << std::endl;
  DumpTestGraph(graph);
  passes.clear();
  passes.push_back(&m_testpass);
  status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after fixpipepass GenraterPass57_1" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass58) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass58" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  FixPipePreQuantPass m_pretestpass;
  m_pretestpass.ReadConfig("Ascend320");
  ge::NodePtr conv_node = CreateTestCase58Graph(graph);
  std::cout << "fixpipe_test  before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_pretestpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after prepass GenraterPass58" << std::endl;
  DumpTestGraph(graph);
  passes.clear();
  passes.push_back(&m_testpass);
  status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after fixpipepass GenraterPass58" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass59) {
  FixPipePass m_testpass;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass59" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  FixPipePreQuantPass m_pretestpass;
  m_pretestpass.ReadConfig("Ascend320");
  ge::NodePtr conv_node = CreateTestCase59Graph(graph);
  std::cout << "fixpipe_test  before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_pretestpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after prepass GenraterPass59" << std::endl;
  DumpTestGraph(graph);
  passes.clear();
  passes.push_back(&m_testpass);
  status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test  after fixpipepass GenraterPass59" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass59_1) {
 FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass59_1" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase59Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass59_1" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass59_1" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass59_1" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass60) {
 FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass60" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase60Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass60" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass60" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass60" << std::endl;
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pass_ut, GenraterPass61) {
 FixPipePass m_testpass;
  
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  std::cout << "fixpipe_test : GenraterPass61" << std::endl;
  bool ret = m_testpass.ReadConfig("Ascend320");
  m_testpass.Init();
  ge::NodePtr conv_node = CreateTestCase61Graph(graph);
  std::cout << "fixpipe_test : beforefusion GenraterPass61" << std::endl;
  DumpTestGraph(graph);
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.isheadnode_ = true;
  std::vector<ge::NodePtr> new_nodes;
  m_testpass.GenerateMatchedPasses(cur_head_info,  new_nodes);
  m_testpass.ModfiyMatchedPasses();
  for (auto pass : m_testpass.m_matchpasses_) {
    PrintPass(pass);
  }
  for (auto pass : m_testpass.m_matchpasses_) {
    ge::NodePtr fixpipenode = m_testpass.CreateFixpipeNode(pass, cur_head_info, *graph);
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    m_testpass.AddInputs(*graph, pass, fixpipenode, new_nodes);
    m_testpass.RelinkOpEdges(cur_head_info, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto pass : m_testpass.m_matchpasses_) {
    m_testpass.DeleteToFusedNodeEdge(*graph, pass, todeletenode);
  }
  m_testpass.DeleteNode(todeletenode);
  std::cout << "fixpipe_test : afterfuison GenraterPass61" << std::endl;
  DumpTestGraph(graph);
  std::cout << "fixpipe_test : end GenraterPass61" << std::endl;
  EXPECT_EQ(true, ret);
}
}  // namespace fe