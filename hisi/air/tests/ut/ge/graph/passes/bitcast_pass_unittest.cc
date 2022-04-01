/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include "graph/passes/bitcast_pass.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph_builder_utils.h"

using namespace std;
using namespace ge;

class UtestBitCastPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
  void BuildGraph() {
    GeTensorDesc td(GeShape({2,2,2,2}), ge::FORMAT_NCHW, ge::DT_INT16);

    auto pre_op_desc = std::make_shared<OpDesc>("pre", "Constant");
    pre_op_desc->AddOutputDesc(td);
    pre_node_ = graph_->AddNode(pre_op_desc);

    auto bitcast_op_desc = std::make_shared<OpDesc>("bitcat", "Bitcast");
    bitcast_op_desc->AddInputDesc(td);
    bitcast_op_desc->AddOutputDesc(td);
    bitcast_node_ = graph_->AddNode(bitcast_op_desc);

    auto peer_op_desc = std::make_shared<OpDesc>("relu", RELU);
    peer_op_desc->AddInputDesc(td);
    peer_op_desc->AddOutputDesc(td);
    peer_node_ = graph_->AddNode(peer_op_desc);

    GraphUtils::AddEdge(pre_node_->GetOutDataAnchor(0), bitcast_node_->GetInDataAnchor(0));
    GraphUtils::AddEdge(bitcast_node_->GetOutDataAnchor(0), peer_node_->GetInDataAnchor(0));
  }

  ComputeGraphPtr graph_;
  BitcastPass pass_;
  NodePtr bitcast_node_;
  NodePtr pre_node_;
  NodePtr peer_node_;
};

TEST_F(UtestBitCastPass, null_node_fail) {
  auto ret = pass_.Run(bitcast_node_);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestBitCastPass, no_attr_fail) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, dtype_not_match_fail) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT32);

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, keep_dtype_succ) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT16);

  EXPECT_EQ(pass_.Run(bitcast_node_), SUCCESS);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 2);
}

TEST_F(UtestBitCastPass, dst_dtype_invalid_fail) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_UNDEFINED);

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, ori_dtype_invalid_fail) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT16);
  bitcast_opdesc->MutableInputDesc(0)->SetDataType(ge::DT_UNDEFINED);

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, compress_dim_succ) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT32);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_INT32);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), SUCCESS);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 2);
}

TEST_F(UtestBitCastPass, compress_dim_fail_1) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_DUAL);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_DUAL);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, compress_dim_fail_2) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT64);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_INT64);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, compress_dim_fail_3) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT32);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_INT32);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, extend_dim_succ) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT8);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_INT8);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), SUCCESS);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 2);
}

TEST_F(UtestBitCastPass, extend_dim_fail_1) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT8);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_INT8);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, extend_dim_fail_2) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_INT16);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_DUAL);

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}

TEST_F(UtestBitCastPass, extend_dim_fail_3) {
  graph_ = std::make_shared<ComputeGraph>("g");
  BuildGraph();
  EXPECT_NE(bitcast_node_, nullptr);
  EXPECT_NE(pre_node_, nullptr);
  EXPECT_NE(peer_node_, nullptr);

  auto bitcast_opdesc = bitcast_node_->GetOpDesc();
  ge::AttrUtils::SetDataType(bitcast_opdesc, "type", ge::DT_COMPLEX64);
  bitcast_opdesc->MutableOutputDesc(0)->SetDataType(ge::DT_DUAL);
  bitcast_opdesc->MutableOutputDesc(0)->SetShape(GeShape({2,2,2,2,2}));

  EXPECT_EQ(pass_.Run(bitcast_node_), PARAM_INVALID);
  EXPECT_EQ(graph_->GetDirectNodesSize(), 3);
}