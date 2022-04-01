/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "graph/passes/variable_ref_useless_control_out_delete_pass.h"
#include "graph_builder_utils.h"
namespace ge {
class UtestVariableRefUselessControlOutDeletePass : public testing::Test
{
 protected:
  void SetUp()
  {
  }

  void TearDown()
  {
  }
};

namespace {
/*
 *      c
 * var1 -> addn2
 *   \    /
 *    addn1
 *     |
 *    data1
 */
ComputeGraphPtr BuildGraph1() {
  ut::GraphBuilder builder("g1");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto var1 = builder.AddNode("var1", "Variable", 1, 1);
  auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);
  AttrUtils::SetStr(var1->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "TestVar");
  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, var1, 0);
  builder.AddDataEdge(addn1, 0, addn2, 0);
  builder.AddControlEdge(var1, addn2);
  return builder.GetGraph();
}

/*
 *      c
 * var1 -> addn2
 *   \       |
 *    addn1  |
 *        \  |
 *        data1
 */
ComputeGraphPtr BuildGraph2() {
  ut::GraphBuilder builder("g2");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto var1 = builder.AddNode("var1", "Variable", 1, 1);
  auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);

  AttrUtils::SetStr(var1->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "TestVar");

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, var1, 0);
  builder.AddDataEdge(data1, 0, addn2, 0);
  builder.AddControlEdge(var1, addn2);

  return builder.GetGraph();
}

/*
 *   netoutput1
 *   /    \
 * var1  addn2
 *   \    /
 *    addn1
 *     |
 *    data1
 */
ComputeGraphPtr BuildGraph3() {
  ut::GraphBuilder builder("g3");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto var1 = builder.AddNode("var1", "Variable", 1, 1);
  auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  AttrUtils::SetStr(var1->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "TestVar");

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, var1, 0);
  builder.AddDataEdge(addn1, 0, addn2, 0);
  builder.AddDataEdge(var1, 0, netoutput1, 0);
  builder.AddDataEdge(addn2, 0, netoutput1, 1);

  return builder.GetGraph();
}
}

TEST_F(UtestVariableRefUselessControlOutDeletePass, DeleteControlOutSuccess) {
  auto graph = BuildGraph1();
  VariableRefUselessControlOutDeletePass pass;

  auto var1 = graph->FindNode("var1");
  EXPECT_EQ(var1->GetOutControlNodes().size(), 1);
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 4);
  EXPECT_EQ(var1->GetOutControlNodes().size(), 0);
}

TEST_F(UtestVariableRefUselessControlOutDeletePass, KeepControlOutSuccess) {
  auto graph = BuildGraph2();
  VariableRefUselessControlOutDeletePass pass;

  auto var1 = graph->FindNode("var1");
  EXPECT_EQ(var1->GetOutControlNodes().size(), 1);
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 4);
  EXPECT_EQ(var1->GetOutControlNodes().size(), 1);
}

TEST_F(UtestVariableRefUselessControlOutDeletePass, NothingChanged) {
  auto graph = BuildGraph3();
  VariableRefUselessControlOutDeletePass pass;

  auto var1 = graph->FindNode("var1");
  EXPECT_EQ(var1->GetOutNodes().size(), 1);
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 5);
  EXPECT_EQ(var1->GetOutNodes().size(), 1);
}
}