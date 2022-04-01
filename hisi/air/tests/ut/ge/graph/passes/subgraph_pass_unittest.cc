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

#include <cstdint>
#include <string>
#include <gtest/gtest.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/subgraph_pass.h"
#include "inc/pass_manager.h"

namespace ge {
namespace {
class UtestGraphPassesSubgraphPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

OpDescPtr CreateOpDesc(const std::string name, const std::string type, uint32_t input_num, uint32_t output_num) {
  OpDescPtr op_desc = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc(name, type));
  if (op_desc == nullptr) {
    return nullptr;
  }
  for (uint32_t i = 0; i < input_num; i++) {
    op_desc->AddInputDesc(GeTensorDesc());
  }
  for (uint32_t i = 0; i < output_num; i++) {
    op_desc->AddOutputDesc(GeTensorDesc());
  }
  return op_desc;
}

bool CheckMemcpyExist(const ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == IDENTITY) {
      return true;
    }
  }
  return false;
}

uint32_t CheckMemcpyNum(const ComputeGraphPtr &graph) {
  uint32_t num = 0;
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == IDENTITY) {
      num++;
    }
  }
  return num;
}
} // namespace

TEST_F(UtestGraphPassesSubgraphPass, add_memcpy_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr function_node = graph->AddNode(CreateOpDesc("Case", CASE, 2, 1));
  NodePtr data_node_0 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr data_node_1 = graph->AddNode(CreateOpDesc("data1", DATA, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_0->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name1 = "instance_branch1";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("branch1");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  std::string subgraph_name2 = "instance_branch2";
  ComputeGraphPtr subgraph2 = std::make_shared<ComputeGraph>(subgraph_name2);
  subgraph2->SetParentNode(function_node);
  subgraph2->SetParentGraph(graph);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("branch2");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name2);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);

  {
    // Const->NetOutput in subgraph
    NodePtr const_node = subgraph1->AddNode(CreateOpDesc("const", CONSTANTOP, 0, 1));
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  {
    // Data->NetOutput in subgraph but not while body
    NodePtr data_node = subgraph2->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph2->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
  EXPECT_EQ(pass_managers.Run(subgraph1), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph1));
  EXPECT_EQ(pass_managers.Run(subgraph2), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph2));

  {
    // Input->FunctionOp and Input link to other nodes
    NodePtr identity_node = graph->AddNode(CreateOpDesc("Identity", RELU, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node_0->GetOutDataAnchor(0), identity_node->GetInDataAnchor(0)), SUCCESS);
  }
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));

  {
    // Data->NetOutput in root_graph
    NodePtr output_node = graph->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));

  {
    // Const->FunctionOp
    EXPECT_EQ(GraphUtils::RemoveEdge(data_node_1->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);
    NodePtr const_node = graph->AddNode(CreateOpDesc("const", CONSTANTOP, 0, 1));
    EXPECT_EQ(GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);
  }
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
}

TEST_F(UtestGraphPassesSubgraphPass, while_subgraph_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr function_node = graph->AddNode(CreateOpDesc("While", WHILE, 2, 2));

  std::string subgraph_name1 = "instance_cond";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("cond");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  graph->AddSubgraph(subgraph1);

  std::string subgraph_name2 = "instance_body";
  ComputeGraphPtr subgraph2 = std::make_shared<ComputeGraph>(subgraph_name2);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name2);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  subgraph2->SetParentNode(function_node);
  subgraph2->SetParentGraph(graph);
  graph->AddSubgraph(subgraph2);

  // Input->While
  NodePtr data_node_0 = graph->AddNode(CreateOpDesc("data_0", DATA, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_0->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);

  NodePtr data_node_1 = graph->AddNode(CreateOpDesc("data_1", DATA, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  {
    // Data->FunctionOp in while cond subgraph
    NodePtr data_node = subgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  {
    // Data->NetOutput in while body subgraph
    NodePtr data_node = subgraph2->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph2->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
  EXPECT_EQ(pass_managers.Run(subgraph1), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(subgraph1));
  EXPECT_EQ(pass_managers.Run(subgraph2), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(subgraph2));

  {
    // Input->While and Input link to other nodes
    NodePtr identity_node = graph->AddNode(CreateOpDesc("Identity", RELU, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), identity_node->GetInDataAnchor(0)), SUCCESS);
  }
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(graph));
}

TEST_F(UtestGraphPassesSubgraphPass, constant_case_subgraph_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr function_node = graph->AddNode(CreateOpDesc("Case", CASE, 2, 1));
  NodePtr data_node_0 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr data_node_1 = graph->AddNode(CreateOpDesc("data1", CONSTANTOP, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_0->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name1 = "instance_branch1";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("branch1");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  std::string subgraph_name2 = "instance_branch2";
  ComputeGraphPtr subgraph2 = std::make_shared<ComputeGraph>(subgraph_name2);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("branch2");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name2);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);

  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  graph->AddSubgraph(subgraph1);
  subgraph2->SetParentNode(function_node);
  subgraph2->SetParentGraph(graph);
  graph->AddSubgraph(subgraph2);

  {
    // Const->FunctionOp in subgraph
    NodePtr data_node = subgraph1->AddNode(CreateOpDesc("data", DATA, 0, 1));
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  {
    // Data->FunctionOp in subgraph but not while body
    NodePtr data_node = subgraph2->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph2->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
}

TEST_F(UtestGraphPassesSubgraphPass, constant_while_subgraph_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr function_node = graph->AddNode(CreateOpDesc("While", WHILE, 2, 1));

  // Input->While
  NodePtr data_node_0 = graph->AddNode(CreateOpDesc("data_0", DATA, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_0->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  NodePtr data_node_1 = graph->AddNode(CreateOpDesc("data_1", CONSTANTOP, 1, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node_1->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name1 = "instance_cond";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("cond");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  std::string subgraph_name2 = "instance_body";
  ComputeGraphPtr subgraph2 = std::make_shared<ComputeGraph>(subgraph_name2);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name2);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);

  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  graph->AddSubgraph(subgraph1);
  subgraph2->SetParentNode(function_node);
  subgraph2->SetParentGraph(graph);
  graph->AddSubgraph(subgraph2);

  {
    // Constant->FunctionOp in while cond subgraph
    NodePtr data_node = subgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  {
    // Constant->FunctionOp in while body subgraph
    NodePtr data_node = subgraph2->AddNode(CreateOpDesc("data", DATA, 1, 1));
    NodePtr output_node = subgraph2->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(graph));
}

TEST_F(UtestGraphPassesSubgraphPass, reassign_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr data_node = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr function_node = graph->AddNode(CreateOpDesc("Case", CASE, 2, 1));
  EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name1 = "instance_branch1";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("branch1");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  std::string subgraph_name2 = "instance_branch2";
  ComputeGraphPtr subgraph2 = std::make_shared<ComputeGraph>(subgraph_name2);
  subgraph2->SetParentNode(function_node);
  subgraph2->SetParentGraph(graph);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("branch2");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name2);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);

  std::string subgraph_name3 = "instance_branch3";
  ComputeGraphPtr subgraph3 = std::make_shared<ComputeGraph>(subgraph_name3);
  subgraph3->SetParentNode(function_node);
  subgraph3->SetParentGraph(graph);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 2);
  function_node->GetOpDesc()->AddSubgraphName("branch3");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 3);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name3);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 3);

  std::string subgraph_name4 = "instance_branch4";
  ComputeGraphPtr subgraph4 = std::make_shared<ComputeGraph>(subgraph_name4);
  subgraph4->SetParentNode(function_node);
  subgraph4->SetParentGraph(graph);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 3);
  function_node->GetOpDesc()->AddSubgraphName("branch4");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 4);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name4);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 4);

  std::string subgraph_name5 = "instance_branch5";
  ComputeGraphPtr subgraph5 = std::make_shared<ComputeGraph>(subgraph_name5);
  subgraph5->SetParentNode(function_node);
  subgraph5->SetParentGraph(graph);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 4);
  function_node->GetOpDesc()->AddSubgraphName("branch5");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 5);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name5);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 5);

  {
    // AtomicOp->NetOutput in subgraph
    OpDescPtr atomic_desc = CreateOpDesc("Atomic", "Atomic", 1, 1);
    AttrUtils::SetBool(atomic_desc, ATOMIC_ATTR_IS_ATOMIC_NODE, true);
    AttrUtils::SetListInt(atomic_desc, ATOMIC_ATTR_OUTPUT_INDEX, { 0, 1 });
    NodePtr atomic_node = subgraph1->AddNode(atomic_desc);
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(atomic_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  {
    // OutputContinuesRequiredOp->NetOutput in subgraph
    OpDescPtr continues_desc = CreateOpDesc("OutputContinues", "OutputContinues", 1, 1);
    AttrUtils::SetBool(continues_desc, ATTR_NAME_CONTINUOUS_OUTPUT, true);
    NodePtr continues_node = subgraph2->AddNode(continues_desc);
    NodePtr output_node = subgraph2->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(continues_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  {
    // NoPaddingOutputContinuesRequiredOp->NetOutput in subgraph
    OpDescPtr nopadding_continues_desc = CreateOpDesc("OutputContinues", "OutputContinues", 1, 1);
    AttrUtils::SetBool(nopadding_continues_desc, ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT, true);
    NodePtr nopadding_continues_node = subgraph3->AddNode(nopadding_continues_desc);
    NodePtr output_node = subgraph3->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(nopadding_continues_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  {
    // DataInputContinuesRequiredOp->NetOutput in subgraph
    OpDescPtr data_desc = CreateOpDesc("data", DATA, 1, 1);
    AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 1);
    NodePtr data_node = subgraph4->AddNode(data_desc);
    OpDescPtr continues_desc = CreateOpDesc("InputContinues", "InputContinues", 1, 1);
    AttrUtils::SetBool(continues_desc, ATTR_NAME_CONTINUOUS_INPUT, true);
    NodePtr continues_node = subgraph4->AddNode(continues_desc);
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), continues_node->GetInDataAnchor(0)), SUCCESS);
    NodePtr output_node = subgraph4->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(continues_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  {
    // DataInputContinuesRequiredOp->NetOutput in subgraph
    OpDescPtr data_desc = CreateOpDesc("data", DATA, 1, 1);
    AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 1);
    NodePtr data_node = subgraph5->AddNode(data_desc);
    OpDescPtr nopadding_continues_desc = CreateOpDesc("InputContinues", "InputContinues", 1, 1);
    AttrUtils::SetBool(nopadding_continues_desc, ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
    NodePtr nopadding_continues_node = subgraph5->AddNode(nopadding_continues_desc);
    EXPECT_EQ(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), nopadding_continues_node->GetInDataAnchor(0)), SUCCESS);
    NodePtr output_node = subgraph5->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 1, 1));
    EXPECT_EQ(GraphUtils::AddEdge(nopadding_continues_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
  EXPECT_EQ(pass_managers.Run(subgraph1), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph1));
  EXPECT_EQ(pass_managers.Run(subgraph2), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph2));
  EXPECT_EQ(pass_managers.Run(subgraph3), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph3));
  EXPECT_EQ(pass_managers.Run(subgraph4), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph4));
  EXPECT_EQ(pass_managers.Run(subgraph5), SUCCESS);
  EXPECT_TRUE(CheckMemcpyExist(subgraph5));
}

TEST_F(UtestGraphPassesSubgraphPass, while_body_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr data_node1 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr data_node2 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr function_node = graph->AddNode(CreateOpDesc("While", WHILE, 2, 2));
  EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name0 = "instance_cond";
  ComputeGraphPtr subgraph0 = std::make_shared<ComputeGraph>(subgraph_name0);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("cond");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name0);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  subgraph0->SetParentNode(function_node);
  subgraph0->SetParentGraph(graph);
  graph->AddSubgraph(subgraph0);

  std::string subgraph_name1 = "instance_body";
  ComputeGraphPtr subgraph1 = std::make_shared<ComputeGraph>(subgraph_name1);
  index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  function_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name1);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  subgraph1->SetParentNode(function_node);
  subgraph1->SetParentGraph(graph);
  graph->AddSubgraph(subgraph1);

  NodePtr while_node = subgraph1->AddNode(CreateOpDesc("While", WHILE, 2, 2));
  {
    // index-exchange in while body subgraph
    NodePtr data_node1 = subgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
    NodePtr data_node2 = subgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node2->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    NodePtr output_node = subgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 2, 2));
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(1), ATTR_NAME_PARENT_NODE_INDEX, 1);
    EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), while_node->GetInDataAnchor(0)), SUCCESS);
    EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), while_node->GetInDataAnchor(1)), SUCCESS);
    EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    EXPECT_EQ(GraphUtils::AddEdge(while_node->GetOutDataAnchor(1), output_node->GetInDataAnchor(1)), SUCCESS);
  }

  std::string subsubgraph_name0 = "instance_sub_cond";
  ComputeGraphPtr subsubgraph0 = std::make_shared<ComputeGraph>(subsubgraph_name0);
  index = while_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  while_node->GetOpDesc()->AddSubgraphName("cond");
  EXPECT_EQ(while_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  while_node->GetOpDesc()->SetSubgraphInstanceName(index, subsubgraph_name0);
  EXPECT_EQ(while_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  subsubgraph0->SetParentNode(while_node);
  subsubgraph0->SetParentGraph(subgraph1);
  graph->AddSubgraph(subsubgraph0);

  std::string subsubgraph_name1 = "instance_sub_body";
  ComputeGraphPtr subsubgraph1 = std::make_shared<ComputeGraph>(subsubgraph_name1);
  index = while_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 1);
  while_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(while_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  while_node->GetOpDesc()->SetSubgraphInstanceName(index, subsubgraph_name1);
  EXPECT_EQ(while_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 2);
  subsubgraph1->SetParentNode(while_node);
  subsubgraph1->SetParentGraph(subgraph1);
  graph->AddSubgraph(subsubgraph1);

  {
    // index-exchange in while body subgraph
    NodePtr data_node1 = subsubgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
    NodePtr data_node2 = subsubgraph1->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node2->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    NodePtr output_node = subsubgraph1->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 2, 2));
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(1), ATTR_NAME_PARENT_NODE_INDEX, 1);
    EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), output_node->GetInDataAnchor(1)), SUCCESS);
    EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  EXPECT_FALSE(CheckMemcpyExist(graph));
  EXPECT_EQ(CheckMemcpyNum(subgraph1), 3); // 1 for while input link to other nodes, 1 for data exchange in while-body
  EXPECT_EQ(CheckMemcpyNum(subsubgraph1), 2);

  {
    auto output_node = subsubgraph1->FindNode(NODE_NAME_NET_OUTPUT);
    output_node->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape({-1,-1}));
    output_node->GetOpDesc()->MutableInputDesc(1)->SetShape(GeShape({-1,-1}));
    PassManager pass_managers;
    pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
    EXPECT_EQ(pass_managers.Run(graph), SUCCESS);
  }
}

TEST_F(UtestGraphPassesSubgraphPass, while_body_no_body_subgraph) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr data_node1 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr data_node2 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr function_node = graph->AddNode(CreateOpDesc("While", WHILE, 2, 2));
  EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name = "instance_body";
  ComputeGraphPtr subgraph = std::make_shared<ComputeGraph>(subgraph_name);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  subgraph->SetParentNode(function_node);
  subgraph->SetParentGraph(graph);

  {
    // index-exchange in while body subgraph
    NodePtr data_node1 = subgraph->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
    NodePtr data_node2 = subgraph->AddNode(CreateOpDesc("data", DATA, 1, 1));
    AttrUtils::SetInt(data_node2->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
    NodePtr output_node = subgraph->AddNode(CreateOpDesc(NODE_NAME_NET_OUTPUT, NETOUTPUT, 2, 2));
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);
    AttrUtils::SetInt(output_node->GetOpDesc()->MutableInputDesc(1), ATTR_NAME_PARENT_NODE_INDEX, 1);
    EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), output_node->GetInDataAnchor(0)), SUCCESS);
    EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), output_node->GetInDataAnchor(1)), SUCCESS);
  }

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_NE(pass_managers.Run(graph), SUCCESS);
}

TEST_F(UtestGraphPassesSubgraphPass, while_body_no_net_output) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  NodePtr data_node1 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr data_node2 = graph->AddNode(CreateOpDesc("data", DATA, 1, 1));
  NodePtr function_node = graph->AddNode(CreateOpDesc("While", WHILE, 2, 2));
  EXPECT_EQ(GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), function_node->GetInDataAnchor(0)), SUCCESS);
  EXPECT_EQ(GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), function_node->GetInDataAnchor(1)), SUCCESS);

  std::string subgraph_name = "instance_body";
  ComputeGraphPtr subgraph = std::make_shared<ComputeGraph>(subgraph_name);
  size_t index = function_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  EXPECT_EQ(index, 0);
  function_node->GetOpDesc()->AddSubgraphName("body");
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);
  function_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph_name);
  EXPECT_EQ(function_node->GetOpDesc()->GetSubgraphInstanceNames().size(), 1);

  subgraph->SetParentNode(function_node);
  subgraph->SetParentGraph(graph);
  graph->AddSubgraph(subgraph);

  PassManager pass_managers;
  pass_managers.AddPass("", new (std::nothrow) SubgraphPass);
  EXPECT_NE(pass_managers.Run(graph), SUCCESS);
}
}  // namespace ge
