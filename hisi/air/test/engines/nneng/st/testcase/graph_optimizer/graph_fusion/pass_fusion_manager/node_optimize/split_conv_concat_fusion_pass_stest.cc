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
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/split_conv_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/split_conv_concat_fusion_pass.h"
#include <gtest/gtest.h>


#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {

class STEST_split_conv_concat_fusion_pass : public testing::Test {
public:
  const string GRAPH_NAME = "test";
  const string SPLITD = "SplitD";
  const string CONCATD = "ConcatD";
  const string DEQUANT = "AscendDequant";
  const string QUANT = "AscendQuant";
  const string CONV2D = "Conv2D";
  const string RELU = "Relu";
  const string STRIDEDWRITE = "StridedWrite";
  const string STRIDEDREAD = "StridedRead";
  const string STRIDE_ATTR_STRIDE = "stride";
  const string STRIDE_ATTR_AXIS = "axis";

protected:
  void SetUp() {}
  void TearDown() {}
  void InitGraph(ComputeGraphPtr &graph, bool with_relu) {
    OpDescPtr data = std::make_shared<OpDesc>("split", RELU);
    OpDescPtr split = std::make_shared<OpDesc>("split", SPLITD);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(split, SPLIT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 32, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    ge::GeShape shape3({1, 64, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT16);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT16);
    out_desc2.SetOriginShape(shape2);
    GeTensorDesc out_desc3(shape3, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc3.SetOriginDataType(ge::DT_FLOAT16);
    out_desc3.SetOriginShape(shape2);
    data->AddInputDesc(out_desc3);
    data->AddOutputDesc(out_desc3);
    split->AddInputDesc(out_desc3);
    split->AddOutputDesc(out_desc1);
    split->AddOutputDesc(out_desc2);
    conv1->AddInputDesc(out_desc1);
    conv2->AddInputDesc(out_desc2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);
    concat->AddOutputDesc(out_desc3);
    // create nodes
    NodePtr data_node = graph->AddNode(data);
    NodePtr split_node = graph->AddNode(split);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);

    ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                            split_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(1),
                            conv2_node->GetInDataAnchor(0));

    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
    if (with_relu) {
      OpDescPtr relu = std::make_shared<OpDesc>("relu1", RELU);
      relu->AddInputDesc(out_desc3);
      relu->AddOutputDesc(out_desc3);
      NodePtr relu_node = graph->AddNode(relu);
      ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                              relu_node->GetInDataAnchor(0));
    } else {
      OpDescPtr split2 = std::make_shared<OpDesc>("split2", SPLITD);
      split2->AddInputDesc(out_desc3);
      NodePtr split2_node = graph->AddNode(split2);
      ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                              split2_node->GetInDataAnchor(0));
    }
  }

  void InitQuantGraph(ComputeGraphPtr &graph, bool with_quant) {
    OpDescPtr relu = std::make_shared<OpDesc>("relu1", RELU);
    OpDescPtr split = std::make_shared<OpDesc>("split", SPLITD);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr quant1 = std::make_shared<OpDesc>("quant1", QUANT);
    OpDescPtr quant2 = std::make_shared<OpDesc>("quant2", QUANT);
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr dequant2 = std::make_shared<OpDesc>("dequant2", DEQUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(split, SPLIT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_INT32);
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_INT32);
    ge::GeShape shape3({1, 64, 14, 14});
    GeTensorDesc out_desc5(shape3, ge::FORMAT_NCHW, ge::DT_INT32);
    out_desc5.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc5.SetOriginDataType(ge::DT_FLOAT16);
    out_desc5.SetOriginShape(shape3);
    split->AddInputDesc(out_desc5);
    quant1->AddOutputDesc(out_desc1);
    quant2->AddOutputDesc(out_desc2);
    conv1->AddInputDesc(out_desc1);
    conv2->AddInputDesc(out_desc2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    dequant1->AddInputDesc(out_desc1);
    dequant2->AddInputDesc(out_desc2);

    GeTensorDesc out_desc3(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc3.SetOriginDataType(ge::DT_FLOAT16);
    out_desc3.SetOriginShape(shape1);

    GeTensorDesc out_desc4(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc4.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc4.SetOriginDataType(ge::DT_FLOAT16);
    out_desc4.SetOriginShape(shape2);

    relu->AddOutputDesc(out_desc5);

    split->AddOutputDesc(out_desc3);
    split->AddOutputDesc(out_desc4);

    dequant1->AddOutputDesc(out_desc3);
    dequant2->AddOutputDesc(out_desc4);

    quant1->AddInputDesc(out_desc3);
    quant2->AddInputDesc(out_desc4);

    concat->AddInputDesc(out_desc3);
    concat->AddInputDesc(out_desc4);

    concat->AddOutputDesc(out_desc5);

    NodePtr relu_node = graph->AddNode(relu);
    NodePtr split_node = graph->AddNode(split);
    NodePtr quant1_node = graph->AddNode(quant1);
    NodePtr quant2_node = graph->AddNode(quant2);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    NodePtr dequant2_node = graph->AddNode(dequant2);
    NodePtr concat_node = graph->AddNode(concat);

    /*             split
     *           /       \
     *         /           \
     *  AcscendQuant   AcscendQuant
     *     /                \
     *  Conv2d             Conv2d
     *      \               /
     * AcsendDequant AcsendDequant
     *         \          /
     *           \      /
     *            Concat
     *               |
     *          AcscendQuant
     */

    ge::GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0),
                            split_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                            quant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(1),
                            quant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant1_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(quant2_node->GetOutDataAnchor(0),
                            conv2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            dequant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));

    if (with_quant) {
      OpDescPtr quant3 = std::make_shared<OpDesc>("quant3", QUANT);
      quant3->AddInputDesc(out_desc5);
      quant3->AddOutputDesc(out_desc5);
      NodePtr quant_node3 = graph->AddNode(quant3);
      ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                              quant_node3->GetInDataAnchor(0));
      OpDescPtr relu = std::make_shared<OpDesc>("relu1", RELU);
      relu->AddInputDesc(out_desc5);
      NodePtr relu_node = graph->AddNode(relu);
      ge::GraphUtils::AddEdge(quant_node3->GetOutDataAnchor(0),
                              relu_node->GetInDataAnchor(0));
    } else {
      OpDescPtr split2 = std::make_shared<OpDesc>("split2", SPLITD);
      split2->AddInputDesc(out_desc3);
      NodePtr split2_node = graph->AddNode(split2);
      ge::GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                              split2_node->GetInDataAnchor(0));
    }
  }

  void InitInvalidGraph(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("split", RELU);
    OpDescPtr data1 = std::make_shared<OpDesc>("data1", "Data");
    OpDescPtr data2 = std::make_shared<OpDesc>("data2", "Data");
    OpDescPtr split = std::make_shared<OpDesc>("split", SPLITD);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(split, SPLIT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 32, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    ge::GeShape shape3({1, 64, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT16);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT16);
    out_desc2.SetOriginShape(shape2);
    GeTensorDesc out_desc3(shape3, ge::FORMAT_NCHW, ge::DT_FLOAT16);
    out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc3.SetOriginDataType(ge::DT_FLOAT16);
    out_desc3.SetOriginShape(shape2);
    data->AddInputDesc(out_desc3);
    data->AddOutputDesc(out_desc3);
    data1->AddOutputDesc(out_desc3);
    data2->AddOutputDesc(out_desc3);
    split->AddInputDesc(out_desc3);
    split->AddOutputDesc(out_desc1);
    split->AddOutputDesc(out_desc2);
    conv1->AddInputDesc(out_desc1);
    conv1->AddInputDesc(out_desc1);
    conv2->AddInputDesc(out_desc2);
    conv2->AddInputDesc(out_desc2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);
    concat->AddOutputDesc(out_desc3);
    // create nodes
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr split_node = graph->AddNode(split);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);

    ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                            split_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0),
                            conv2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(1),
                            conv2_node->GetInDataAnchor(1));

    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  Status CheckGraph(ComputeGraphPtr &graph) {
    for (auto &node : graph->GetDirectNode()) {
      FE_CHECK_NOTNULL(node);
      if (node->GetType() == CONCATD) {
        OpDescPtr concat_op_desc_ptr = node->GetOpDesc();
        bool no_task = false;
        bool output_reuse_input = false;
        bool no_padding_continuous_input = false;
        bool no_padding_continuous_output = false;
        int dim_index = -1;
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr, ge::ATTR_NAME_NOTASK,
                                     no_task);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_OUTPUT_REUSE_INPUT,
                                     output_reuse_input);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT,
                                     no_padding_continuous_input);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT,
                                     no_padding_continuous_output);
        (void)ge::AttrUtils::GetInt(
            concat_op_desc_ptr, ge::ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, dim_index);

        EXPECT_EQ(no_task, true);
        EXPECT_EQ(output_reuse_input, true);
        EXPECT_EQ(no_padding_continuous_input, true);
        EXPECT_EQ(no_padding_continuous_output, false);
        EXPECT_EQ(dim_index, 1);
        for (auto &input_anchor : node->GetAllInDataAnchors()) {
          EXPECT_NE(input_anchor, nullptr);
          EXPECT_NE(input_anchor->GetPeerOutAnchor(), nullptr);
          EXPECT_NE(input_anchor->GetPeerOutAnchor()->GetOwnerNode(), nullptr);
          auto peer_output_node = input_anchor->GetPeerOutAnchor()->GetOwnerNode();
          EXPECT_EQ(peer_output_node->GetType(), STRIDEDWRITE);
          OpDescPtr peer_output_op_desc = peer_output_node->GetOpDesc();
          int axis = 0;
          (void)ge::AttrUtils::GetInt(peer_output_op_desc, STRIDE_ATTR_AXIS, axis);
          EXPECT_EQ(axis, 1);
        }
      }
    }
    return SUCCESS;
  }
  Status CheckSplitGraph(ComputeGraphPtr &graph) {
    for (auto &node : graph->GetDirectNode()) {
      FE_CHECK_NOTNULL(node);
      if (node->GetType() == SPLITD) {
        OpDescPtr concat_op_desc_ptr = node->GetOpDesc();
        bool no_task = false;
        bool output_reuse_input = false;
        bool no_padding_continuous_input = false;
        bool no_padding_continuous_output = false;
        int dim_index = -1;
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr, ge::ATTR_NAME_NOTASK,
                                     no_task);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_OUTPUT_REUSE_INPUT,
                                     output_reuse_input);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT,
                                     no_padding_continuous_input);
        (void)ge::AttrUtils::GetBool(concat_op_desc_ptr,
                                     ge::ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT,
                                     no_padding_continuous_output);
        (void)ge::AttrUtils::GetInt(
                concat_op_desc_ptr, ge::ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, dim_index);

        EXPECT_EQ(no_task, true);
        EXPECT_EQ(output_reuse_input, true);
        EXPECT_EQ(no_padding_continuous_input, false);
        EXPECT_EQ(no_padding_continuous_output, true);
        EXPECT_EQ(dim_index, 1);
        for (auto &peer_output_node : node->GetOutDataNodes()) {
          EXPECT_EQ(peer_output_node->GetType(), STRIDEDREAD);
          OpDescPtr peer_output_op_desc = peer_output_node->GetOpDesc();
          int axis = 0;
          (void)ge::AttrUtils::GetInt(peer_output_op_desc, STRIDE_ATTR_AXIS, axis);
          EXPECT_EQ(axis, 1);
        }
      }
    }
    return SUCCESS;
  }
  void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name) {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for (ge::NodePtr node : graph->GetAllNodes()) {
      printf("node name = %s, node type = %s.\n", node->GetName().c_str(),
             node->GetType().c_str());
      for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
        for (ge::InDataAnchorPtr peer_in_anchor :
             anchor->GetPeerInDataAnchors()) {
          printf("node name = %s[%d], out data node name = %s[%d].\n",
                 node->GetName().c_str(), anchor->GetIdx(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                 peer_in_anchor->GetIdx());
        }
      }
      if (node->GetOutControlAnchor() != nullptr) {
        for (ge::InControlAnchorPtr peer_in_anchor :
             node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
          printf("node name = %s, out control node name = %s, type = %s.\n",
                 node->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetType().c_str());
        }
      }
    }
    printf("end to dump graph %s...\n", graph_name.c_str());
    return;
  }
};

TEST_F(STEST_split_conv_concat_fusion_pass, con2d_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph(graph, false);
  DumpGraph(graph, GRAPH_NAME);
  SplitConvConcatFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  DumpGraph(graph, GRAPH_NAME);
  CheckGraph(graph);
}

TEST_F(STEST_split_conv_concat_fusion_pass, con2d_relu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph(graph, true);
  DumpGraph(graph, GRAPH_NAME);
  SplitConvConcatFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  DumpGraph(graph, GRAPH_NAME);
  CheckGraph(graph);
}

TEST_F(STEST_split_conv_concat_fusion_pass, con2d_dequant_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitQuantGraph(graph, false);
  SplitConvConcatFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  DumpGraph(graph, GRAPH_NAME);
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph);
}

TEST_F(STEST_split_conv_concat_fusion_pass, con2d_dequant_relu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitQuantGraph(graph, true);
  SplitConvConcatFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  DumpGraph(graph, GRAPH_NAME);
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph);
}

TEST_F(STEST_split_conv_concat_fusion_pass, split_con2d_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph(graph, true);
  DumpGraph(graph, GRAPH_NAME);
  SplitConvFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  DumpGraph(graph, GRAPH_NAME);
  CheckSplitGraph(graph);
}

TEST_F(STEST_split_conv_concat_fusion_pass, split_conv2d_invalid) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitInvalidGraph(graph);
  DumpGraph(graph, GRAPH_NAME);
  SplitConvFusionPass pass;
  vector<GraphPass *> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(NOT_CHANGED, status);
  DumpGraph(graph, GRAPH_NAME);
}
} // namespace fe