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

#include <string>
#include <gtest/gtest.h>

#define private public
#include "graph/passes/enter_pass.h"
#include "common/ge_inner_error_codes.h"
#include "inc/pass_manager.h"
#include "utils/graph_utils.h"
#undef private
#include "ge_graph_dsl/graph_dsl.h"

namespace ge {
namespace {

class UtestGraphPassesEnterPass : public testing::Test {
 protected:
void BuildGraph(int32_t type) {
    // Tensor
    GeTensorDesc bool_tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_BOOL);
    GeTensorDesc scalar_tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);

    // const
    auto const_op_desc = std::make_shared<OpDesc>("a", "Constant");
    const_op_desc->AddOutputDesc(scalar_tensor_desc);
    auto const_node_ = graph_->AddNode(const_op_desc);

    // enter
    auto enter_op_desc = std::make_shared<OpDesc>("Enter", "Enter");
    enter_op_desc->AddInputDesc(scalar_tensor_desc);
    enter_op_desc->AddOutputDesc(scalar_tensor_desc);
    enter_node_ = graph_->AddNode(enter_op_desc);
    (void)GraphUtils::AddEdge(const_node_->GetOutDataAnchor(0), enter_node_->GetInDataAnchor(0));

    // less
    auto x_op_desc = std::make_shared<OpDesc>("x", VARIABLEV2);
    x_op_desc->AddOutputDesc(scalar_tensor_desc);
    auto x_node = graph_->AddNode(x_op_desc);
    auto y_op_desc = std::make_shared<OpDesc>("y", VARIABLEV2);
    y_op_desc->AddOutputDesc(scalar_tensor_desc);
    auto y_node = graph_->AddNode(y_op_desc);

    auto less_op_desc = std::make_shared<OpDesc>("Less", "Less");
    less_op_desc->AddInputDesc(scalar_tensor_desc);
    less_op_desc->AddInputDesc(scalar_tensor_desc);
    less_op_desc->AddOutputDesc(bool_tensor_desc);

    if (type == 1) {
      auto less_node = graph_->AddNode(less_op_desc);
      (void)GraphUtils::AddEdge(x_node->GetOutDataAnchor(0), less_node->GetInDataAnchor(0));
      (void)GraphUtils::AddEdge(y_node->GetOutDataAnchor(0), less_node->GetInDataAnchor(1));
      (void)GraphUtils::AddEdge(enter_node_->GetOutControlAnchor(), less_node->GetInControlAnchor());
    } else if (type == 2) {
      less_op_desc->AddInputDesc(scalar_tensor_desc);
      auto less_node = graph_->AddNode(less_op_desc);
      (void)GraphUtils::AddEdge(x_node->GetOutDataAnchor(0), less_node->GetInDataAnchor(0));
      (void)GraphUtils::AddEdge(y_node->GetOutDataAnchor(0), less_node->GetInDataAnchor(1));
      (void)GraphUtils::AddEdge(enter_node_->GetOutDataAnchor(0), less_node->GetInDataAnchor(2));
    } else if (type == 3) {
      auto merge_desc = std::make_shared<OpDesc>("Merge", MERGE);
      merge_desc->AddInputDesc(scalar_tensor_desc);
      merge_desc->AddOutputDesc(bool_tensor_desc);
      auto merge_node = graph_->AddNode(merge_desc);
      (void)GraphUtils::AddEdge(enter_node_->GetOutDataAnchor(0), merge_node->GetInDataAnchor(0));
    }

  }

  ComputeGraphPtr graph_;
  EnterPass pass_;
  NodePtr enter_node_;
};
}  // namespace

TEST_F(UtestGraphPassesEnterPass, null_input) {
  NodePtr node = nullptr;
  EXPECT_EQ(pass_.Run(node), PARAM_INVALID);
}

TEST_F(UtestGraphPassesEnterPass, run_success) {
  graph_ = std::make_shared<ComputeGraph>("UTEST_graph_passes_enter_pass_run_success");
  BuildGraph(1);
  EXPECT_NE(enter_node_, nullptr);

  EXPECT_EQ(pass_.Run(enter_node_), SUCCESS);
  EXPECT_EQ(enter_node_->GetOutControlAnchor()->GetPeerAnchors().empty(), true);
}

TEST_F(UtestGraphPassesEnterPass, OptimizeEnter_success) {
  graph_ = std::make_shared<ComputeGraph>("UTEST_graph_passes_enter_pass_OptimizeEnter_success");
  BuildGraph(2);
  EXPECT_NE(enter_node_, nullptr);

  EXPECT_EQ(pass_.Run(enter_node_), SUCCESS);
  auto all_nodes = graph_->GetAllNodes();
  EXPECT_EQ(std::find(all_nodes.begin(), all_nodes.end(), enter_node_), all_nodes.end());
}

TEST_F(UtestGraphPassesEnterPass, test_in_data_node_empty) {
  const auto compute_graph = std::make_shared<ComputeGraph>("test_in_data_node_empty");
  const auto enter_op_desc = std::make_shared<OpDesc>("Enter", "Enter");
  GeTensorDesc desc;
  enter_op_desc->AddInputDesc(desc);
  enter_op_desc->AddOutputDesc(desc);
  auto enter_node = compute_graph->AddNode(enter_op_desc);
  EXPECT_NE(enter_node, nullptr);

  EnterPass pass;
  EXPECT_EQ(pass.Run(enter_node), PARAM_INVALID);
}

TEST_F(UtestGraphPassesEnterPass, test_in_data_node_is_not_const) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", VARIABLE)->EDGE(0, 0)->NODE("Enter", ENTER));
  };
  const auto compute_graph = ToComputeGraph(g1);
  auto enter_node = compute_graph->FindNode("Enter");
  EXPECT_NE(enter_node, nullptr);
  EnterPass pass;
  EXPECT_EQ(pass.Run(enter_node), SUCCESS);
}

TEST_F(UtestGraphPassesEnterPass, test_need_remove_flag_false) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("Const", CONSTANT)->EDGE(0, 0)->NODE("Enter", ENTER));
    CTRL_CHAIN(NODE("Const1", CONSTANT)->NODE("Enter", ENTER));
  };
  const auto compute_graph = ToComputeGraph(g1);
  auto enter_node = compute_graph->FindNode("Enter");
  EXPECT_NE(enter_node, nullptr);
  EnterPass pass;
  EXPECT_EQ(pass.Run(enter_node), SUCCESS);
}

TEST_F(UtestGraphPassesEnterPass, test_unlink_ctrl_edge_before_cosnt) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("Const", CONSTANT)->EDGE(0, 0)->NODE("Enter", ENTER));
    CHAIN(NODE("Enter", ENTER)->EDGE(0, 0)->NODE("add1", ADD));
    CHAIN(NODE("Const1", CONSTANT)->EDGE(0, 1)->NODE("add1", ADD));

    CHAIN(NODE("Const2", CONSTANT)->EDGE(0, 0)->NODE("add2", ADD));
    CHAIN(NODE("add1", ADD)->EDGE(0, 1)->NODE("add2", ADD));

    CTRL_CHAIN(NODE("Enter", ENTER)->NODE("Const1", CONSTANT));
    CTRL_CHAIN(NODE("Enter", ENTER)->NODE("add2", ADD));
  };
  const auto compute_graph = ToComputeGraph(g1);
  auto enter_node = compute_graph->FindNode("Enter");
  EXPECT_NE(enter_node, nullptr);
  EnterPass pass;
  EXPECT_EQ(pass.Run(enter_node), SUCCESS);
  const auto add1 = compute_graph->FindNode("add1");
  ASSERT_NE(add1, nullptr);
  EXPECT_EQ(add1->GetInControlNodes().size(), 1);
  const auto enter_node_after_pass = compute_graph->FindNode("Enter");
  EXPECT_NE(enter_node_after_pass, nullptr);
}

}  // namespace ge
