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

#include <cstdint>
#include <string>
#include <gtest/gtest.h>

#include "common/ge_inner_error_codes.h"
#define protected public
#define private public
#include "graph/passes/hccl_memcpy_pass.h"
#include "inc/pass_manager.h"
#undef protected
#undef private
#include "graph_builder_utils.h"

namespace ge {
class UtestGraphPassesHcclMemcpyPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}

  void make_graph(ComputeGraphPtr graph, bool match = true) {
    GeTensorDesc scalarTensorDesc(GeShape({7, 7, 3, 1}));

    auto input_op = std::make_shared<OpDesc>("input", DATA);
    input_op->AddOutputDesc(scalarTensorDesc);
    auto input_node = graph->AddNode(input_op);

    auto allreduce_op = std::make_shared<OpDesc>("allreduce_op", HCOMALLREDUCE);
    (void)ge::AttrUtils::SetBool(allreduce_op, "_input_mutable", true);
    allreduce_op->AddInputDesc(scalarTensorDesc);
    allreduce_op->AddOutputDesc(scalarTensorDesc);
    auto allreduce_node = graph->AddNode(allreduce_op);

    auto relu_op = std::make_shared<OpDesc>("relu_op", RELU);
    relu_op->AddInputDesc(scalarTensorDesc);
    relu_op->AddOutputDesc(scalarTensorDesc);
    auto relu_node = graph->AddNode(relu_op);

    auto add_op = std::make_shared<OpDesc>("add_op", ADD);
    add_op->AddInputDesc(scalarTensorDesc);
    add_op->AddInputDesc(scalarTensorDesc);
    add_op->AddOutputDesc(scalarTensorDesc);
    auto add_node = graph->AddNode(add_op);

    (void) GraphUtils::AddEdge(input_node->GetOutDataAnchor(0), allreduce_node->GetInDataAnchor(0));
    (void) GraphUtils::AddEdge(input_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));

    (void) GraphUtils::AddEdge(allreduce_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
    (void) GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(1));
  }


  void make_sub_graph(ComputeGraphPtr graph, bool match = true) {
    GeTensorDesc scalarTensorDesc(GeShape({7, 7, 3, 1}));

    auto input_op = std::make_shared<OpDesc>("input", DATA);
    input_op->AddOutputDesc(scalarTensorDesc);
    auto input_node = graph->AddNode(input_op);

    auto allreduce_op = std::make_shared<OpDesc>("allreduce_op", HCOMALLREDUCE);
    (void)ge::AttrUtils::SetBool(allreduce_op, "_input_mutable", true);
    allreduce_op->AddInputDesc(scalarTensorDesc);
    allreduce_op->AddOutputDesc(scalarTensorDesc);
    auto allreduce_node = graph->AddNode(allreduce_op);

    auto relu_op = std::make_shared<OpDesc>("relu_op", RELU);
    relu_op->AddInputDesc(scalarTensorDesc);
    relu_op->AddOutputDesc(scalarTensorDesc);
    auto relu_node = graph->AddNode(relu_op);

    (void) GraphUtils::AddEdge(input_node->GetOutDataAnchor(0), allreduce_node->GetInDataAnchor(0));
    (void) GraphUtils::AddEdge(allreduce_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  }
};
namespace {

/*
 *         var                               var
 *         |  \                             |   \
 *         |   assign                       |   assign
 *         |   //         =======>          |   //
 *     allreduce                         identity
 *        |                                 |
 *       netoutput                        allreduce
 *                                          |
 *                                        netoutput
 */
ComputeGraphPtr BuildGraph_Allreduce_Read_Var_After_Assign(){
  auto builder = ut::GraphBuilder("test");
  auto var = builder.AddNode("var", VARIABLE, 0, 1);
  auto assign = builder.AddNode("assign", ASSIGN, 1, 1);
  auto allreduce = builder.AddNode("allreduce", HCOMALLREDUCE, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(var, 0, assign, 0);
  builder.AddDataEdge(var,0,allreduce,0);
  builder.AddControlEdge(assign, allreduce);
  return builder.GetGraph();
}
}  // namespace

// const -> allreduce
// const -> Identity -> allreduce
TEST_F(UtestGraphPassesHcclMemcpyPass, testInsertIdentityBeforeHccl) {
  ComputeGraphPtr graph = BuildGraph_Allreduce_Read_Var_After_Assign();
  auto src_node = graph->FindNode("var");
  auto dst_node = graph->FindNode("allreduce");
  // test InsertIdentityBeforeHccl
  HcclMemcpyPass hccl_memcpy_pass;
  hccl_memcpy_pass.InsertIdentityBeforeHccl(graph, src_node->GetOutDataAnchor(0),
                                            dst_node->GetInDataAnchor(0));

  // check
  dst_node = graph->FindNode("allreduce");
  auto in_node_before_dst_node = dst_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  EXPECT_EQ(in_node_before_dst_node->GetType(), IDENTITY);
  EXPECT_EQ(in_node_before_dst_node->GetInControlNodes().size(), 1);
  EXPECT_EQ(in_node_before_dst_node->GetInControlNodes().at(0)->GetName(), "assign");
}

TEST_F(UtestGraphPassesHcclMemcpyPass, run_empty_graph_failed)
{
  ComputeGraphPtr graph = nullptr;

  HcclMemcpyPass hcclMemcpyPass;
  std::vector<std::pair<string, GraphPass*>> passes;
  passes.emplace_back("", &hcclMemcpyPass);
  EXPECT_EQ(hcclMemcpyPass.Run(graph), ge::PARAM_INVALID);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, run_success)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_graph(graph);

  HcclMemcpyPass hcclMemcpyPass;
  std::vector<std::pair<string, GraphPass*>> passes;
  passes.emplace_back("", &hcclMemcpyPass);
  EXPECT_EQ(hcclMemcpyPass.Run(graph), ge::SUCCESS);

  bool has_memcpy_async = false;
  std::string node_type;
  for (ge::NodePtr node : graph->GetDirectNode()) {
    OpDescPtr tmp_desc = node->GetOpDesc();
    node_type = tmp_desc->GetType();
    if (node_type == IDENTITY) {
      has_memcpy_async = true;
      break;
    }
  }

  EXPECT_EQ(has_memcpy_async, true);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, run_sub_graph_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_sub_graph(graph);

  HcclMemcpyPass hcclMemcpyPass;
  std::vector<std::pair<string, GraphPass*>> passes;
  passes.emplace_back("", &hcclMemcpyPass);
  EXPECT_EQ(hcclMemcpyPass.Run(graph), ge::SUCCESS);

  bool has_memcpy_async = false;
  std::string node_type;
  for (ge::NodePtr node : graph->GetDirectNode()) {
    OpDescPtr tmp_desc = node->GetOpDesc();
    node_type = tmp_desc->GetType();
    if (node_type == IDENTITY) {
      has_memcpy_async = true;
      break;
    }
  }

  EXPECT_EQ(has_memcpy_async, true);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, create_memcpy_node_success)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_graph(graph);
  HcclMemcpyPass hcclMemcpyPass;
  std::vector<std::pair<string, GraphPass*>> passes;
  passes.emplace_back("", &hcclMemcpyPass);
  std::shared_ptr<ge::Node> null_Node;

  EXPECT_EQ(hcclMemcpyPass.Run(graph), SUCCESS);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, ClearStatus)
{
  HcclMemcpyPass hcclMemcpyPass;
  Status ret = hcclMemcpyPass.ClearStatus();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, Run_test)
{
  HcclMemcpyPass hcclMemcpyPass;
  GeTensorDesc scalarTensorDesc(GeShape({7, 7, 3, 1}));
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  auto hcomBroadcast_op = std::make_shared<OpDesc>("HcomBroadcast", HCOMBROADCAST);
  (void)ge::AttrUtils::SetBool(hcomBroadcast_op, "_input_mutable", true);
  hcomBroadcast_op->AddInputDesc(scalarTensorDesc);
  hcomBroadcast_op->AddInputDesc(scalarTensorDesc);
  hcomBroadcast_op->AddOutputDesc(scalarTensorDesc);
  auto hcomBroadcast_node = graph->AddNode(hcomBroadcast_op);

  Status ret = hcclMemcpyPass.Run(graph);
  EXPECT_EQ(ret, PARAM_INVALID);

  auto input_op = std::make_shared<OpDesc>("input", DATA);
  input_op->AddOutputDesc(scalarTensorDesc);
  auto input_node = graph->AddNode(input_op);

  auto add_op = std::make_shared<OpDesc>("add_op", ADD);
  add_op->AddInputDesc(scalarTensorDesc);
  add_op->AddOutputDesc(scalarTensorDesc);
  auto add_node = graph->AddNode(add_op);
  (void) GraphUtils::AddEdge(input_node->GetOutDataAnchor(0), hcomBroadcast_node->GetInDataAnchor(0));
  (void) GraphUtils::AddEdge(hcomBroadcast_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
  ret = hcclMemcpyPass.Run(graph);
  EXPECT_EQ(ret, PARAM_INVALID);

  auto variable_op = std::make_shared<OpDesc>("Variable", VARIABLE);
  variable_op->AddOutputDesc(scalarTensorDesc);
  variable_op->AddInputDesc(scalarTensorDesc);
  auto variable_node = graph->AddNode(variable_op);

  (void) GraphUtils::AddEdge(variable_node->GetOutDataAnchor(0), hcomBroadcast_node->GetInDataAnchor(1));
  ret = hcclMemcpyPass.Run(graph);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphPassesHcclMemcpyPass, Insert_assign_test)
{
  HcclMemcpyPass hcclMemcpyPass;
  GeTensorDesc scalarTensorDesc(GeShape({7, 7, 3, 1}));
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  auto hcomBroadcast_op = std::make_shared<OpDesc>("HcomBroadcast", HCOMBROADCAST);
  (void)ge::AttrUtils::SetBool(hcomBroadcast_op, "_input_mutable", true);
  hcomBroadcast_op->AddInputDesc(scalarTensorDesc);
  hcomBroadcast_op->AddInputDesc(scalarTensorDesc);
  hcomBroadcast_op->AddOutputDesc(scalarTensorDesc);
  auto hcomBroadcast_node = graph->AddNode(hcomBroadcast_op);

  auto variable_op = std::make_shared<OpDesc>("Variable", VARIABLE);
  variable_op->AddOutputDesc(scalarTensorDesc);
  variable_op->AddInputDesc(scalarTensorDesc);
  auto variable_node = graph->AddNode(variable_op);

  (void) GraphUtils::AddEdge(variable_node->GetOutDataAnchor(0), hcomBroadcast_node->GetInDataAnchor(0));
  auto ret = hcclMemcpyPass.InsertAssignAfterBroadcastIfNeed(graph, variable_node->GetOutDataAnchor(0), hcomBroadcast_node->GetInDataAnchor(0));
  EXPECT_EQ(ret, SUCCESS);
}
}  // namespace ge
