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

#define protected public
#define private public
#include "graph/passes/common_subexpression_elimination_pass.h"
#include "graph/passes/base_pass.h"
#include "graph/utils/tensor_utils.h"
#include "graph/tuning_utils.h"
#include "graph_builder_utils.h"
#include "graph/ge_context.h"
#include "inc/pass_manager.h"
#include "inc/kernel_factory.h"
#include "inc/kernel.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
using namespace ge;

class UTestCommonSubexpressionEliminationPass : public Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {
const char *const kAddYes = "CseAddYes";
class TestKernel : public Kernel {
 public:
  Status Compute(const ge::NodePtr &node,
                 std::vector<ge::GeTensorPtr> &v_output) {
    return SUCCESS;
  }
};
REGISTER_KERNEL(kAddYes, TestKernel);


/*
 *  netoutput1
 *   /  \
 * ===========
 *  add1 add2
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph1() {
  ut::GraphBuilder builder("g1");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);

  return builder.GetGraph();
}

/*
 *  netoutput1
 *   /  \
 * ===========
 *  add1 add2
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph1Switch() {
  ut::GraphBuilder builder("g1");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 1);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 0);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);

  return builder.GetGraph();
}

/*
 *  netoutput1
 *   /  \
 * ===========
 *  add1 add2
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph1NotSupportType() {
  ut::GraphBuilder builder("g1");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "AddNo", 2, 1);
  auto add2 = builder.AddNode("add2", "AddNo", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);

  return builder.GetGraph();
}

/*
 *  netoutput1
 *   /  \
 * ===========
 *  add1 add2 add3 add4
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph2() {
  ut::GraphBuilder builder("g2");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto add3 = builder.AddNode("add3", "CseAddYes", 2, 1);
  auto add4 = builder.AddNode("add4", "CseAddYes", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 4, 4);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const1, 0, add3, 0);
  builder.AddDataEdge(const1, 0, add4, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(const2, 0, add3, 1);
  builder.AddDataEdge(const2, 0, add4, 1);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);
  builder.AddDataEdge(add3, 0, netoutput1, 2);
  builder.AddDataEdge(add4, 0, netoutput1, 3);

  return builder.GetGraph();
}

/*
 *               netoutput1
 *              /      \
 * ===========||   c    \
 *  add1 add2 || <----addn1
 * ===========||       |
 *   |     \           |
 * const1   const2   data1
 */
ComputeGraphPtr BuildGraph3() {
  ut::GraphBuilder builder("g3");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 3, 3);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddControlEdge(addn1, add1);
  builder.AddControlEdge(addn1, add2);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);
  builder.AddDataEdge(addn1, 0, netoutput1, 2);

  return builder.GetGraph();
}

/*
 *  netoutput1
 *   /    \
 * addn1  |
 *   |    |
 *  add1 add2
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph4() {
  ut::GraphBuilder builder("4");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(add1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);

  return builder.GetGraph();
}

/*
 *  netoutput1
 *   /  \
 * ===========
 *  add1 add2
 *  =========
 *   |     \
 * const1   const2
 */
ComputeGraphPtr BuildGraph5() {
  ut::GraphBuilder builder("g5");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto add3 = builder.AddNode("add3", "CseAddYes", 2, 1);
  auto add4 = builder.AddNode("add4", "CseAddYes", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 4, 4);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const1, 0, add3, 0);
  builder.AddDataEdge(const1, 0, add4, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(const2, 0, add3, 1);
  builder.AddDataEdge(const2, 0, add4, 1);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);
  builder.AddDataEdge(add3, 0, netoutput1, 2);
  builder.AddDataEdge(add4, 0, netoutput1, 3);

  return builder.GetGraph();
}

/*
 *         netoutput1
 *         /      \
 *      add3       add4
 *     /    \     /   \
 *  add1    const3     add2
 * =======================
 *        |     \
 *      const1   const2
 */
ComputeGraphPtr BuildGraph6() {
  ut::GraphBuilder builder("6");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto const3 = builder.AddNode("const3", "Const", 0, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto add3 = builder.AddNode("add3", "CseAddYes", 2, 1);
  auto add4 = builder.AddNode("add4", "CseAddYes", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 2);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(add1, 0, add3, 0);
  builder.AddDataEdge(add2, 0, add4, 0);
  builder.AddDataEdge(const3, 0, add3, 1);
  builder.AddDataEdge(const3, 0, add4, 1);

  builder.AddDataEdge(add3, 0, netoutput1, 0);
  builder.AddDataEdge(add4, 0, netoutput1, 1);

  return builder.GetGraph();
}

/*
 *               netoutput1
 *              /      \
 * ===========||   c    \
 *  add1 add2 || <----addn1,2
 * ===========||       |
 *   |     \           |
 * const1   const2   data1
 */
ComputeGraphPtr BuildGraph7() {
  ut::GraphBuilder builder("7");

  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 4, 4);

  builder.AddDataEdge(const1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add2, 0);
  builder.AddDataEdge(const2, 0, add1, 1);
  builder.AddDataEdge(const2, 0, add2, 1);
  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(data1, 0, addn2, 0);
  builder.AddControlEdge(addn1, add1);
  builder.AddControlEdge(addn2, add2);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(add2, 0, netoutput1, 1);
  builder.AddDataEdge(addn1, 0, netoutput1, 2);
  builder.AddDataEdge(addn2, 0, netoutput1, 3);

  return builder.GetGraph();
}

/*
 *            netoutput1
 *               |
 *             merge1
 *            /      \
 *         add1     add2
 *       /   \t    f/  \
 * const2    switch1   const2(same one to the left)
 *           /    \
 *       data1   const1
 */
ComputeGraphPtr BuildGraph8() {
  ut::GraphBuilder builder("8");
  auto data1 = builder.AddNode("data1", "Data1", 1,1);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto switch1 = builder.AddNode("switch1", "Switch", 2, 2);
  auto add1 = builder.AddNode("add1", "CseAddYes", 2, 1);
  auto add2 = builder.AddNode("add2", "CseAddYes", 2, 1);
  auto merge1 = builder.AddNode("merge1", "Merge", 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 1, 1);

  builder.AddDataEdge(data1, 0, switch1, 0);
  builder.AddDataEdge(const1, 0, switch1, 1);
  builder.AddDataEdge(const2, 0, add1, 0);
  builder.AddDataEdge(const2, 0, add2, 0);
  builder.AddDataEdge(switch1, 0, add1, 1);
  builder.AddDataEdge(switch1, 1, add2, 1);
  builder.AddDataEdge(add1, 0, merge1, 0);
  builder.AddDataEdge(add2, 0, merge1, 1);
  builder.AddDataEdge(merge1, 0, netoutput1, 0);

  return builder.GetGraph();
}

std::string FindFirstNode(const ComputeGraphPtr &graph, const std::string &type) {
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == type) {
      return node->GetName();
    }
  }
  return "";
}

std::string FindFirstNode(const ComputeGraphPtr &graph, const std::string &type, std::initializer_list<std::string> names) {
  std::set<std::string> names_set(names);
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == type && names_set.count(node->GetName()) > 0) {
      return node->GetName();
    }
  }
  return "";
}
NodePtr FindFirstNodeIns(const ComputeGraphPtr &graph, const std::string &type) {
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() == type) {
      return node;
    }
  }
  return nullptr;
}

std::set<std::string> GetNames(const Node::Vistor<NodePtr> &node_list) {
  std::set<std::string> name_set;
  for (const auto &node : node_list) {
    name_set.insert(node->GetName());
  }
  return  name_set;
}

std::set<std::string> GetNames(const ComputeGraph::Vistor<NodePtr> &node_list) {
  std::set<std::string> name_set;
  for (const auto &node : node_list) {
    name_set.insert(node->GetName());
  }
  return  name_set;
}
} // namespace

TEST_F(UTestCommonSubexpressionEliminationPass, Success) {
  auto graph = BuildGraph1();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 4);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", FindFirstNode(graph, "CseAddYes"), "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 1);
  EXPECT_EQ(const2->GetOutNodes().size(), 1);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()).size(), 1);
  EXPECT_EQ(const1->GetOutNodes().at(0)->GetName(), netoutput1->GetInNodes().at(0)->GetName());
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessWithControlEdges) {
  auto graph = BuildGraph3();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 6);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "data1", "addn1", FindFirstNode(graph, "CseAddYes"), "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto addn1 = graph->FindNode("addn1");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 1);
  EXPECT_EQ(const2->GetOutNodes().size(), 1);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 3);
  EXPECT_EQ(GetNames(netoutput1->GetInDataNodes()), std::set<std::string>({"addn1", FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(const1->GetOutNodes().at(0)->GetName(), netoutput1->GetInNodes().at(0)->GetName());
  EXPECT_EQ(addn1->GetOutNodes().size(), 2);
  EXPECT_EQ(addn1->GetOutControlNodes().size(), 1);
  EXPECT_EQ(addn1->GetOutDataNodes().size(), 1);
  EXPECT_EQ(GetNames(addn1->GetOutControlNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(GetNames(addn1->GetOutDataNodes()), std::set<std::string>({"netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessFourNodes) {
  auto graph = BuildGraph2();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 4);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", FindFirstNode(graph, "CseAddYes"), "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 1);
  EXPECT_EQ(const2->GetOutNodes().size(), 1);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 4);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()).size(), 1);
  EXPECT_EQ(const1->GetOutNodes().at(0)->GetName(), netoutput1->GetInNodes().at(0)->GetName());
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessDiffOutNodes) {
  auto graph = BuildGraph4();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "addn1", FindFirstNode(graph, "CseAddYes"), "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 1);
  EXPECT_EQ(const2->GetOutNodes().size(), 1);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes")}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()).size(), 2);

  auto add = FindFirstNodeIns(graph, "CseAddYes");
  EXPECT_EQ(GetNames(add->GetOutNodes()), std::set<std::string>({"addn1", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessContinuesNodes) {
  auto graph = BuildGraph6();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 6);
  EXPECT_EQ(GetNames(graph->GetAllNodes()),
            std::set<std::string>({"const1", "const2", "const3", "netoutput1",
                                   FindFirstNode(graph, "CseAddYes", {"add1", "add2"}),
                                   FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto const3 = graph->FindNode("const3");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add1", "add2"})}));
  EXPECT_EQ(GetNames(const2->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add1", "add2"})}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
  EXPECT_EQ(GetNames(const3->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessContainsAttr) {
  auto graph = BuildGraph6();
  auto add1 = graph->FindNode("add1");
  auto add2 = graph->FindNode("add2");
  AttrUtils::SetListInt(add1->GetOpDesc(), "TestAttr1", std::vector<int64_t>({1,10, 100,1000}));
  AttrUtils::SetListInt(add2->GetOpDesc(), "TestAttr1", std::vector<int64_t>({1,10, 100,1000}));
  AttrUtils::SetBool(add1->GetOpDesc(), "TestAttr2", true);
  AttrUtils::SetBool(add2->GetOpDesc(), "TestAttr2", true);

  auto add3 = graph->FindNode("add3");
  auto add4 = graph->FindNode("add4");
  AttrUtils::SetStr(add3->GetOpDesc(), "TestAttr1", "HelloWorld111");
  AttrUtils::SetStr(add4->GetOpDesc(), "TestAttr1", "HelloWorld111");
  AttrUtils::SetFloat(add3->GetOpDesc(), "TestAttr2",  3.14);
  AttrUtils::SetFloat(add4->GetOpDesc(), "TestAttr2", 3.14);

  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 6);
  EXPECT_EQ(GetNames(graph->GetAllNodes()),
            std::set<std::string>({"const1", "const2", "const3", "netoutput1",
                                   FindFirstNode(graph, "CseAddYes", {"add1", "add2"}),
                                   FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto const3 = graph->FindNode("const3");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add1", "add2"})}));
  EXPECT_EQ(GetNames(const2->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add1", "add2"})}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
  EXPECT_EQ(GetNames(const3->GetOutNodes()), std::set<std::string>({FindFirstNode(graph, "CseAddYes", {"add3", "add4"})}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, SuccessTwoGroups) {

}

TEST_F(UTestCommonSubexpressionEliminationPass, NoCse) {
  auto graph = BuildGraph1();
  auto add1 = graph->FindNode("add1");
  AttrUtils::SetListInt(add1->GetOpDesc(), "TestAttr1", std::vector<int64_t>({1,10, 100,1000}));
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "add1", "add2", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, InputSwitch) {
  auto graph = BuildGraph1Switch();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "add1", "add2", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, DifferentAttr) {
  auto graph = BuildGraph1();
  auto add1 = graph->FindNode("add1");
  auto add2 = graph->FindNode("add2");
  AttrUtils::SetListInt(add1->GetOpDesc(), "TestAttr1", std::vector<int64_t>({1,10, 100,1000}));
  AttrUtils::SetListInt(add2->GetOpDesc(), "TestAttr1", std::vector<int64_t>({1,10, 100,1001}));
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "add1", "add2", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, NodeTypeNotSupport) {
  auto graph = BuildGraph1NotSupportType();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2", "add1", "add2", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, DifferentControlEdges) {
  auto graph = BuildGraph7();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 8);
  EXPECT_EQ(GetNames(graph->GetAllNodes()),
            std::set<std::string>({"const1", "const2", "data1", "add1", "add2", "addn1", "addn2", "netoutput1"}));
}


TEST_F(UTestCommonSubexpressionEliminationPass, DiffOutAnchorsFromOneNode) {
  auto graph = BuildGraph8();
  CommonSubexpressionEliminationPass pass;
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 8);
  EXPECT_EQ(GetNames(graph->GetAllNodes()),
            std::set<std::string>({"const1", "const2", "data1", "add1", "add2", "merge1", "switch1", "netoutput1"}));
}

TEST_F(UTestCommonSubexpressionEliminationPass, CseWithGetUnknowShapeFailed) {
  auto graph = BuildGraph1();
  CommonSubexpressionEliminationPass pass;
  auto add1 = graph->FindNode("add1");
  auto op_desc1 = add1->GetOpDesc();
  auto add2 = graph->FindNode("add2");
  auto op_desc2 = add2->GetOpDesc();
  op_desc1->AddSubgraphName("EmptySubGraph");
  op_desc2->AddSubgraphName("EmptySubGraph");
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2",
                                                                       "add1", "add2", "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 2);
  EXPECT_EQ(const2->GetOutNodes().size(), 2);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({"add1", "add2"}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()).size(), 2);
  EXPECT_EQ(const1->GetOutNodes().at(0)->GetName(), netoutput1->GetInNodes().at(0)->GetName());
}

TEST_F(UTestCommonSubexpressionEliminationPass, CseWithUnknowShape) {
  auto graph = BuildGraph1();
  CommonSubexpressionEliminationPass pass;
  auto add1 = graph->FindNode("add1");
  auto op_desc1 = add1->GetOpDesc();
  auto add2 = graph->FindNode("add2");
  auto op_desc2 = add2->GetOpDesc();
  std::vector<int64_t> dims = {-1, 224, 224, 3};
  op_desc1->MutableOutputDesc(0)->SetShape(GeShape(dims));
  op_desc2->MutableOutputDesc(0)->SetShape(GeShape(dims));
  EXPECT_EQ(pass.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 5);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"const1", "const2",
                                                                       "add1", "add2", "netoutput1"}));
  auto const1 = graph->FindNode("const1");
  auto const2 = graph->FindNode("const2");
  auto netoutput1 = graph->FindNode("netoutput1");
  EXPECT_EQ(const1->GetOutNodes().size(), 2);
  EXPECT_EQ(const2->GetOutNodes().size(), 2);
  EXPECT_EQ(GetNames(const1->GetOutNodes()), std::set<std::string>({"add1", "add2"}));
  EXPECT_EQ(netoutput1->GetInNodes().size(), 2);
  EXPECT_EQ(GetNames(netoutput1->GetInNodes()).size(), 2);
  EXPECT_EQ(const1->GetOutNodes().at(0)->GetName(), netoutput1->GetInNodes().at(0)->GetName());
}