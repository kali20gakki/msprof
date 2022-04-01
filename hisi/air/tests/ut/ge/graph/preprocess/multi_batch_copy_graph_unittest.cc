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
#include <memory>

#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#include "common/util.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/manager/graph_var_manager.h"

#define private public
#define protected public
#include "graph/op_desc_impl.h"
#include "graph/op_desc.h"
#include "graph/ge_local_context.h"
#include "graph/preprocess/graph_preprocess.h"
#include "graph/preprocess/multi_batch_copy_graph.h"
#include "ge/ge_api.h"
#undef private
#undef protected

using namespace std;
namespace ge {
namespace multibatch {
class MultiBatchGraphCopyerUnitTest : public testing::Test {
 protected:
  void SetUp() {
    SetLocalOmgContext(domi::GetContext());
    GetLocalOmgContext().need_multi_batch = true;
  }

  void TearDown() {
  }
};

namespace {
const int32_t kDataOutIndex = 0;
const int32_t kDataInIndex = 0;
std::set<std::string> GetNames(const ComputeGraph::Vistor<NodePtr> &nodes) {
  std::set<std::string> names;
  for (auto &node : nodes) {
    names.insert(node->GetName());
  }
  return names;
}

/*
 *                             netoutput1
 *                                 |
 *                               merge
 *                             /      \
 *  netoutput1            addn1_b1    addn1_b2
 *    |                        \     /
 *  addn1        ===>          switchn
 *   |                         /     \
 * data1                   data1     shape_data
 */
ComputeGraphPtr BuildGraph1() {
  ut::GraphBuilder builder("g1");
  auto data1 = builder.AddNode("data1", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {-1, 3, 224, 224});
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 1, 0);

  EXPECT_TRUE(AttrUtils::SetInt(addn1->GetOpDesc(), "TEST_OP_ATTR", 512));
  auto input_desc = addn1->GetOpDesc()->GetInputDesc(0);
  EXPECT_TRUE(AttrUtils::SetInt(input_desc, "TEST_ATTR", 1024));
  addn1->GetOpDesc()->UpdateInputDesc(0, input_desc);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  auto graph = builder.GetGraph();

  graph->SetInputSize(1);
  graph->SetInputsOrder({"data1"});
  graph->AddInputNode(data1);

  return graph;
}

ComputeGraphPtr BuildGraph1_hw() {
  ut::GraphBuilder builder("g1");
  auto data1 = builder.AddNode("data1", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {8, 3, -1, -1});
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 1, 0);

  EXPECT_TRUE(AttrUtils::SetInt(addn1->GetOpDesc(), "TEST_OP_ATTR", 512));
  auto input_desc = addn1->GetOpDesc()->GetInputDesc(0);
  EXPECT_TRUE(AttrUtils::SetInt(input_desc, "TEST_ATTR", 1024));
  addn1->GetOpDesc()->UpdateInputDesc(0, input_desc);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  auto graph = builder.GetGraph();

  graph->SetInputSize(1);
  graph->SetInputsOrder({"data1"});
  graph->AddInputNode(data1);

  return graph;
}

/*
 *                                         netoutput1
 *                                          /      \
 *                                       merge      \
 *                                        |          \
 *                                  ---------------   |
 *                                  addn2_b1 .. b3    |
 *       netoutput1                       |           |
 *         |     \                  conv1_b1 .. b3    |
 *       addn2   |                  ---------------  addn1
 *        |      |                   |             \  |
 *      conv1  addn1     ===>    switchn            var1
 *     /    \   |                /     \
 * data1     var1            data1     shape_data
 */
ComputeGraphPtr BuildGraph3() {
  ut::GraphBuilder builder("g3");
  auto data1 = builder.AddNode("data1", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {-1, 3, 224, 224});
  auto var1 = builder.AddNode("var1", "Variable", 1, 1, FORMAT_HWCN, DT_FLOAT, {4, 4, 3, 16});
  auto conv1 = builder.AddNode("conv1", "Conv2D", 2, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 2, 0);

  builder.AddDataEdge(data1, 0, conv1, 0);
  builder.AddDataEdge(var1, 0, conv1, 1);
  builder.AddDataEdge(var1, 0, addn1, 0);
  builder.AddDataEdge(conv1, 0, addn2, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  builder.AddDataEdge(addn2, 0, netoutput1, 1);
  auto graph = builder.GetGraph();

  graph->SetInputSize(1);
  graph->SetInputsOrder({"data1"});
  graph->AddInputNode(data1);

  return graph;
}
}


TEST_F(MultiBatchGraphCopyerUnitTest, ProcessMultiBatch_NoNeed) {
  auto graph = BuildGraph1();
  EXPECT_EQ(ProcessMultiBatch(graph, 0), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 3);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"data1", "addn1", "netoutput1"}));
}

TEST_F(MultiBatchGraphCopyerUnitTest, ProcessMultiBatch_EmptyBatchSize) {
  auto graph = BuildGraph1();
  // GetThreadLocalContext().SetGlobalOption(std::map<std::string, std::string>({{"ge.dynamicBatchSize","2,4,,88"}}));
  GetLocalOmgContext().dynamic_batch_size = "2,4,88";
  EXPECT_EQ(ProcessMultiBatch(graph, 0), SUCCESS);
  GetLocalOmgContext().dynamic_batch_size.clear();
}

TEST_F(MultiBatchGraphCopyerUnitTest, ProcessMultiBatch_EmptyImageSize) {
  auto graph = BuildGraph1_hw();
  // GetThreadLocalContext().SetGlobalOption(std::map<std::string, std::string>({{"ge.dynamicImageSize",
  //                                                                                   "22,22;44,44;;99,99"}}));
  GetLocalOmgContext().dynamic_image_size = "22,22;44,44;;99,99";
  EXPECT_EQ(ProcessMultiBatch(graph, 0), SUCCESS);
  GetLocalOmgContext().dynamic_image_size.clear();
}

TEST_F(MultiBatchGraphCopyerUnitTest, GetDynamicOutputShape_success) {
  auto graph = BuildGraph3();
  GetLocalOmgContext().dynamic_batch_size = "2,4,88";
  auto ret = ProcessMultiBatch(graph, 0);
  ret = GetDynamicOutputShape(graph);
  EXPECT_EQ(ret, SUCCESS);
  GetLocalOmgContext().dynamic_batch_size.clear();
}

TEST_F(MultiBatchGraphCopyerUnitTest, ProcessMultiBatch_EnvNoNeed) {
  GetLocalOmgContext().need_multi_batch = false;
  auto graph = BuildGraph1();
  EXPECT_EQ(ProcessMultiBatch(graph, 0), SUCCESS);
  EXPECT_EQ(graph->GetAllNodesSize(), 3);
  EXPECT_EQ(GetNames(graph->GetAllNodes()), std::set<std::string>({"data1", "addn1", "netoutput1"}));
  GetLocalOmgContext().need_multi_batch = true;
}
}
}
