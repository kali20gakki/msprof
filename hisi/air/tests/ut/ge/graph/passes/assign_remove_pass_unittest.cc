/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include <string>
#include <gtest/gtest.h>
#define private public
#include "inc/pass_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/passes/assign_remove_pass.h"
#undef private

using namespace domi;
namespace ge {
namespace {
class UtestGraphPassesAssignRemovePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
  void build_graph(bool value_const_flag = false, bool ref_var_flag = true, bool var_in_flag = false,
                   bool var_multi_flag = false, bool value_multi_flag = false) {
    // Tensor
    GeTensorDesc tensor_desc(GeShape({2, 2, 2, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);

    // assign_ref_in
    OpDescPtr ref_in_desc = nullptr;
    if (ref_var_flag) {
      ref_in_desc = std::make_shared<OpDesc>("assign_ref_in", VARIABLE);
    } else {
      ref_in_desc = std::make_shared<OpDesc>("assign_ref_in", "assign_ref_in");
    }
    ref_in_desc->AddInputDesc(tensor_desc);
    ref_in_desc->AddOutputDesc(tensor_desc);
    assign_ref_in_ = graph_->AddNode(ref_in_desc);

    // assign_var_in has data input
    if (var_in_flag) {
      auto data_desc = std::make_shared<OpDesc>("var_in", CONSTANTOP);
      data_desc->AddOutputDesc(tensor_desc);
      auto var_in_node = graph_->AddNode(data_desc);
      (void)GraphUtils::AddEdge(var_in_node->GetOutDataAnchor(0), assign_ref_in_->GetInDataAnchor(0));
    }

    // assign_value_in
    OpDescPtr value_in_desc = nullptr;
    if (value_const_flag) {
      value_in_desc = std::make_shared<OpDesc>("assign_value_in", CONSTANTOP);
    } else {
      value_in_desc = std::make_shared<OpDesc>("assign_value_in", "assign_value_in");
    }
    value_in_desc->AddOutputDesc(tensor_desc);
    assign_value_in_ = graph_->AddNode(value_in_desc);

    // assign_ctrl_in
    auto const_desc = std::make_shared<OpDesc>("assign_ctrl_in", CONSTANTOP);
    const_desc->AddOutputDesc(tensor_desc);
    assign_ctrl_in_ = graph_->AddNode(const_desc);

    // assign
    auto assign_desc = std::make_shared<OpDesc>("assign", ASSIGN);
    assign_desc->AddInputDesc(tensor_desc);
    assign_desc->AddInputDesc(tensor_desc);
    assign_desc->AddOutputDesc(tensor_desc);
    assign_node_ = graph_->AddNode(assign_desc);
    (void)GraphUtils::AddEdge(assign_ref_in_->GetOutDataAnchor(0), assign_node_->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(assign_value_in_->GetOutDataAnchor(0), assign_node_->GetInDataAnchor(1));
    (void)GraphUtils::AddEdge(assign_ctrl_in_->GetOutControlAnchor(), assign_node_->GetInControlAnchor());

    // assign_ref_out
    auto addn_desc = std::make_shared<OpDesc>("assign_ref_out", ADDN);
    addn_desc->AddInputDesc(tensor_desc);
    addn_desc->AddOutputDesc(tensor_desc);
    assign_ref_out_ = graph_->AddNode(addn_desc);
    (void)GraphUtils::AddEdge(assign_node_->GetOutDataAnchor(0), assign_ref_out_->GetInDataAnchor(0));

    // assign_ctrl_out
    auto noop_desc = std::make_shared<OpDesc>("assign_ctrl_out", NOOP);
    assign_ctrl_out_ = graph_->AddNode(noop_desc);
    (void)GraphUtils::AddEdge(assign_node_->GetOutControlAnchor(), assign_ctrl_out_->GetInControlAnchor());

    if (var_multi_flag) {
      auto addn_desc = std::make_shared<OpDesc>("addn", ADDN);
      addn_desc->AddInputDesc(tensor_desc);
      addn_desc->AddOutputDesc(tensor_desc);
      auto addn_node = graph_->AddNode(addn_desc);
      (void)GraphUtils::AddEdge(assign_ref_in_->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    }

    if (value_multi_flag) {
      auto addn_desc = std::make_shared<OpDesc>("addn", ADDN);
      addn_desc->AddInputDesc(tensor_desc);
      addn_desc->AddOutputDesc(tensor_desc);
      auto addn_node = graph_->AddNode(addn_desc);
      (void)GraphUtils::AddEdge(assign_value_in_->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    }
  }

  ComputeGraphPtr graph_;
  AssignRemovePass pass_;
  NodePtr assign_ref_in_;
  NodePtr assign_value_in_;
  NodePtr assign_ctrl_in_;
  NodePtr assign_node_;
  NodePtr assign_ref_out_;
  NodePtr assign_ctrl_out_;
};
} // namespace

TEST_F(UtestGraphPassesAssignRemovePass, run_success_match) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph();
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_TRUE(assign_node_->GetOutAllNodes().empty());
  EXPECT_TRUE(assign_node_->GetInAllNodes().empty());
  EXPECT_TRUE(assign_value_in_->GetOutDataAnchor(0)->IsLinkedWith(assign_ref_in_->GetInDataAnchor(0)));
  EXPECT_TRUE(assign_ref_in_->GetOutDataAnchor(0)->IsLinkedWith(assign_ref_out_->GetInDataAnchor(0)));
  EXPECT_TRUE(assign_value_in_->GetOutControlAnchor()->IsLinkedWith(assign_ctrl_out_->GetInControlAnchor()));
  EXPECT_TRUE(assign_ref_in_->GetOutControlAnchor()->IsLinkedWith(assign_ctrl_out_->GetInControlAnchor()));
  EXPECT_TRUE(assign_ctrl_in_->GetOutControlAnchor()->IsLinkedWith(assign_ctrl_out_->GetInControlAnchor()));
  EXPECT_TRUE(assign_ctrl_in_->GetOutControlAnchor()->IsLinkedWith(assign_ref_out_->GetInControlAnchor()));
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_not_assign) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  GeTensorDesc tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
  auto const_desc = std::make_shared<OpDesc>("const", CONSTANTOP);
  const_desc->AddOutputDesc(tensor_desc);
  auto const_node = graph_->AddNode(const_desc);

  EXPECT_EQ(pass_.Run(const_node), SUCCESS);
}

TEST_F(UtestGraphPassesAssignRemovePass, test_transform_attr_fail) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  GeTensorDesc tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
  auto const_desc = std::make_shared<OpDesc>("const", CONSTANTOP);
  const_desc->AddOutputDesc(tensor_desc);
  auto const_node = graph_->AddNode(const_desc);

  GeTensorDesc test_tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
  int32_t inplace_input_idx = 0;
  std::string assign_var_name = "var_name";
  AttrUtils::SetInt(test_tensor_desc, INPLACE_SUPPORT_INPUT_INDEX, inplace_input_idx);
  AttrUtils::SetStr(test_tensor_desc, ASSIGN_VAR_NAME, assign_var_name);
  auto test_op_desc = std::make_shared<OpDesc>("test", "Div");
  test_op_desc->AddInputDesc(test_tensor_desc);
  test_op_desc->AddOutputDesc(test_tensor_desc);
  auto test_node = graph_->AddNode(test_op_desc);

  EXPECT_EQ(GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), test_node->GetInDataAnchor(0)), SUCCESS);

  EXPECT_EQ(pass_.Run(test_node), SUCCESS);
}

TEST_F(UtestGraphPassesAssignRemovePass, test_no_input_fail) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  GeTensorDesc tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
  auto const_desc = std::make_shared<OpDesc>("const", CONSTANTOP);
  auto const_node = graph_->AddNode(const_desc);

  EXPECT_EQ(pass_.OptimizedAssignNode(const_node), FAILED);
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_mismatch_const) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph(true);
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_FALSE(assign_node_->GetOutAllNodes().empty());
  EXPECT_FALSE(assign_node_->GetInAllNodes().empty());
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_mismatch_not_var) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph(false, false);
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_FALSE(assign_node_->GetOutAllNodes().empty());
  EXPECT_FALSE(assign_node_->GetInAllNodes().empty());
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_mismatch_var_in) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph(false, true, true);
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_FALSE(assign_node_->GetOutAllNodes().empty());
  EXPECT_FALSE(assign_node_->GetInAllNodes().empty());
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_mismatch_var_multi_out) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph(false, true, false, true);
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_FALSE(assign_node_->GetOutAllNodes().empty());
  EXPECT_FALSE(assign_node_->GetInAllNodes().empty());
}

TEST_F(UtestGraphPassesAssignRemovePass, run_success_mismatch_value_multi_out) {
  graph_ = std::make_shared<ComputeGraph>("test_graph");
  build_graph(false, true, false, false, true);
  EXPECT_NE(assign_ref_in_, nullptr);
  EXPECT_NE(assign_value_in_, nullptr);
  EXPECT_NE(assign_ctrl_in_, nullptr);
  EXPECT_NE(assign_node_, nullptr);
  EXPECT_NE(assign_ref_out_, nullptr);
  EXPECT_NE(assign_ctrl_out_, nullptr);

  EXPECT_EQ(pass_.Run(assign_node_), SUCCESS);
  EXPECT_FALSE(assign_node_->GetOutAllNodes().empty());
  EXPECT_FALSE(assign_node_->GetInAllNodes().empty());
}
} // namespace ge
