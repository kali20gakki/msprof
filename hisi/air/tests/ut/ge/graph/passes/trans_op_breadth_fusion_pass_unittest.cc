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

#include "graph/passes/transop_breadth_fusion_pass.h"

#include <gtest/gtest.h>
#include <string>

#include "common/ge_inner_error_codes.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/node_adapter.h"
using namespace ge;

class UtestGraphPassesTransOpBreadthFusionPass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

class NodeBuilder {
 public:
  NodeBuilder(const std::string &name, const std::string &type) { op_desc_ = std::make_shared<OpDesc>(name, type); }

  NodeBuilder &AddInputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                            ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                             ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  ge::NodePtr Build(const ge::ComputeGraphPtr &graph) { return graph->AddNode(op_desc_); }

 private:
  ge::GeTensorDescPtr CreateTensorDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                                       ge::DataType data_type = DT_FLOAT) {
    GeShape ge_shape{std::vector<int64_t>(shape)};
    ge::GeTensorDescPtr tensor_desc = std::make_shared<ge::GeTensorDesc>();
    tensor_desc->SetShape(ge_shape);
    tensor_desc->SetFormat(format);
    tensor_desc->SetDataType(data_type);
    return tensor_desc;
  }

  ge::OpDescPtr op_desc_;
};
/*
TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_simple_trans_data) {
  ///             ___ NodeTrans4DToFZ_1 __ NodeFZ
  ///            |
  ///            |___ NodeTrans4DToFZ_2 __ NodeFZ
  ///   Node4D __|
  ///             |___ NodeTrans4dTo5D_1 __ Node5D
  ///            |
  ///             |___ NodeTrans4DTo5D_2 __ Node5D
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  // Node4D
  ge::NodePtr node_4d = NodeBuilder("Node4D", DATA).AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT32).Build(graph);

  // NodeTrans4DToFZ
  ge::NodePtr node_4d_to_fz_1 = NodeBuilder("4d_to_fz_1", TRANSDATA)
                                    .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT32)
                                    .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_FRACTAL_Z, DT_FLOAT)
                                    .Build(graph);

  ge::NodePtr node_4d_to_fz_2 = NodeBuilder("4d_to_fz_2", TRANSDATA)
                                    .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT32)
                                    .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_FRACTAL_Z, DT_FLOAT)
                                    .Build(graph);

  // NodeTrans4DTo5D
  ge::NodePtr node_4d_to_5d_1 = NodeBuilder("4d_to_5d_1", TRANSDATA)
                                    .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT32)
                                    .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
                                    .Build(graph);

  ge::NodePtr node_4d_to_5d_2 = NodeBuilder("4d_to_5d_1", TRANSDATA)
                                    .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT32)
                                    .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
                                    .Build(graph);

  // NodeFZ
  ge::NodePtr node_fz_1 =
      NodeBuilder("FZ_1", RELU).AddInputDesc({1, 2, 3, 4, 5}, FORMAT_FRACTAL_Z, DT_FLOAT).Build(graph);

  ge::NodePtr node_fz_2 =
      NodeBuilder("FZ_2", RELU).AddInputDesc({1, 2, 3, 4, 5}, FORMAT_FRACTAL_Z, DT_FLOAT).Build(graph);

  // Node5D
  ge::NodePtr node_5d_1 =
      NodeBuilder("5D_1", RELU).AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT).Build(graph);

  ge::NodePtr node_5d_2 =
      NodeBuilder("5D_2", RELU).AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT).Build(graph);

  // add edge
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_to_fz_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_to_fz_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_to_5d_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_to_5d_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_to_fz_1->GetOutDataAnchor(0), node_fz_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_to_fz_2->GetOutDataAnchor(0), node_fz_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_to_5d_1->GetOutDataAnchor(0), node_5d_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_to_5d_2->GetOutDataAnchor(0), node_5d_2->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(node_4d_to_fz_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(node_4d_to_5d_1->GetOutDataNodes().size(), 2);
  EXPECT_TRUE(node_4d_to_fz_2->GetOutDataNodes().empty());
  EXPECT_TRUE(node_4d_to_5d_2->GetOutDataNodes().empty());
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_simple_cast) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr node1 = NodeBuilder("node1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_INT32).Build(graph);

  ge::NodePtr cast_node_1 = NodeBuilder("cast_node_1", CAST)
                                .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                                .Build(graph);

  ge::NodePtr cast_node_2 = NodeBuilder("cast_node_2", CAST)
                                .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                                .Build(graph);

  ge::NodePtr node_2 = NodeBuilder("node2", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr node_3 = NodeBuilder("node3", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), cast_node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), cast_node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_node_1->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_node_2->GetOutDataAnchor(0), node_3->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(cast_node_1->GetOutDataNodes().size(), 2);
  EXPECT_TRUE(cast_node_2->GetOutDataNodes().empty());
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_simple_reshape) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr node1 = NodeBuilder("node1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_INT32).Build(graph);

  ge::NodePtr reshape_node_1 = NodeBuilder("reshape_node_1", RESHAPE)
                                   .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                   .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32)
                                   .Build(graph);

  ge::NodePtr reshape_node_2 = NodeBuilder("reshape_node_2", RESHAPE)
                                   .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                   .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32)
                                   .Build(graph);

  ge::NodePtr node_2 = NodeBuilder("node2", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32).Build(graph);

  ge::NodePtr node_3 = NodeBuilder("node3", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32).Build(graph);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), reshape_node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), reshape_node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(reshape_node_1->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(reshape_node_2->GetOutDataAnchor(0), node_3->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(reshape_node_1->GetOutDataNodes().size(), 2);
  EXPECT_TRUE(reshape_node_2->GetOutDataNodes().empty());
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_simple_transpose) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr node1 = NodeBuilder("node1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_INT32).Build(graph);

  ge::NodePtr transpose_node_1 = NodeBuilder("transpose_node_1", TRANSPOSE)
                                     .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                     .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32)
                                     .Build(graph);

  ge::NodePtr transpose_node_2 = NodeBuilder("transpose_node_2", TRANSPOSE)
                                     .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                     .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32)
                                     .Build(graph);

  ge::NodePtr node_2 = NodeBuilder("node2", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32).Build(graph);

  ge::NodePtr node_3 = NodeBuilder("node3", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT32).Build(graph);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), transpose_node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), transpose_node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transpose_node_1->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transpose_node_2->GetOutDataAnchor(0), node_3->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(transpose_node_1->GetOutDataNodes().size(), 2);
  EXPECT_TRUE(transpose_node_2->GetOutDataNodes().empty());
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_partial_matching) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr node1 = NodeBuilder("node1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_INT32).Build(graph);

  ge::NodePtr cast_node = NodeBuilder("cast_node", CAST)
                              .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                              .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                              .Build(graph);

  ge::NodePtr transdata_node_1 = NodeBuilder("transdata_node_1", TRANSDATA)
                                     .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                     .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_FLOAT)
                                     .Build(graph);

  ge::NodePtr transdata_node_2 = NodeBuilder("transdata_node_2", TRANSDATA)
                                     .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                     .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_FLOAT)
                                     .Build(graph);

  ge::NodePtr transdata_node_3 = NodeBuilder("transdata_node_3", TRANSDATA)
                                     .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                     .AddOutputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT16)
                                     .Build(graph);

  ge::NodePtr node_2 = NodeBuilder("node2", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_FLOAT).Build(graph);

  ge::NodePtr node_3 = NodeBuilder("node3", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_FLOAT).Build(graph);

  ge::NodePtr node_4 = NodeBuilder("node4", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_FLOAT).Build(graph);

  ge::NodePtr node_5 = NodeBuilder("node5", RELU).AddInputDesc({1, 1}, FORMAT_NC1HWC0, DT_INT16).Build(graph);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), transdata_node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), transdata_node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), transdata_node_3->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transdata_node_1->GetOutDataAnchor(0), node_3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transdata_node_2->GetOutDataAnchor(0), node_4->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transdata_node_3->GetOutDataAnchor(0), node_5->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(cast_node->GetOutDataNodes().size(), 1);
  EXPECT_EQ(transdata_node_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(transdata_node_3->GetOutDataNodes().size(), 1);
  EXPECT_TRUE(transdata_node_2->GetOutDataNodes().empty());
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_control_anchor) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr node1 = NodeBuilder("node1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_INT32).Build(graph);

  ge::NodePtr cast_node_1 = NodeBuilder("cast_node_1", CAST)
                                .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                                .Build(graph);

  ge::NodePtr cast_node_2 = NodeBuilder("cast_node_2", CAST)
                                .AddInputDesc({1}, FORMAT_NCHW, DT_INT32)
                                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                                .Build(graph);

  ge::NodePtr node_2 = NodeBuilder("node2", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr node_3 = NodeBuilder("node3", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), cast_node_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), cast_node_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_node_1->GetOutDataAnchor(0), node_2->GetInControlAnchor());
  ge::GraphUtils::AddEdge(cast_node_2->GetOutDataAnchor(0), node_3->GetInControlAnchor());

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(cast_node_1->GetOutControlNodes().size(), 2);
  EXPECT_TRUE(cast_node_2->GetOutControlNodes().empty());
  EXPECT_TRUE(cast_node_1->GetOutDataNodes().empty());
}
*/

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_reshape_op_failed) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr data1 = NodeBuilder("data1", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr data2 = NodeBuilder("data2", DATA).AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr constant = NodeBuilder("constant", CONSTANT).AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr exp1 = NodeBuilder("exp1", EXP)
                         .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                         .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                         .Build(graph);

  ge::NodePtr exp2 = NodeBuilder("exp2", EXP)
                         .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                         .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                         .Build(graph);

  ge::NodePtr reshape1 = NodeBuilder("reshape1", RESHAPE)
                             .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .Build(graph);

  ge::NodePtr reshape2 = NodeBuilder("reshape2", RESHAPE)
                             .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                             .Build(graph);

  ge::NodePtr relu1 = NodeBuilder("relu1", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::NodePtr relu2 = NodeBuilder("relu2", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT).Build(graph);

  ge::GraphUtils::AddEdge(data1->GetOutDataAnchor(0), exp1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(data2->GetOutDataAnchor(0), exp2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(exp1->GetOutDataAnchor(0), reshape1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(exp2->GetOutDataAnchor(0), reshape2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(constant->GetOutDataAnchor(0), reshape1->GetInDataAnchor(1));
  ge::GraphUtils::AddEdge(constant->GetOutDataAnchor(0), reshape2->GetInDataAnchor(1));
  ge::GraphUtils::AddEdge(reshape1->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(reshape2->GetOutDataAnchor(0), relu2->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(reshape1->GetOutDataNodes().size(), 1);
  EXPECT_EQ(reshape2->GetOutDataNodes().size(), 1);
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, test_multi_anchor_case) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  ge::NodePtr data1 = NodeBuilder("data1", DATA)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);

  ge::NodePtr cast1 = NodeBuilder("cast1", CAST)
                          .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT16)
                          .Build(graph);

  ge::NodePtr cast2 = NodeBuilder("cast2", CAST)
                          .AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT16)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);

  ge::NodePtr relu1 = NodeBuilder("relu1", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT16).Build(graph);

  ge::NodePtr relu2 = NodeBuilder("relu2", RELU).AddInputDesc({1}, FORMAT_NCHW, DT_FLOAT16).Build(graph);

  ge::GraphUtils::AddEdge(data1->GetOutDataAnchor(0), cast1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(data1->GetOutDataAnchor(1), cast2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast1->GetOutDataAnchor(0), relu1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast2->GetOutDataAnchor(0), relu2->GetInDataAnchor(0));

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(cast1->GetOutDataNodes().size(), 1);
  EXPECT_EQ(cast1->GetOutDataNodes().size(), 1);
}

///           ----> netoutput1
///         /        |       \.
/// transdata1    transdata2  transdata3
///          \   /             |
///           var1--------------
static ComputeGraphPtr BuildGraph1() {
  ut::GraphBuilder builder("g1");
  auto var1 = builder.AddNode("var1", "Variable", 0, 1);
  auto transdata1 = builder.AddNode("transdata1", "TransData", 1, 1);
  transdata1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  AttrUtils::SetStr(transdata1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label1");
  auto transdata2 = builder.AddNode("transdata2", "TransData", 1, 1);
  transdata2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  auto transdata3 = builder.AddNode("transdata3", "TransData", 1, 1);
  transdata3->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata3->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput1", 10, 0);

  builder.AddDataEdge(var1, 0, transdata1, 0);
  builder.AddDataEdge(var1, 0, transdata2, 0);
  builder.AddDataEdge(var1, 0, transdata3, 0);
  builder.AddDataEdge(transdata1, 0, netoutput1, 0);
  builder.AddDataEdge(transdata2, 0, netoutput1, 1);
  builder.AddDataEdge(transdata3, 0, netoutput1, 2);

  return builder.GetGraph();
}

///           --------->   netoutput1
///         /              |       \.
/// transdata1  transdata2(l1)  transdata3(l1)
///          \   /                  |
///           var1------------------
static ComputeGraphPtr BuildGraph2() {
  ut::GraphBuilder builder("g2");
  auto var1 = builder.AddNode("var1", "Variable", 0, 1);
  auto transdata1 = builder.AddNode("transdata1", "TransData", 1, 1);
  transdata1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  auto transdata2 = builder.AddNode("transdata2", "TransData", 1, 1);
  transdata2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  AttrUtils::SetStr(transdata2->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label1");
  auto transdata3 = builder.AddNode("transdata3", "TransData", 1, 1);
  transdata3->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NC1HWC0);
  transdata3->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 1, 224, 224, 16})));
  AttrUtils::SetStr(transdata3->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label1");
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput1", 10, 0);

  builder.AddDataEdge(var1, 0, transdata1, 0);
  builder.AddDataEdge(var1, 0, transdata2, 0);
  builder.AddDataEdge(var1, 0, transdata3, 0);
  builder.AddDataEdge(transdata1, 0, netoutput1, 0);
  builder.AddDataEdge(transdata2, 0, netoutput1, 1);
  builder.AddDataEdge(transdata3, 0, netoutput1, 2);

  return builder.GetGraph();
}

///                    netoutput
///                    /       \
/// const1  --  transpose1    transpose2  --  const2
///                       \   /
///                        var
static ComputeGraphPtr BuildGraph3() {
 vector<int64_t> perm1{0, 3, 1, 2};
  GeTensorDesc tensor_desc1(GeShape(vector<int64_t>{4}), FORMAT_ND, DT_INT64);
  GeTensorPtr const_tensor1 = 
    std::make_shared<GeTensor>(tensor_desc1, reinterpret_cast<uint8_t *>(perm1.data()) , sizeof(int64_t)*perm1.size());
  auto const1 = OP_CFG(CONSTANT).Weight(const_tensor1);

  vector<int32_t> perm2{0, 2, 1, 3};
  GeTensorDesc tensor_desc2(GeShape(vector<int64_t>{4}), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor2 = 
    std::make_shared<GeTensor>(tensor_desc2, reinterpret_cast<uint8_t *>(perm2.data()), sizeof(int32_t)*perm2.size());
  auto const2 = OP_CFG(CONSTANT).Weight(const_tensor2);

  auto transpose1 = OP_CFG(TRANSPOSE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 128, 52, 52});
  auto transpose2 = OP_CFG(TRANSPOSE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 128, 52, 52});

  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("transpose1", transpose1));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("transpose2", transpose2));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("transpose1", transpose1));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("transpose2", transpose2));
    CHAIN(NODE("transpose1", transpose1)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("transpose2", transpose2)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
  };

  auto graph = ToGeGraph(g1);

  map<string, uint32_t> name_index;
  name_index.emplace("x", 0);
  name_index.emplace("perm", 1);
  for (auto &gn : graph.GetAllNodes()) {
    AscendString type;
    (void)gn.GetType(type);
    if (type == TRANSPOSE) {
      auto node = NodeAdapter::GNode2Node(gn);
      if (node != nullptr && node->GetOpDesc()) {
        node->GetOpDesc()->UpdateInputName(name_index);
      }
    }
  }

  return ge::GraphUtils::GetComputeGraph(graph);
}

/*
*  if we want to fusion cast1 and cast2
*  it will cause a cycle between fusion_cast and transdata
*       data1
*       /    \
*      /      \
*    cast1     \
*      |        \
*   trandata---> cast2
*/
static ComputeGraphPtr BuildGraphMayCauseCycleWhenFusion() {
  auto root_builder = ut::GraphBuilder("root");
  const auto &data1 = root_builder.AddNode("data1", "Data", 1, 1);
  const auto &cast1 = root_builder.AddNode("cast1", "Cast", 1, 1);
  const auto &cast2 = root_builder.AddNode("cast2", "Cast", 1, 1);
  const auto &transdata = root_builder.AddNode("transdata", "TransData", 1, 1);

  root_builder.AddDataEdge(data1, 0, cast1, 0);
  root_builder.AddDataEdge(data1, 0, cast2, 0);
  root_builder.AddDataEdge(cast1, 0, transdata, 0);
  root_builder.AddControlEdge(transdata, cast2);
  return root_builder.GetGraph();
}

/*
*  if we want to fusion cast1 and cast2
*  it will cause a cycle between fusion_cast and transdata
*       data1
*       /     \
*      /       \
*    cast1 --->cast2
*      |        
*   trandata
*/
static ComputeGraphPtr BuildGraphWithCtrlEdgeBetweenTransop() {
  auto root_builder = ut::GraphBuilder("root");
  const auto &data1 = root_builder.AddNode("data1", "Data", 1, 1);
  const auto &cast1 = root_builder.AddNode("cast1", "Cast", 1, 1);
  const auto &cast2 = root_builder.AddNode("cast2", "Cast", 1, 1);
  const auto &transdata = root_builder.AddNode("transdata", "TransData", 1, 1);

  root_builder.AddDataEdge(data1, 0, cast1, 0);
  root_builder.AddDataEdge(data1, 0, cast2, 0);
  root_builder.AddDataEdge(cast1, 0, transdata, 0);
  root_builder.AddControlEdge(cast1, cast2);
  return root_builder.GetGraph();
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, diff_stream1) {
  auto graph = BuildGraph1();

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);

  auto transdata1 = graph->FindNode("transdata1");
  auto transdata2 = graph->FindNode("transdata2");
  auto transdata3 = graph->FindNode("transdata3");

  EXPECT_EQ(transdata1->GetOutNodes().size(), 1);
  EXPECT_EQ(transdata1->GetOutDataNodes().at(0)->GetName(), "netoutput1");
  EXPECT_EQ(transdata1->GetInNodes().size(), 1);
  EXPECT_EQ(transdata1->GetInDataNodes().at(0)->GetName(), "var1");

  EXPECT_TRUE(transdata2 == nullptr || transdata3 == nullptr);
  EXPECT_FALSE(transdata2 == nullptr && transdata3 == nullptr);
  auto not_empty_node = transdata2 != nullptr ? transdata2 : transdata3;
  EXPECT_FALSE(not_empty_node->GetInNodes().empty());
  EXPECT_EQ(not_empty_node->GetInDataNodes().at(0)->GetName(), "var1");
  EXPECT_FALSE(not_empty_node->GetOutNodes().empty());
  EXPECT_EQ(not_empty_node->GetOutDataNodes().at(0)->GetName(), "netoutput1");
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, diff_stream2) {
  auto graph = BuildGraph2();

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);

  auto transdata1 = graph->FindNode("transdata1");
  auto transdata2 = graph->FindNode("transdata2");
  auto transdata3 = graph->FindNode("transdata3");

  EXPECT_EQ(transdata1->GetOutNodes().size(), 1);
  EXPECT_EQ(transdata1->GetOutDataNodes().at(0)->GetName(), "netoutput1");
  EXPECT_EQ(transdata1->GetInNodes().size(), 1);
  EXPECT_EQ(transdata1->GetInDataNodes().at(0)->GetName(), "var1");

  EXPECT_TRUE(transdata2 == nullptr || transdata3 == nullptr);
  EXPECT_FALSE(transdata2 == nullptr && transdata3 == nullptr);
  auto not_empty_node = transdata2 != nullptr ? transdata2 : transdata3;
  EXPECT_FALSE(not_empty_node->GetInNodes().empty());
  EXPECT_EQ(not_empty_node->GetInDataNodes().at(0)->GetName(), "var1");
  EXPECT_FALSE(not_empty_node->GetOutNodes().empty());
  EXPECT_EQ(not_empty_node->GetOutDataNodes().at(0)->GetName(), "netoutput1");
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, diff_transpose) {
  auto graph = BuildGraph3();

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);

  auto transpose1 = graph->FindNode("transpose1");
  auto transpose2 = graph->FindNode("transpose2");

  EXPECT_FALSE(transpose1 == nullptr);
  EXPECT_FALSE(transpose2 == nullptr);
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, check_cycle_quit_fusion) {
  auto graph = BuildGraphMayCauseCycleWhenFusion();
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);
}

TEST_F(UtestGraphPassesTransOpBreadthFusionPass, remove_control_edge_between_transop_to_avoid_cycle) {
  auto graph = BuildGraphWithCtrlEdgeBetweenTransop();
  EXPECT_EQ(graph->GetDirectNodesSize(), 4);

  ge::TransOpBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(SUCCESS, status);
  EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  EXPECT_EQ(graph->TopologicalSorting(), SUCCESS);
}