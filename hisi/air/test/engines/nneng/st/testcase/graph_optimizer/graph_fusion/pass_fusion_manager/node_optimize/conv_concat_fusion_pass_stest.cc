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
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/conv_concat_fusion_pass.h"
#include <gtest/gtest.h>

#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {

class STEST_conv_concat_fusion_pass : public testing::Test {
 public:
  const string GRAPH_NAME = "test";
  const string CONCATD = "ConcatD";
  const string DEQUANT = "AscendDequant";
  const string CONV2D = "Conv2D";
  const string RELU = "Relu";
  const string LEAKYRELU = "LeakyRelu";
  const string STRIDEDWRITE = "StridedWrite";
  const string STRIDEDREAD = "StridedRead";
  const string STRIDE_ATTR_STRIDE = "stride";
  const string STRIDE_ATTR_AXIS = "axis";
  const int NCHW_DIM_N = 1;
  const int NCHW_DIM_H = 14;
  const int NCHW_DIM_W = 12;

protected:
  void SetUp() {}
  void TearDown() {}
  void InitNoQuantGraph(ComputeGraphPtr& graph, string relu_str) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({NCHW_DIM_N, 16, NCHW_DIM_H, NCHW_DIM_W});
    ge::GeShape shape2({NCHW_DIM_N, 32, NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);
    ge::GeShape shape({1, 48, 14, 14});
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc.SetOriginDataType(ge::DT_FLOAT);
    out_desc.SetOriginShape(shape);
    concat->AddOutputDesc(out_desc);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);

    /*
     *  Conv2d             Conv2d
     *      \               /
     *      (Relu)      (Relu)
     *           \      /
     *            Concat
     *              |
     */
    if (!relu_str.empty()) {
      OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", relu_str);
      OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", relu_str);
      relu1->AddInputDesc(out_desc1);
      relu1->AddOutputDesc(out_desc1);
      relu2->AddInputDesc(out_desc2);
      relu2->AddOutputDesc(out_desc2);
      NodePtr relu_node1 = graph->AddNode(relu1);
      NodePtr relu_node2 = graph->AddNode(relu2);
      ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                              relu_node1->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                              relu_node2->GetInDataAnchor(0));

      ge::GraphUtils::AddEdge(relu_node1->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu_node2->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(1));
    } else {
      ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(1));
    }
  }

  void InitQuantGraph(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr dequant2 = std::make_shared<OpDesc>("dequant2", DEQUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({NCHW_DIM_N, 32, NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_INT32);
    ge::GeShape shape2({NCHW_DIM_N, 64, NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_INT32);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    dequant1->AddInputDesc(out_desc1);
    dequant2->AddInputDesc(out_desc2);

    GeTensorDesc out_desc3(shape1, ge::FORMAT_NCHW, ge::DT_INT8);
    out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc3.SetOriginDataType(ge::DT_INT8);
    out_desc3.SetOriginShape(shape1);

    GeTensorDesc out_desc4(shape2, ge::FORMAT_NCHW, ge::DT_INT8);
    out_desc4.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc4.SetOriginDataType(ge::DT_INT8);
    out_desc4.SetOriginShape(shape2);
    dequant1->AddOutputDesc(out_desc3);
    dequant2->AddOutputDesc(out_desc4);

    concat->AddInputDesc(out_desc3);
    concat->AddInputDesc(out_desc4);
    ge::GeShape shape({NCHW_DIM_N, 96, NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc.SetOriginDataType(ge::DT_FLOAT);
    out_desc.SetOriginShape(shape);
    concat->AddOutputDesc(out_desc);

    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr dequant1_node = graph->AddNode(dequant1);
    NodePtr dequant2_node = graph->AddNode(dequant2);
    NodePtr concat_node = graph->AddNode(concat);
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                              dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                              dequant2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant2_node->GetOutDataAnchor(0),
                              concat_node->GetInDataAnchor(1));
  }


  void InitQuantAndConvAndReluAndLeakyreluGraph(ComputeGraphPtr& graph, vector<string> op_type_vec) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr dequant1 = std::make_shared<OpDesc>("dequant1", DEQUANT);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);
    ge::GeShape shape({NCHW_DIM_N, 32, NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc1(shape, ge::FORMAT_NCHW, ge::DT_INT32);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_INT32);
    out_desc1.SetOriginShape(shape);
    conv1->AddOutputDesc(out_desc1);
    dequant1->AddInputDesc(out_desc1);

    GeTensorDesc out_desc2(shape, ge::FORMAT_NCHW, ge::DT_INT8);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_INT8);
    out_desc2.SetOriginShape(shape);
    dequant1->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc2);

    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr dequant1_node = graph->AddNode(dequant1);

    GeTensorDesc out_desc3(shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc3.SetOriginDataType(ge::DT_FLOAT);
    out_desc3.SetOriginShape(shape);

    int op_type_vec_size = op_type_vec.size();
    for(int i = 0; i != op_type_vec_size; ++i) {
      concat->AddInputDesc(out_desc3);
    }

    ge::GeShape out_shape({NCHW_DIM_N, 32*(op_type_vec_size +1), NCHW_DIM_H, NCHW_DIM_W});
    GeTensorDesc out_desc(out_shape, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc.SetOriginDataType(ge::DT_FLOAT);
    out_desc.SetOriginShape(out_shape);
    concat->AddOutputDesc(out_desc);

    NodePtr concat_node = graph->AddNode(concat);
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            dequant1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(dequant1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));

    for(int i = 0; i != op_type_vec_size; ++i){
      OpDescPtr conv_op = std::make_shared<OpDesc>("conv"+to_string(i), CONV2D);
      out_desc3.SetOriginFormat(ge::FORMAT_NCHW);
      out_desc3.SetOriginDataType(ge::DT_FLOAT16);
      out_desc3.SetOriginShape(shape);
      conv_op->AddOutputDesc(out_desc3);
      NodePtr conv_node = graph->AddNode(conv_op);
      if(op_type_vec[i] == CONV2D) {
        ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                                concat_node->GetInDataAnchor(i+1));
      } else {
        OpDescPtr cur_op =
            std::make_shared<OpDesc>(op_type_vec[i] + to_string(i), op_type_vec[i]);
        cur_op->AddInputDesc(out_desc3);
        cur_op->AddOutputDesc(out_desc3);
        if (op_type_vec[i] == LEAKYRELU) {
          (void)ge::AttrUtils::SetFloat(cur_op, "negative_slope",0.0);
        }
        NodePtr cur_node = graph->AddNode(cur_op);
        ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                                cur_node->GetInDataAnchor(0));
        ge::GraphUtils::AddEdge(cur_node->GetOutDataAnchor(0),
                                concat_node->GetInDataAnchor(i + 1));
      }
    }
  }

  Status CheckGraph(ComputeGraphPtr& graph, int expect_axis) {
    for (auto& node : graph->GetDirectNode()) {
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
        for (auto& input_anchor : node->GetAllInDataAnchors()) {
          EXPECT_NE(input_anchor, nullptr);
          EXPECT_NE(input_anchor->GetPeerOutAnchor(), nullptr);
          EXPECT_NE(input_anchor->GetPeerOutAnchor()->GetOwnerNode(), nullptr);
          // check stridedwride op
          auto peer_output_node = input_anchor->GetPeerOutAnchor()->GetOwnerNode();
          EXPECT_EQ(peer_output_node->GetType(), STRIDEDWRITE);
          OpDescPtr peer_output_op_desc = peer_output_node->GetOpDesc();
          int axis = 0;
          (void)ge::AttrUtils::GetInt(peer_output_op_desc, STRIDE_ATTR_AXIS, axis);
          EXPECT_EQ(axis, expect_axis);
        }
      }
    }
    return SUCCESS;
  }
};

// conv2d+concat
// con2d+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitNoQuantGraph(graph, "");
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+concat
// conv2d+relu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_relu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitNoQuantGraph(graph, RELU);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+concat
// conv2d+leakyrelu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_leakyrelu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitNoQuantGraph(graph, RELU);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+acsenddequant+concat
// conv2d+acsenddequant+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {DEQUANT};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  InitQuantGraph(graph);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  //DumpGraph(graph, GRAPH_NAME);
  CheckGraph(graph, 1);
}

// conv2d+acsenddequant+concat
// conv2d+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_conv_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {CONV2D};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  //DumpGraph(graph, GRAPH_NAME);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+relu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_relu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {RELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+leakyrelu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_leakyrelu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {LEAKYRELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+relu+concat
// conv2d+leakyrelu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_relu_leakyrelu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {RELU, LEAKYRELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+concat
// conv2d+relu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_conv_relu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {CONV2D, RELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+concat
// conv2d+leakyrelu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_conv_leakyrelu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {CONV2D, LEAKYRELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);
  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  CheckGraph(graph, 1);
}

// conv2d+dequant+concat
// conv2d+concat
// conv2d+relu+concat 
// conv2d+leakyrelu+concat
TEST_F(STEST_conv_concat_fusion_pass, con2d_dequant_and_conv_relu_leakyrelu_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  vector<string> op_type_vec = {CONV2D, RELU, LEAKYRELU};
  InitQuantAndConvAndReluAndLeakyreluGraph(graph, op_type_vec);

  ConvConcatFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);

  CheckGraph(graph, 1);
}
}  // namespace fe