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
#include <string>
#include <map>
#include <gmock/gmock.h>

#define private public
#define protected public
#include "graph/node.h"
#include "graph/passes/variable_prepare_op_pass.h"
#include "ge_graph_dsl/graph_dsl.h"
#undef private
#undef protected

using namespace ge;

class UtestGraphPassesVariablePreparePass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

static ComputeGraphPtr BuildGraphVariablePreparePass() {
  DEF_GRAPH(variablePreparePassGraph) {
    CHAIN(NODE("variable", VARIABLE)->EDGE(0U, 0U)->NODE("assign", ASSIGN));
    CHAIN(NODE("const", CONSTANT)->EDGE(0U, 1U)->NODE("assign", ASSIGN));
    CHAIN(NODE("variable", CONSTANT)->EDGE(0U, 0U)->NODE("framworkop", FRAMEWORKOP));
    CHAIN(NODE("variable", VARIABLE)->EDGE(0U, 0U)->NODE("variable1", VARIABLE));
    CHAIN(NODE("variable1", VARIABLE)->EDGE(0U, 0U)->NODE("netoutput", NETOUTPUT));
  };

  const auto graph = ToGeGraph(variablePreparePassGraph);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  compute_graph->TopologicalSorting();

  // set the same name for in/output name for variable
  const map<string, uint32_t> name_index = {{"var", 0}};
  auto var = compute_graph->FindNode("variable");
  auto var_op_desc = var->GetOpDesc();
  var_op_desc->UpdateInputName(name_index);
  var_op_desc->UpdateOutputName(name_index);

  // set the same name for in/output name for variable1
  var = compute_graph->FindNode("variable1");
  var_op_desc = var->GetOpDesc();
  var_op_desc->UpdateInputName(name_index);
  var_op_desc->UpdateOutputName(name_index);

  // set the same name for in/output name for netoutput
  var = compute_graph->FindNode("netoutput");
  var_op_desc = var->GetOpDesc();
  var_op_desc->UpdateInputName(name_index);

  // set the same name for in/output name for assign
  var = compute_graph->FindNode("assign");
  var_op_desc = var->GetOpDesc();
  const map<string, uint32_t> name_index0 = {{"var", 0}};
  var_op_desc->UpdateInputName(name_index0);
  const map<string, uint32_t> name_index1 = {{"var", 0}};
  var_op_desc->UpdateInputName(name_index1);
  var_op_desc->UpdateOutputName(name_index0);

  return compute_graph;
}

TEST_F(UtestGraphPassesVariablePreparePass, variable_prepare_pass_succ1) {
  auto graph = BuildGraphVariablePreparePass();
  ge::VariablePrepareOpPass pass_;
  ge::Status status = pass_.Run(graph);
  EXPECT_EQ(SUCCESS, status);

  // InsertVariableRef
  auto cur_node = graph->FindNode("variable1");
  auto var_node = graph->FindNode("netoutput");
  status = pass_.InsertVariableRef(cur_node, 0U, var_node);
  EXPECT_EQ(SUCCESS, status);
}

TEST_F(UtestGraphPassesVariablePreparePass, get_peer_node_failed) {
  auto graph = BuildGraphVariablePreparePass();
  VariablePrepareOpPass pass_;

  // invalid input failed
  auto cur_node = graph->FindNode("variable1");
  std::stack<std::pair<NodePtr, std::pair<int32_t, int32_t>>> nodes;
  ge::Status status = pass_.GetPeerNodeOfRefOutput(cur_node, -1, nodes);
  EXPECT_EQ(PARAM_INVALID, status);

  status = pass_.GetPeerNodeOfRefOutput(cur_node, 1U, nodes);
  EXPECT_EQ(SUCCESS, status);

  status = pass_.GetPeerNodeOfRefOutput(cur_node, 2U, nodes);
  EXPECT_EQ(SUCCESS, status);
}

TEST_F(UtestGraphPassesVariablePreparePass, add_variable_ref) {
  auto graph = BuildGraphVariablePreparePass();
  VariablePrepareOpPass pass_;

  // invalid input failed
  auto cur_node = graph->FindNode("variable");
  auto var_node = graph->FindNode("variable1");
  ge::Status status = pass_.AddVariableRef(cur_node, var_node, 3U);
  EXPECT_EQ(SUCCESS, status);

  OpDescPtr pdesc = var_node->GetOpDesc();
  (void)ge::AttrUtils::SetStr(pdesc, REF_VAR_SRC_VAR_NAME, "src_var_name");
  status = pass_.AddVariableRef(cur_node, var_node, 0U);
  EXPECT_EQ(SUCCESS, status);
}

TEST_F(UtestGraphPassesVariablePreparePass, create_ref_identity_failed) {
  VariablePrepareOpPass pass_;
  // invalid input failed
  const NodePtr node = shared_ptr<ge::Node>(new (std::nothrow) ge::Node());
  auto ptr = pass_.CreateRefIdentity("src_var_name", node, 0U);
  EXPECT_EQ(nullptr, ptr);
}

TEST_F(UtestGraphPassesVariablePreparePass, create_var_ref_failed) {
  VariablePrepareOpPass pass_;
  // invalid input failed
  const NodePtr node = shared_ptr<ge::Node>(new (std::nothrow) ge::Node());
  auto ptr = pass_.CreateVariableRef("src_var_name", node);
  EXPECT_EQ(nullptr, ptr);
}

TEST_F(UtestGraphPassesVariablePreparePass, GetWritableNodeOutIndex) {
  VariablePrepareOpPass pass_;
  // invalid input failed
  std::vector<int32_t> vec;
  pass_.GetWritableNodeOutIndex(nullptr, 0U, vec);
}

TEST_F(UtestGraphPassesVariablePreparePass, generate_ref_type_and_inputoutput_map_failed) {
  VariablePrepareOpPass pass_;
  // invalid input failed
  const NodePtr node = shared_ptr<ge::Node>(new (std::nothrow) ge::Node());
  pass_.GenerateRefTypeAndInputOutputMap(node);
}