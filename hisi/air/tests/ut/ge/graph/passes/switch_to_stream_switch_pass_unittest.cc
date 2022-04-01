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
#define private public
#define protected public
#include "common/ge_inner_error_codes.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/node_adapter.h"
#include "graph/ge_context.h"
#include "graph/ge_local_context.h"
#include "graph/passes/switch_to_stream_switch_pass.h"
#undef private
#undef protected
using namespace ge;

class UtestSwitch2StreamSwitchPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

//string name, string type, int in_cnt, int out_cnt, Format format,
//DataType data_type, vector<int64_t> shape
static ComputeGraphPtr BuildGraph() {
  DEF_GRAPH(switchGraph) {
                           CHAIN(NODE("arg_0", DATA)->NODE("addNode", ADD)->NODE("output", NETOUTPUT));
                           CHAIN(NODE("arg_1", DATA)->NODE("addNode"));

                           CHAIN(NODE("cast0", CAST)->EDGE(0U, 0U)->NODE("switch1", SWITCH));
                           CHAIN(NODE("switch1", SWITCH)->EDGE(0U, 0U)->NODE("switch2", SWITCH));

                           CHAIN(NODE("data0", DATA)->EDGE(0U, 0U)->NODE("switch3", SWITCH));
                          };

  return ToComputeGraph(switchGraph);;
}

TEST_F(UtestSwitch2StreamSwitchPass, Run0) {
  Status retStatus;
  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  retStatus = switch2StrPass.Run(graph);

  retStatus = switch2StrPass.ClearStatus();
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestSwitch2StreamSwitchPass, FindSwitchCondInputSuccess) {
  Status retStatus;
  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  auto node = graph->FindNode("switch1");
  auto data_anchor = node->GetOutDataAnchor(0U);
  retStatus = switch2StrPass.FindSwitchCondInput(data_anchor);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestSwitch2StreamSwitchPass, MarkBranchesSuccess) {
  Status retStatus;
  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  auto node = graph->FindNode("cast0");
  auto data_anchor = node->GetOutDataAnchor(0U);
  auto vec = std::vector<std::list<NodePtr>>();
  std::map<int64_t, std::vector<std::list<NodePtr>>> m;

  switch2StrPass.cond_node_map_[data_anchor] = m;
  retStatus = switch2StrPass.MarkBranches(data_anchor, node, true);

}

TEST_F(UtestSwitch2StreamSwitchPass, GetGroupIdSuccess) {

  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  auto node = graph->FindNode("cast0");
  GetThreadLocalContext().graph_options_.insert({OPTION_EXEC_ENABLE_TAILING_OPTIMIZATION, "1"});
  auto ret = switch2StrPass.GetGroupId(node);
  EXPECT_EQ(ret, 0);

  AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_HCCL_FUSED_GROUP, "a");
  ret = switch2StrPass.GetGroupId(node);
  EXPECT_EQ(ret, 0);

  AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_HCCL_FUSED_GROUP, "_123");
  ret = switch2StrPass.GetGroupId(node);
  EXPECT_EQ(ret, 123);
}

TEST_F(UtestSwitch2StreamSwitchPass, ModifySwitchInCtlEdgesFailed) {

  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  auto switch1 = graph->FindNode("switch1");
  auto cast0 = graph->FindNode("cast0");
  auto s = std::set<NodePtr>({cast0});
  auto ret = switch2StrPass.ModifySwitchInCtlEdges(switch1, cast0, s);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

TEST_F(UtestSwitch2StreamSwitchPass, MoveCtrlEdgesFailed) {

  SwitchToStreamSwitchPass switch2StrPass;
  ComputeGraphPtr graph = BuildGraph();
  auto switch3 = graph->FindNode("switch3");
  auto switch2 = graph->FindNode("switch2");
  auto data0 = graph->FindNode("data0");
  auto cast0 = graph->FindNode("cast0");
  switch2StrPass.switch_cyclic_map_[data0] = {"switch2"};

  GraphUtils::AddEdge(switch2->GetOutControlAnchor(), data0->GetInControlAnchor());
  GraphUtils::AddEdge(data0->GetOutControlAnchor(), cast0->GetInControlAnchor());
  switch2StrPass.MoveCtrlEdges(data0, switch3);
}

