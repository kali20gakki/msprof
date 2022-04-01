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
#include <map>
#include <memory>
#include <string>
#include <vector>

#define protected public
#define prviate public
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/fe_fp16_t.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/range_vistor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_addinputstrategy.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_common.h"
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

class fixpipe_addinputstrategy_ut : public testing::Test {
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

  static ge::NodePtr CreateFixpipeNode(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "FixPipe");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    return node;

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
    auto node = graph->AddNode(opdesc);
    if (hasattr) {
      ge::AttrUtils::SetFloat(opdesc, ATTR_OFFSET, offset);
      ge::AttrUtils::SetFloat(opdesc, ATTR_SCALE, scale);
    }
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
    auto constnode = CreateConstNode(graph, opname + "_constinput");
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

  static ge::NodePtr CreatePRelu2Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "PRelu");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    auto constnode = CreateConstNode(graph, opname + "_inputconstnode");
    CreateEdges(constnode, node, 0, 1);
    return node;
  }

  static ge::NodePtr CreatePRelu3Node(ge::ComputeGraphPtr graph, std::string opname) {
    ge::GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT);
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "PRelu");
    opdesc->AddOutputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    opdesc->AddInputDesc(const_tensor_desc);
    auto node = graph->AddNode(opdesc);
    //auto constnode = CreateConstNode(graph, opname + "_inputconstnode");
    //CreateEdges(constnode, node, 0, 1);
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
    ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(opname, "Conv2d");
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
};

TEST_F(fixpipe_addinputstrategy_ut, TransM1Scale01) {
  FixPipeAddInputBase m_testpass{};
  float src_value = -128;
  uint64_t tmpdata =  m_testpass.TransM1Scale(src_value);
  bool ret = false;
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_addinputstrategy_ut, SetM1OfQuant01) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  float offset = -128;
  float scale = 66.45;
  uint64_t tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  bool ret = false;
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, false);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_UINT16);
  ret = false;
  tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_UINT8);
  ret = false;
  tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT4);
  ret = false;
  tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT16);
  ret = false;
  tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT8);
  ret = false;
  tmpdata =  m_testpass.SetM1OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_addinputstrategy_ut, SetM3OfQuant01) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  float offset = -128;
  float scale = 66.45;
  uint64_t tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  bool ret = false;
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, false);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_UINT16);
  ret = false;
  tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_UINT8);
  ret = false;
  tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT4);
  ret = false;
  tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT16);
  ret = false;
  tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT8);
  ret = false;
  tmpdata =  m_testpass.SetM3OfQuant(scale, offset, dequant_node);
  if (tmpdata != 0) {
    ret = true;
  }
  std::cout << "fixpipe_test tmpdata = " << tmpdata << std::endl;
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_addinputstrategy_ut, IsScalar01) {
  FixPipeAddInputBase m_testpass{};
  ge::GeShape shape{};
  bool ret = m_testpass.IsScalar(shape);
  EXPECT_EQ(ret, true);
  ge::GeShape original_shape = GeShape({128, 12, 5, 6, 16});
  ret = m_testpass.IsScalar(original_shape);
  EXPECT_EQ(ret, false);
}

TEST_F(fixpipe_addinputstrategy_ut, CreateAndUpdateVectorMulsInput01) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.CreateAndUpdateVectorMulsInput(*graph, funtcparam, dequant_node_info, prelu_node_info, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, CloneVectorInput01) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.CloneVectorInput(prelu_node_info, funtcparam);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, CloneVectorInput02) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePRelu2Node(graph, "prelu");
  prelu_node->GetOpDesc()->MutableInputDesc(1) = nullptr;
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret  = m_testpass.CloneVectorInput(prelu_node_info, funtcparam);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, CloneVectorInput03) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePRelu3Node(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.CloneVectorInput(prelu_node_info, funtcparam);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(fixpipe_addinputstrategy_ut, CloneVectorInput04) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePRelu2Node(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.CloneVectorInput(prelu_node_info, funtcparam);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, CloneVectorInput05) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePRelu2Node(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  fixpipenode->GetInDataAnchor(3) = nullptr;
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.CloneVectorInput(prelu_node_info, funtcparam);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, SetClipValue601) {
  FixPipeAddInputBase m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  std::vector<ge::NodePtr> new_nodes;
  bool ret = true;
  m_testpass.SetClipValue6(*graph, funtcparam, ge::DT_FLOAT, new_nodes);
  m_testpass.SetClipValue6(*graph, funtcparam, ge::DT_FLOAT16, new_nodes);
  m_testpass.SetClipValue6(*graph, funtcparam, ge::DT_INT8, new_nodes);
  m_testpass.SetClipValue6(*graph, funtcparam, ge::DT_INT4, new_nodes);
  m_testpass.SetClipValue6(*graph, funtcparam, ge::DT_INT32, new_nodes);
  EXPECT_EQ(ret, true);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput31) {
  FixPipeAddInputStrategy31 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(quant_node_info);
  match_pass.m_opnodes.push_back(prelu_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(1);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput32) {
  FixPipeAddInputStrategy32 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(dequant_node_info);
  match_pass.m_opnodes.push_back(prelu_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(1);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput33) {
  FixPipeAddInputStrategy33 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(prelu_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(0);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput61) {
  FixPipeAddInputStrategy61 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(prelu_node_info);
  match_pass.m_opnodes.push_back(quant_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(1);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput43) {
  FixPipeAddInputStrategy43 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant", true, (float)66.55, (float)128.0);
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  auto relu6_node = CreateRelu6Node(graph, "relu6");
  
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipeNodeInfo relu6_node_info(relu6_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(dequant_node_info);
  match_pass.m_opnodes.push_back(relu6_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(1);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_FLOAT);
  ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_FLOAT16);
  ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT8);
  ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT4);
  ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT32);
  ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput62) {
  FixPipeAddInputStrategy62 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto dequant_node = CreateDequantNode(graph, "dequant");
  auto prelu_node = CreatePReluNode(graph, "prelu");
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo dequant_node_info(dequant_node);
  FixPipeNodeInfo prelu_node_info(prelu_node);
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(prelu_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_FLOAT);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(0);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(fixpipe_addinputstrategy_ut, DoAddInput91) {
  FixPipeAddInputStrategy91 m_testpass{};
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>(GRAPH_NAME);
  auto quant_node = CreateQuantNode(graph, "quant");
  FixPipeNodeInfo quant_node_info(quant_node);
  FixPipePassInfo match_pass;
  match_pass.pass_index = 2;
  match_pass.m_flag = 1;
  match_pass.unit_index = 2;
  match_pass.m_opnodes.push_back(quant_node_info);
  ge::NodePtr fixpipenode = CreateFixpipeNode(graph, "fixpipenode");
  FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
  FixPipeFunctionParamPtr funtcparam;
  uint32_t index = 3;
  funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, index);
  funtcparam->SetDataType(ge::DT_INT8);
  funtcparam->SetFirstIndex(0);
  funtcparam->SetSecondIndex(0);
  std::vector<ge::NodePtr> new_nodes;
  Status ret = m_testpass.DoAddInput(*graph, match_pass, funtcparam, new_nodes);
  EXPECT_EQ(ret, SUCCESS);
}

}  // namespace fe