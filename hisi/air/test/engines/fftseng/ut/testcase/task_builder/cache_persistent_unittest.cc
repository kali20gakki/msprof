/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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
#include <iostream>

#include <list>

#define private public
#define protected public
#include "common/sgt_slice_type.h"
#include "inc/ffts_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "task_builder/fftsplus_ops_kernel_builder.h"
#include "task_builder/data_ctx/cache_persistent_manual_task_builder.h"
#include "graph/node.h"
#include "graph_builder_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/compute_graph.h"
#include "graph/op_kernel_bin.h"
#include "runtime/context.h"
#include "runtime/stream.h"
#include "runtime/rt_model.h"
#include "runtime/kernel.h"
#include "ops_store/ops_kernel_manager.h"
#include "optimizer/graph_optimizer/fftsplus_graph_optimizer.h"
#include "optimizer/cache_optimizer/cache_manager.h"
#include "../../../graph_constructor/graph_constructor.h"
#include "graph/debug/ge_attr_define.h"
#include "../test_utils.h"

using namespace std;
using namespace ffts;
using namespace ge;
using namespace fe;
using FFTSPlusGraphOptimizerPtr = std::shared_ptr<FFTSPlusGraphOptimizer>;
class UTEST_CACHE_PERSISTETNT : public testing::Test {
 protected:

  void SetUp() {

  }

  void TearDown() {


  }
};

void SecondGraphPartition(ComputeGraphPtr &sub_graph, ge::NodePtr &place_holder0, ge::NodePtr &place_holder1,
                          bool set_thread_scope=true) {
  /* Do the second time partition for sub graph optimization for FE.
   * Create three new placeholder and set there parent node as these three data. */
  ge::NodePtr data0;
  ge::NodePtr data1;
  ge::NodePtr data2;
  for (const auto &node : sub_graph->GetAllNodes()) {
    auto name = node->GetName();
    if (name == "test_sgt_graph_0/Data_0") {
      data0 = node;
      ge::GraphUtils::IsolateNode(node, {0});
      continue;
    }
    if (name == "test_sgt_graph_0/Data_1") {
      data1 = node;
      ge::GraphUtils::IsolateNode(node, {0});
      continue;
    }
    if (name == "test_sgt_graph_0/Data_2") {
      data2 = node;
      ge::GraphUtils::IsolateNode(node, {0});
      continue;
    }
  }

  ge::GeShape original_shape = GeShape({3, 12, 5, 6});
  GraphConstructor test3(sub_graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT, original_shape);
  test3.SetInput("test_sgt_graph_0/am1:0", ge::FORMAT_NC1HWC0, "PlaceHolder_2:0", ge::FORMAT_NC1HWC0)
       .SetInput("test_sgt_graph_0/am2:0", "PlaceHolder_3:0")
       .SetInput("test_sgt_graph_0/am3:0", ge::FORMAT_FRACTAL_Z, "PlaceHolder_4:0", ge::FORMAT_FRACTAL_Z);

  ge::NodePtr place_holder2;
  ge::NodePtr place_holder3;
  ge::NodePtr place_holder4;

  test3.GetNodeByName("PlaceHolder_2", place_holder2);
  test3.GetNodeByName("PlaceHolder_3", place_holder3);
  test3.GetNodeByName("PlaceHolder_4", place_holder4);
  place_holder2->GetOpDesc()->SetExtAttr(ATTR_NAME_PARENT_NODE, data0);
  place_holder3->GetOpDesc()->SetExtAttr(ATTR_NAME_PARENT_NODE, data1);
  place_holder4->GetOpDesc()->SetExtAttr(ATTR_NAME_PARENT_NODE, data2);

  auto op_desc_plhd0 = place_holder0->GetOpDesc();
  auto op_desc_plhd1 = place_holder1->GetOpDesc();
  op_desc_plhd0->SetType("Const");
  op_desc_plhd0->SetName("const_pld0");
  if (set_thread_scope) {
    (void)ge::AttrUtils::SetListInt(op_desc_plhd0, kCachePersistScopeIds, {13});
  }

  op_desc_plhd1->SetType("Const");
  op_desc_plhd1->SetName("const_pld1");
}

Status CreateGraphOfFunctionOp1(ComputeGraphPtr graph, ge::NodePtr &const_node,
                                ge::NodePtr &place_holder0, ge::NodePtr &place_holder1,
                                bool set_thread_scope=true) {
  /* In this graph we will create a function op case
   * which have three inputs and three outputs.
   * Graph will be like:
   * parent node of placeholder:
   *                   (Const                    Constant)
   *                     |                          |
   *                PlaceHolder0               PlaceHolder1
   *                     |      \                   |
   *                     |       \                  |
   *                     |        \                 |
   *                     |         \                |
   *                  am1(5HD)     am2(NCHW)      am3(Fz)
   *                     |           |              |
   *                     |           |              |
   *                  x1(5HD)     x2(NCHW)      x3(Fz)
   *                     |           |              |
   *                     |           |              |
   *                 Conv2D(5HD)  am4(NCHW)  Conv2D(Fz)
   *                     \           |         /
   *                       \         |        /
   *                         \       |       /
   *                           \     |      /
   *                             \   |     /
   *                              NetOutput
   *
   * am1, am2, am3, x1, x2, x3 are six sgt slicing op, they will be merged into a function op.
   * After merging {am1, am2, am3, x1, x2, x3}, the graph will look like:
   *                   (Const                    Constant)
   *                     |                          |
   *                PlaceHolder0               PlaceHolder1
   *                     |      \                   |
   *                     |       \                  |
   *                     |        \                 |
   *                     |         \                |
   *                     |          \              /
   *                     \           |            /
   *                      \          |          /
   *                        \        |        /
   *                         FunctionOp(5HD,NCHW,Fz)
   *                            /   |     \
   *                          /     |      \
   *                        /       |       \
   *                 Conv2D(5HD)  am4(NCHW)  Conv2D(Fz)
   *                     \           |         /
   *                       \         |        /
   *                         \       |       /
   *                           \     |      /
   *                             \   |     /
   *                              NetOutput
   *
   * Inside the case operator, the subgraph look like:
   *                      Data    Data   Data
   *                         \     |     /
   *                          \    |    /
   *                         am1  am2 am3
   *                            \  |  /
   *                             X X X (three formatAgnosticOp)
   *                             / | \
   *                           NetOutput
   *
   */

  ge::GeShape original_shape = GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT, original_shape);
  string fa = "FormatAgnosticOp";
  test.AddOpDesc("conv2d", CONV2D, 3, 1)
      .AddOpDesc("conv2d1", CONV2D, 2, 1)
      .AddOpDesc("x1", fa)
      .AddOpDesc("x2", fa)
      .AddOpDesc("x3", fa)
      .AddOpDesc("am1", fa, 1, 1)
      .AddOpDesc("am2", fa, 1, 1)
      .AddOpDesc("am3", fa, 1, 1)
      .AddOpDesc("am4", fa, 1, 1)

      .SetInput("conv2d:0", "", ge::FORMAT_NC1HWC0)
      .SetInput("conv2d:1", "", ge::FORMAT_FRACTAL_Z, ge::FORMAT_HWCN)
      .SetInput("conv2d:2", "", {12})

      .SetInput("conv2d1:0", "", ge::FORMAT_NC1HWC0)
      .SetInput("conv2d1:1", "", ge::FORMAT_FRACTAL_Z);

  test.SetInput("am1:0", ge::FORMAT_NC1HWC0, "PlaceHolder_0:0", ge::FORMAT_NC1HWC0)
      .SetInput("am2:0", "PlaceHolder_0:0")
      .SetInput("am3:0", ge::FORMAT_FRACTAL_Z, "PlaceHolder_1:0", ge::FORMAT_FRACTAL_Z);

  test.SetInput("x1:0", ge::FORMAT_NC1HWC0, "am1:0", ge::FORMAT_NC1HWC0)
      .SetInput("x2", "am2")
      .SetInput("x3", ge::FORMAT_FRACTAL_Z, "am3", ge::FORMAT_FRACTAL_Z);

  test.SetInput("conv2d:0", ge::FORMAT_NC1HWC0, "x1:0", ge::FORMAT_NC1HWC0);
  test.SetInput("am4:0", "x2:0");
  test.SetInput("conv2d1:1", ge::FORMAT_FRACTAL_Z, "x3:0", ge::FORMAT_FRACTAL_Z);
  ge::NodePtr am1;
  ge::NodePtr am2;
  ge::NodePtr am3;
  test.GetNodeByName("am1", am1);
  test.GetNodeByName("am2", am2);
  test.GetNodeByName("am3", am3);

  ge::NodePtr x1;
  ge::NodePtr x2;
  ge::NodePtr x3;
  test.GetNodeByName("x1", x1);
  test.GetNodeByName("x2", x2);
  test.GetNodeByName("x3", x3);
  ge::AttrUtils::SetInt(x1->GetOpDesc(), kThreadScopeId, 13);
  ge::AttrUtils::SetInt(x2->GetOpDesc(), kThreadScopeId, 13);
  ge::AttrUtils::SetInt(x3->GetOpDesc(), kThreadScopeId, 13);
  ge::AttrUtils::SetInt(am1->GetOpDesc(), kThreadScopeId, 13);

  ge::AttrUtils::SetInt(am3->GetOpDesc(), kThreadScopeId, 13);
  if (set_thread_scope) {
    ge::AttrUtils::SetInt(am2->GetOpDesc(), kThreadScopeId, 13);
  }

  vector<NodePtr> nodes = {am1, am2, am3, x1, x2, x3};
  FFTSPlusGraphOptimizer ffts_plus;
  std::shared_ptr<fe::GraphComm> graph_comm_ptr = nullptr;
  FFTS_MAKE_SHARED(graph_comm_ptr = std::make_shared<fe::GraphComm>(kFFTSPlusCoreName),
  return fe::FAILED);
  FFTS_CHECK(graph_comm_ptr == nullptr, FFTS_LOGE("graphCommPtr is nullptr."), return fe::FAILED);
  ffts_plus.graph_comm_ptr_ = graph_comm_ptr;

  ge::AttrUtils::SetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, "0_1");

  // Create source graph(the graph above is the sub graph with placeholder and end)
  // In the source graph the input node of am1/am2 is const and am3 is constant
  ComputeGraphPtr src_graph = std::make_shared<ComputeGraph>("src_graph");
  GraphConstructor test2(src_graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT, original_shape);
  test2.AddOpDesc("const", "Const")
       .AddOpDesc("constant", "Constant");
  ge::NodePtr constant_node;
  test2.GetNodeByName("const", const_node);
  test2.GetNodeByName("constant", constant_node);

  graph->SetExtAttr("part_src_graph", src_graph);
  graph->SetParentGraph(src_graph);

  test.GetNodeByName("PlaceHolder_0", place_holder0);
  test.GetNodeByName("PlaceHolder_1", place_holder1);
  place_holder0->GetOpDesc()->SetExtAttr(ATTR_NAME_PARENT_NODE, const_node);
  place_holder1->GetOpDesc()->SetExtAttr(ATTR_NAME_PARENT_NODE, constant_node);
  CacheManager::SetPersistWeightForGraph(*graph);

  ffts_plus.TransSingleSubGraph(*graph.get(), nodes);

  GraphConstructor::DumpGraph(graph);
  auto sub_graph = src_graph->GetSubgraph("test_sgt_graph_0");
  GraphConstructor::DumpGraph(sub_graph);

  SecondGraphPartition(sub_graph, place_holder0, place_holder1, set_thread_scope);

  GraphConstructor::DumpGraph(sub_graph);
  return ffts::SUCCESS;
}

TEST_F(UTEST_CACHE_PERSISTETNT, converage_01) {
  std::unordered_set<string> types = {"a", "ab", "cd"};
  bool matched = false;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<OpDesc>("test", "cd");
  ge::GeTensorDesc t;
  op->AddInputDesc(t);
  ge::NodePtr node = graph->AddNode(op);

  ge::OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  const_op->AddOutputDesc(t);
  ge::NodePtr const_node = graph->AddNode(const_op);

  ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node->GetInDataAnchor(0));

  //FeGraphUtils::IsNodeSpecificType(types, node, matched);
  //EXPECT_EQ(matched, true);

  //ge::NodePtr peer_out_node;
  //bool ret = FeGraphUtils::IsPeerOutConst(peer_out_node.get(), 0, peer_out_node);
  //EXPECT_EQ(ret, false);


  //ret = FeGraphUtils::IsPeerOutConst(node.get(), 0, peer_out_node);
  //EXPECT_EQ(ret, true);
}

// PlaceHolder0 connects to two nodes with same thread scope id
TEST_F(UTEST_CACHE_PERSISTETNT, case_01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr const_node;
  ge::NodePtr place_holder0;
  ge::NodePtr place_holder1;

  CreateGraphOfFunctionOp1(graph, const_node, place_holder0, place_holder1);

  vector<uint32_t> persist_scope_ids;
  vector<uint32_t> expect = {13};
  (void)ge::AttrUtils::GetListInt(const_node->GetOpDesc(), kCachePersistScopeIds, persist_scope_ids);
  EXPECT_EQ(persist_scope_ids, expect);


  /* Set data as placeholder for handling cache persist. */
  /* This is a simplified method. */
  place_holder0->GetOpDesc()->SetType("Const");
  int node_count = 0;
  FFTSPlusGraphOptimizer graph_optimizer;
  graph_optimizer.CacheOptionJudge(*graph.get());
  for (auto &node : graph->GetAllNodes()) {
    CacheManager::HandleCachePersist(node);
  }

  for (auto &node : graph->GetAllNodes()) {
    node_count++;
  }

  for (auto &node : graph->GetAllNodes()) {
    if (node->GetName() == "am1am2am3x1x2x3") {
      uint32_t persist_id = 0;
      int64_t max_cache_size = 0;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kCachePersist, persist_id);
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kCachePersistSize, max_cache_size);
      EXPECT_EQ(persist_id, 5);
      EXPECT_EQ(max_cache_size, 4320);

      domi::TaskDef task_def;
      CachePersistTaskBuilder cp;
      domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

      Status ret = cp.GenContextDef(node, ffts_plus_task_def);
      EXPECT_EQ(ret, fe::SUCCESS);
    }
  }
  EXPECT_EQ(node_count, 19);
}

// PlaceHolder0 connects to only one sgt sliced node, so it
// does not need to do cache persistetent
TEST_F(UTEST_CACHE_PERSISTETNT, case_02) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr const_node;
  ge::NodePtr place_holder0;
  ge::NodePtr place_holder1;

  CreateGraphOfFunctionOp1(graph, const_node, place_holder0, place_holder1, false);

  vector<uint32_t> persist_scope_ids;
  vector<uint32_t> expect = {};
  (void)ge::AttrUtils::GetListInt(const_node->GetOpDesc(), kCachePersistScopeIds, persist_scope_ids);
  EXPECT_EQ(persist_scope_ids, expect);


  /* Set data as placeholder for handling cache persist. */
  /* This is a simplified method. */
  place_holder0->GetOpDesc()->SetType("Const");
  int node_count = 0;
  FFTSPlusGraphOptimizer graph_optimizer;
  graph_optimizer.CacheOptionJudge(*graph.get());
  for (auto &node : graph->GetAllNodes()) {
    CacheManager::HandleCachePersist(node);
  }

  for (auto &node : graph->GetAllNodes()) {
    node_count++;
  }

  for (auto &node : graph->GetAllNodes()) {
    if (node->GetName() == "am1am2am3x1x2x3") {
      uint32_t persist_id = 0;
      int64_t max_cache_size = 0;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kCachePersist, persist_id);
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kCachePersistSize, max_cache_size);
      EXPECT_EQ(persist_id, 0);
      EXPECT_EQ(max_cache_size, 0);

      domi::TaskDef task_def;
      CachePersistTaskBuilder cp;
      domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

      Status ret = cp.GenContextDef(node, ffts_plus_task_def);
      EXPECT_EQ(ret, fe::FAILED);
    }
  }
  EXPECT_EQ(node_count, 19);
}
