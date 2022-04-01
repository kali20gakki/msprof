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
#include "common/math_util.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_common.h"
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

class fixpipe_pre_quant_pass_st : public testing::Test {
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
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT);
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
    const_opdesc->AddOutputDesc(const_tensor_desc);
    const_opdesc->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_opdesc);
    return const_node;
  }

  static ge::NodePtr CreateDequantNode(ge::ComputeGraphPtr graph, std::string opname, bool hasattr = false,
                                       float scale = 0.0, float offset = 0.0) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
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
    dequant_input->SetData(reinterpret_cast<uint8_t *>(inputv01.data()), 720);
    ge::OpDescPtr const_opdesc = std::make_shared<ge::OpDesc>(opname + "_constinput", "Const");
    ge::AttrUtils::SetTensor(const_opdesc, ge::ATTR_NAME_WEIGHTS, dequant_input);
    const_opdesc->AddOutputDesc(const_tensor_desc);
    const_opdesc->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_opdesc);

    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendDequant");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);

    if (hasattr) {
      ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
      ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    }
    auto node = graph->AddNode(opdesc);
    CreateEdges(const_node, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateQuantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendQuant");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    float offset = 8.0;
    float scale = 4.0;
    ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
    ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateRequantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendRequant");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_inputconst");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateCastNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Cast");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateAntiQuantNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "AscendAntiQuant");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
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
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto inputnode = CreateAntiQuantNode(graph, opname + "_inputantinode");
    CreateEdges(inputnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateAdd2Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto inputnode = CreateConstNode(graph, opname + "_inputconstnode");
    CreateEdges(inputnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateAdd3Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    const_tensor_desc.SetOriginFormat(ge::FORMAT_FRACTAL_NZ);
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateAdd4Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Add");
    const_tensor_desc.SetOriginFormat(ge::FORMAT_ND);
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateSubNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Sub");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto inputnode = CreateAntiQuantNode(graph, opname + "_inputantinode");
    CreateEdges(inputnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Relu");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreatePReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "PRelu");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_inputconstnode");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreateLReluNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "LeakyRelu");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    float negative_slope = 8.0f;
    ge::AttrUtils::SetFloat(opdesc, "negative_slope", negative_slope);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateRelu6Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Relu6");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateTransFormNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_FRACTAL_NZ, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "TransData");
    opdesc->AddInputDesc(const_tensor_desc);
    const_tensor_desc.SetFormat(ge::FORMAT_ND);
    opdesc->AddOutputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateCubeNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Conv2D");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateOtherNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "MaxPool");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;
  }

  static ge::NodePtr CreateNodeSimpleFactorTemplate(ge::ComputeGraphPtr graph, std::string opname,
                                                    std::string opunittype) {
    if (opunittype == "A") {
      return CreateCubeNode(graph, opname);
    } else if (opunittype == "B") {
      return CreateDequantNode(graph, opname);
    } else if (opunittype == "C") {
      return CreateReluNode(graph, opname);
    } else if (opunittype == "D") {
      return CreateAddNode(graph, opname);
    } else if (opunittype == "E") {
      return CreateQuantNode(graph, opname);
    } else if (opunittype == "F") {
      return CreateTransFormNode(graph, opname);
    } else {
      return CreateOtherNode(graph, opname);
    }
  }

  static void CreateTestCase01Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateNodeSimpleFactorTemplate(graph, "1", "A");
    auto node2 = CreateNodeSimpleFactorTemplate(graph, "2", "E");
    CreateEdges(node1, node2, 0, 0);
  }

  static void CreateTestCase02Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateQuantNode(graph, "3");
    auto node4 = CreateCastNode(graph, "4");
    auto node31 = CreateReluNode(graph, "31");
    auto node41 = CreateReluNode(graph, "41");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node4, node41, 0, 0);
  }

  static void CreateTestCase03Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node31 = CreateCastNode(graph, "31");
    auto node41 = CreateReluNode(graph, "41");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node4, node41, 0, 0);
  }

  static void CreateTestCase04Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node31 = CreateQuantNode(graph, "31");
    auto node41 = CreateReluNode(graph, "41");
    auto node311 = CreateQuantNode(graph, "311");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node31, node311, 0, 0);
    CreateEdges(node4, node41, 0, 0);
  }

  static void CreateTestCase05Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateQuantNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node5 = CreateQuantNode(graph, "5");
    auto node31 = CreateReluNode(graph, "31");
    auto node41 = CreateReluNode(graph, "41");
    auto node51 = CreateReluNode(graph, "51");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node2, node5, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node5, node51, 0, 0);
    CreateEdges(node4, node41, 0, 0);
  }

  static void CreateTestCase06Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto noded = CreateDequantNode(graph, "2d");
    auto node2 = CreateReluNode(graph, "2");
    auto node3 = CreateQuantNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node5 = CreateQuantNode(graph, "5");
    auto node31 = CreateReluNode(graph, "31");
    auto node41 = CreateReluNode(graph, "41");
    auto node51 = CreateReluNode(graph, "51");
    CreateEdges(node1, noded, 0, 0);
    CreateEdges(noded, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node2, node5, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node5, node51, 0, 0);
    CreateEdges(node4, node41, 0, 0);
  }

  static void CreateTestCase07Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node5 = CreateCastNode(graph, "5");
    auto node31 = CreateQuantNode(graph, "31");
    auto node32 = CreateCastNode(graph, "32");
    auto node311 = CreateReluNode(graph, "311");
    auto node41 = CreateReluNode(graph, "41");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node2, node5, 0, 0);
    CreateEdges(node3, node31, 0, 0);
    CreateEdges(node3, node32, 0, 0);
    CreateEdges(node4, node41, 0, 0);
    CreateEdges(node31, node311, 0, 0);
  }

  static void CreateTestCase08Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node3 = CreateReluNode(graph, "3");
    auto node4 = CreateQuantNode(graph, "4");
    auto node5 = CreateCastNode(graph, "5");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node3, 0, 0);
    CreateEdges(node3, node4, 0, 0);
    CreateEdges(node4, node5, 0, 0);
  }

  static void CreateTestCase09Graph(ge::ComputeGraphPtr graph) {
    auto node1 = CreateCubeNode(graph, "1");
    auto node2 = CreateDequantNode(graph, "2");
    auto node4 = CreateQuantNode(graph, "4");
    auto node5 = CreateCastNode(graph, "5");
    CreateEdges(node1, node2, 0, 0);
    CreateEdges(node2, node4, 0, 0);
    CreateEdges(node4, node5, 0, 0);
  }
};

TEST_F(fixpipe_pre_quant_pass_st, ReadConfig01) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  for (auto unit : m_testpass.m_units_) {
    std::cout << "fixpipe_test unit = : " << unit << std::endl;
  }
  EXPECT_EQ(true, ret);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph01) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase01Graph(graph);
  std::cout << "fixpipe_test Graph01: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph01: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(status, NOT_CHANGED);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph02) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase02Graph(graph);
  std::cout << "fixpipe_test Graph02: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph02: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph03) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase03Graph(graph);
  std::cout << "fixpipe_test Graph03: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph03: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph04) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase04Graph(graph);
  std::cout << "fixpipe_test Graph04: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph04: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph05) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase05Graph(graph);
  std::cout << "fixpipe_test Graph05: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph05: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph06) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase06Graph(graph);
  std::cout << "fixpipe_test Graph06: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph06: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph07) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase07Graph(graph);
  std::cout << "fixpipe_test Graph07: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph07: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_pre_quant_pass_st, Graph08) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase08Graph(graph);
  std::cout << "fixpipe_test Graph08: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph08: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(status, SUCCESS);
}


TEST_F(fixpipe_pre_quant_pass_st, Graph09) {
  FixPipePreQuantPass m_testpass;
  bool ret = m_testpass.ReadConfig("Ascend320");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  CreateTestCase09Graph(graph);
  std::cout << "fixpipe_test Graph08: before fusion" << std::endl;
  DumpTestGraph(graph);
  vector<GraphPass *> passes;
  passes.push_back(&m_testpass);
  Status status = PassManager::Run(*graph, passes);
  std::cout << "fixpipe_test Graph08: after fusion" << std::endl;
  DumpTestGraph(graph);
  EXPECT_EQ(status, SUCCESS);
}

}  // namespace fe