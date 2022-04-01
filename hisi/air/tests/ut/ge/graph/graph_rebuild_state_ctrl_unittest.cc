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

#include "passes/graph_builder_utils.h"

#define private public
#define protected public
#include "graph/manager/util/graph_rebuild_state_ctrl.h"
#define protected public
#undef private

namespace ge {
class UtestGraphRebuildStateCtrl : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {
///      netoutput1
///         |
///       shapeNo1
///        |
///      addnYes1
///     /    \.
///   /       \.
/// const1   const2

ComputeGraphPtr BuildGraph1() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", "CONSTANT", 0, 1);
  auto const2 = builder.AddNode("const2", "CONSTANT", 0, 1);
  auto addn1 = builder.AddNode("addn1", "AddNYes", 2, 1);
  auto shape1 = builder.AddNode("shape1", "ShapeNo", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", "NETOUTPUT", 1, 0);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///
///          netoutput1
///        /   \      \.
///     add1  assign1   \.
///    /   \  /     \     \.
/// var1  var2    const1  var3

ComputeGraphPtr BuildGraph2() {
  auto builder = ut::GraphBuilder("test");
  auto var1 = builder.AddNode("var1", "Variable", 0, 1);
  auto var2 = builder.AddNode("var2", "VariableV2", 0, 1);
  auto var3 = builder.AddNode("var3", "VarHandleOp", 0, 1);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "Add", 2, 1);
  auto assign1 = builder.AddNode("assign1", "Assign", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "Netoutput", 3, 0);

  builder.AddDataEdge(var1, 0, add1, 0);
  builder.AddDataEdge(var2, 0, add1, 1);
  builder.AddDataEdge(var2, 0, assign1, 1);
  builder.AddDataEdge(var3, 0, netoutput1, 2);
  builder.AddDataEdge(const1, 0, assign1, 0);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(assign1, 0, netoutput1, 1);

  return builder.GetGraph();
}

}  // namespace

TEST_F(UtestGraphRebuildStateCtrl, add_graph_null_ptr) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, nullptr);
  EXPECT_TRUE(c.graph_ids_to_resource_names_.empty());
}

TEST_F(UtestGraphRebuildStateCtrl, add_graph_no_var) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph1());
  EXPECT_TRUE(c.graph_ids_to_resource_names_.count(1) > 0);
  EXPECT_TRUE(c.graph_ids_to_resource_names_[1].empty());
}

TEST_F(UtestGraphRebuildStateCtrl, add_graph_vars) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_TRUE(c.graph_ids_to_resource_names_.count(1) > 0);
  EXPECT_EQ(c.graph_ids_to_resource_names_[1].size(), 3);
  EXPECT_EQ(c.graph_ids_to_resource_names_[1].count("var1"), 1);
  EXPECT_EQ(c.graph_ids_to_resource_names_[1].count("var2"), 1);
  EXPECT_EQ(c.graph_ids_to_resource_names_[1].count("var3"), 1);
}

TEST_F(UtestGraphRebuildStateCtrl, remove_graph_vars) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_FALSE(c.graph_ids_to_resource_names_.empty());
  c.RemoveGraph(1);
  EXPECT_TRUE(c.graph_ids_to_resource_names_.empty());
}

TEST_F(UtestGraphRebuildStateCtrl, graph_rebuild) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
  c.SetStateChanged("var1");
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
}

TEST_F(UtestGraphRebuildStateCtrl, graph_rebuild_multi_changed) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
  c.SetStateChanged("var2");
  c.SetStateChanged("var3");
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
}

TEST_F(UtestGraphRebuildStateCtrl, graph_rebuild_multi_graph) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  c.AddGraph(2, BuildGraph2());
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
  EXPECT_FALSE(c.IsGraphNeedRebuild(2));
  c.SetStateChanged("var1");
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
  EXPECT_TRUE(c.IsGraphNeedRebuild(2));
}

TEST_F(UtestGraphRebuildStateCtrl,  graph_rebuild_after_remove_graph) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  c.AddGraph(2, BuildGraph2());
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
  EXPECT_FALSE(c.IsGraphNeedRebuild(2));
  c.SetStateChanged("var1");
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
  EXPECT_TRUE(c.IsGraphNeedRebuild(2));
  c.RemoveGraph(2);
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
  EXPECT_FALSE(c.IsGraphNeedRebuild(2));
}

TEST_F(UtestGraphRebuildStateCtrl, graph_rebuild_after_build_end) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  c.AddGraph(2, BuildGraph2());
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
  EXPECT_FALSE(c.IsGraphNeedRebuild(2));
  c.SetStateChanged("var1");
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
  EXPECT_TRUE(c.IsGraphNeedRebuild(2));
  c.SetGraphBuildEnd(2);
  EXPECT_TRUE(c.IsGraphNeedRebuild(1));
  EXPECT_FALSE(c.IsGraphNeedRebuild(2));
}

TEST_F(UtestGraphRebuildStateCtrl, var_permit_to_change) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
  c.SetStateChanged("var1");
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
}

TEST_F(UtestGraphRebuildStateCtrl, var_permit_to_change_remove_graph_not_change) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
  c.SetStateChanged("var1");
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
  c.RemoveGraph(1);
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
}

TEST_F(UtestGraphRebuildStateCtrl, var_permit_to_change_excceds_the_max_num) {
  GraphRebuildStateCtrl c;
  c.AddGraph(1, BuildGraph2());
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
  c.SetStateChanged("var1");
  c.SetStateChanged("var1");
  c.SetStateChanged("var1");
  c.SetStateChanged("var1");
  c.SetStateChanged("var1");
  c.SetStateChanged("var1");
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
}

TEST_F(UtestGraphRebuildStateCtrl, var_changed_before_add_graph) {
  GraphRebuildStateCtrl c;
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var1"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var2"));
  EXPECT_TRUE(c.IsVarPermitToChangeFormats("var3"));
  c.SetStateChanged("var1");
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  c.AddGraph(1, BuildGraph2());
  EXPECT_FALSE(c.IsVarPermitToChangeFormats("var1"));
  // graph no need to set again
  EXPECT_FALSE(c.IsGraphNeedRebuild(1));
}
}  // namespace ge