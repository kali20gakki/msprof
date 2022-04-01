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

#define protected public
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/concat_quant_fusion_pass.h"
#include <gtest/gtest.h>

#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

#undef protected

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {

class STEST_concat_quant_fusion_pass : public testing::Test {
public:
  const string GRAPH_NAME = "test";
  const string CONCATD = "ConcatD";
  const string DEQUANT = "AscendDequant";
  const string QUANT = "AscendQuant";
  const string CONV2D = "Conv2D";

protected:
  void SetUp() {}

  void TearDown() {}

  void InitGraph(ComputeGraphPtr &graph, ge::DataType quant_data_type) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr maxpool = std::make_shared<OpDesc>("maxpool", "MaxPool");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr quant1 = std::make_shared<OpDesc>("quant1", QUANT);
    OpDescPtr quant2 = std::make_shared<OpDesc>("quant2", QUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");

    // add descriptor
    ge::GeShape shape0({1, 64, 7, 7});
    GeTensorDesc out_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat->AddInputDesc(out_desc0);
    maxpool->AddInputDesc(out_desc0);
    maxpool->AddOutputDesc(out_desc0);
    quant1->AddInputDesc(out_desc0);
    GeTensorDesc quant1_outdesc(shape0, ge::FORMAT_NCHW, quant_data_type);
    quant1->AddOutputDesc(quant1_outdesc);
    conv1->AddInputDesc(quant1_outdesc);
    GeTensorDesc conv1_outdesc(shape0, ge::FORMAT_NCHW, ge::DT_INT32);
    conv1->AddOutputDesc(conv1_outdesc);
    dequant1->AddInputDesc(conv1_outdesc);

    GeTensorDesc out_desc3(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    dequant1->AddOutputDesc(conv1_outdesc);
    concat->AddInputDesc(conv1_outdesc);

    ge::GeShape shape1({1, 128, 7, 7});
    GeTensorDesc out2_desc0(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat->AddOutputDesc(out2_desc0);
    quant2->AddInputDesc(out2_desc0);
    GeTensorDesc quant2_desc0(shape1, ge::FORMAT_NCHW, quant_data_type);
    quant2->AddOutputDesc(quant2_desc0);
    end->AddInputDesc(quant2_desc0);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr maxpool_node = graph->AddNode(maxpool);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    NodePtr quant1_node = graph->AddNode(quant1);
    NodePtr quant2_node = graph->AddNode(quant2);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr end_node = graph->AddNode(end);
    ge::GraphUtils::AddEdge(maxpool_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant1_node->GetOutDataAnchor(0), conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0), dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), quant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant2_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  void InitSameInputGraph(ComputeGraphPtr &graph, ge::DataType quant_data_type) {
    OpDescPtr concat0 = std::make_shared<OpDesc>("concat0", CONCATD);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr maxpool = std::make_shared<OpDesc>("maxpool", "MaxPool");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr quant1 = std::make_shared<OpDesc>("quant1", QUANT);
    OpDescPtr quant2 = std::make_shared<OpDesc>("quant2", QUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");

    ge::AttrUtils::SetFloat(quant1, "scale", 0.1);
    ge::AttrUtils::SetFloat(quant1, "offset", -128);
    ge::AttrUtils::SetFloat(quant2, "scale", 0.1);
    ge::AttrUtils::SetFloat(quant2, "offset", -128);
    // add descriptor
    ge::GeShape shape0({1, 64, 7, 7});
    GeTensorDesc out_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat0->AddOutputDesc(out_desc0);
    concat->AddInputDesc(out_desc0);
    concat->AddInputDesc(out_desc0);
    maxpool->AddInputDesc(out_desc0);
    maxpool->AddOutputDesc(out_desc0);
    quant1->AddInputDesc(out_desc0);
    GeTensorDesc quant1_outdesc(shape0, ge::FORMAT_NCHW, quant_data_type);
    quant1->AddOutputDesc(quant1_outdesc);
    conv1->AddInputDesc(quant1_outdesc);
    GeTensorDesc conv1_outdesc(shape0, ge::FORMAT_NCHW, ge::DT_INT32);
    conv1->AddOutputDesc(conv1_outdesc);
    dequant1->AddInputDesc(conv1_outdesc);

    GeTensorDesc out_desc3(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    dequant1->AddOutputDesc(conv1_outdesc);
    concat->AddInputDesc(conv1_outdesc);

    ge::GeShape shape1({1, 128, 7, 7});
    GeTensorDesc out2_desc0(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat->AddOutputDesc(out2_desc0);
    quant2->AddInputDesc(out2_desc0);
    GeTensorDesc quant2_desc0(shape1, ge::FORMAT_NCHW, quant_data_type);
    quant2->AddOutputDesc(quant2_desc0);
    end->AddInputDesc(quant2_desc0);
    NodePtr concat0_node = graph->AddNode(concat0);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr maxpool_node = graph->AddNode(maxpool);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    NodePtr quant1_node = graph->AddNode(quant1);
    NodePtr quant2_node = graph->AddNode(quant2);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr end_node = graph->AddNode(end);
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), maxpool_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(maxpool_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), quant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant1_node->GetOutDataAnchor(0), conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0), dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), quant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant2_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }
  void InitSameInputDiffQuantGraph(ComputeGraphPtr &graph, ge::DataType quant_data_type) {
    OpDescPtr concat0 = std::make_shared<OpDesc>("concat0", CONCATD);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr maxpool = std::make_shared<OpDesc>("maxpool", "MaxPool");
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr quant1 = std::make_shared<OpDesc>("quant1", QUANT);
    OpDescPtr quant2 = std::make_shared<OpDesc>("quant2", QUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    OpDescPtr end = std::make_shared<OpDesc>("end", "End");

    ge::AttrUtils::SetFloat(quant1, "scale", 0.2);
    ge::AttrUtils::SetFloat(quant1, "offset", -128);
    ge::AttrUtils::SetFloat(quant2, "scale", 0.1);
    ge::AttrUtils::SetFloat(quant2, "offset", -128);
    // add descriptor
    ge::GeShape shape0({1, 64, 7, 7});
    GeTensorDesc out_desc0(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat0->AddOutputDesc(out_desc0);
    concat->AddInputDesc(out_desc0);
    concat->AddInputDesc(out_desc0);
    maxpool->AddInputDesc(out_desc0);
    maxpool->AddOutputDesc(out_desc0);
    quant1->AddInputDesc(out_desc0);
    GeTensorDesc quant1_outdesc(shape0, ge::FORMAT_NCHW, quant_data_type);
    quant1->AddOutputDesc(quant1_outdesc);
    conv1->AddInputDesc(quant1_outdesc);
    GeTensorDesc conv1_outdesc(shape0, ge::FORMAT_NCHW, ge::DT_INT32);
    conv1->AddOutputDesc(conv1_outdesc);
    dequant1->AddInputDesc(conv1_outdesc);

    GeTensorDesc out_desc3(shape0, ge::FORMAT_NCHW, ge::DT_FLOAT);
    dequant1->AddOutputDesc(conv1_outdesc);
    concat->AddInputDesc(conv1_outdesc);

    ge::GeShape shape1({1, 128, 7, 7});
    GeTensorDesc out2_desc0(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    concat->AddOutputDesc(out2_desc0);
    quant2->AddInputDesc(out2_desc0);
    GeTensorDesc quant2_desc0(shape1, ge::FORMAT_NCHW, quant_data_type);
    quant2->AddOutputDesc(quant2_desc0);
    end->AddInputDesc(quant2_desc0);
    NodePtr concat0_node = graph->AddNode(concat0);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr maxpool_node = graph->AddNode(maxpool);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    NodePtr quant1_node = graph->AddNode(quant1);
    NodePtr quant2_node = graph->AddNode(quant2);
    NodePtr concat_node = graph->AddNode(concat);
    NodePtr end_node = graph->AddNode(end);
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), maxpool_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(maxpool_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(concat0_node->GetOutDataAnchor(0), quant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant1_node->GetOutDataAnchor(0), conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0), dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0), concat_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0), quant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant2_node->GetOutDataAnchor(0), end_node->GetInDataAnchor(0));
  }

  static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name) {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for (ge::NodePtr node : graph->GetAllNodes()) {
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
};

TEST_F(STEST_concat_quant_fusion_pass, no_same_input) {
  vector <ge::DataType> quant_data_types = {ge::DT_INT4, ge::DT_INT8};
  for (auto quant_data_type: quant_data_types) {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
    InitGraph(graph, quant_data_type);
    ConcatQuantFusionPass pass;
    vector<GraphPass *> passes = {&pass};
    Status status = PassManager::Run(*graph, passes);
    DumpGraph(graph, "test");
    size_t node_num = graph->GetDirectNodesSize();
    EXPECT_EQ(node_num, 7);
  }
}

TEST_F(STEST_concat_quant_fusion_pass, same_input) {
  vector <ge::DataType> quant_data_types = {ge::DT_INT4, ge::DT_INT8};
  for (auto quant_data_type: quant_data_types) {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
    InitSameInputGraph(graph, quant_data_type);
    ConcatQuantFusionPass pass;
    vector<GraphPass *> passes = {&pass};
    Status status = PassManager::Run(*graph, passes);
    DumpGraph(graph, "test");
    size_t node_num = graph->GetDirectNodesSize();
    EXPECT_EQ(node_num, 8);
  }
}

TEST_F(STEST_concat_quant_fusion_pass, same_input_diff_quant) {
  vector <ge::DataType> quant_data_types = {ge::DT_INT4, ge::DT_INT8};
  for (auto quant_data_type: quant_data_types) {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
    InitSameInputDiffQuantGraph(graph, quant_data_type);
    ConcatQuantFusionPass pass;
    vector<GraphPass *> passes = {&pass};
    Status status = PassManager::Run(*graph, passes);
    DumpGraph(graph, "test");
    size_t node_num = graph->GetDirectNodesSize();
    EXPECT_EQ(node_num, 8);
  }
}
}
