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

#define private public
#include "graph/passes/hccl_group_pass.h"
#include "graph_builder_utils.h"
#include "graph/graph.h"
#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#undef private

using namespace domi;
namespace ge {
class UtestHcclGroupPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

/*
 *             data1
 *           /        \
 *  hcomallreduce1--->hcomallreduce2
 *         |           |
 *        add1        add2
 *         |           |
 *       cast1        cast2
 *         |           |
 *  hcomallreduce3    cast4
 *         |
 *       cast3
 *         |
 *       conv2d
 */
ComputeGraphPtr BuildGraphPass1() {
  auto builder = ut::GraphBuilder("g1");
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto add1 = builder.AddNode("add1", ADD, 2, 1);
  auto add2 = builder.AddNode("add2", ADD, 2, 1);
  auto cast1 = builder.AddNode("cast1", CAST, 1, 1);
  auto cast2 = builder.AddNode("cast2", CAST, 1, 1);
  auto cast3 = builder.AddNode("cast3", CAST, 1, 1);
  auto cast4 = builder.AddNode("cast4", CAST, 1, 1);
  auto hcomallreduce1 = builder.AddNode("hcomallreduce1", HCOMALLREDUCE, 1, 1);
  auto hcomallreduce2 = builder.AddNode("hcomallreduce2", HCOMALLREDUCE, 1, 1);
  auto hcomallreduce3 = builder.AddNode("hcomallreduce3", HCOMALLREDUCE, 1, 1);
  auto conv2d = builder.AddNode("conv2d", CONV2D, 1, 1);

  AttrUtils::SetBool(hcomallreduce1->GetOpDesc(),"_hccl_fused_node",true);
  AttrUtils::SetBool(hcomallreduce2->GetOpDesc(),"_hccl_fused_node",true);
  AttrUtils::SetBool(hcomallreduce3->GetOpDesc(),"_hccl_fused_node",true);

  builder.AddDataEdge(data1, 0, hcomallreduce1, 0);
  builder.AddDataEdge(data1, 0, hcomallreduce2, 0);
  builder.AddDataEdge(hcomallreduce1, 0, add1, 0);
  builder.AddDataEdge(hcomallreduce2, 0, add2, 0);
  builder.AddDataEdge(add1, 0, cast1, 0);
  builder.AddDataEdge(add2, 0, cast2, 0);
  builder.AddDataEdge(cast2, 0, cast4, 0);
  builder.AddDataEdge(cast1, 0, hcomallreduce3, 0);
  builder.AddDataEdge(hcomallreduce3, 0, cast3, 0);
  builder.AddDataEdge(cast3, 0, conv2d, 0);
  builder.AddDataEdge(cast3, 0, conv2d, 0);
  builder.AddControlEdge(hcomallreduce1, hcomallreduce2);

  return builder.GetGraph();
}


/*
 *                 data1
 *           /               \
 *  hcomallreduce1   ----->    hcomallreduce2
 *         \                     |
 *            add1 -const       cast2
 *          |                     |
 *       cast1                  conv2d2
 *         |
 *       conv2d
 */

ComputeGraphPtr BuildGraphPass2() {
  auto builder = ut::GraphBuilder("g1");
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto data2 = builder.AddNode("data2", DATA, 0, 1);
  auto const1 = builder.AddNode("const", CONSTANT, 0, 1);
  auto add1 = builder.AddNode("add1", ADD, 2, 1);
  auto cast1 = builder.AddNode("cast1", CAST, 1, 1);
  auto cast2 = builder.AddNode("cast2", CAST, 1, 1);
  auto hcomallreduce1 = builder.AddNode("hcomallreduce1", HCOMALLREDUCE, 1, 1);
  auto hcomallreduce2 = builder.AddNode("hcomallreduce2", HCOMALLREDUCE, 1, 1);
  auto conv2d = builder.AddNode("conv2d", CONV2D, 1, 1);
  auto conv2d2 = builder.AddNode("conv2d2", CONV2D, 1, 1);

  AttrUtils::SetBool(hcomallreduce1->GetOpDesc(),"_hccl_fused_node",true);
  AttrUtils::SetBool(hcomallreduce2->GetOpDesc(),"_hccl_fused_node",true);
  builder.AddDataEdge(data1, 0, hcomallreduce1, 0);
  builder.AddDataEdge(data1, 0, hcomallreduce2, 0);
  builder.AddDataEdge(hcomallreduce1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add1, 1);
  builder.AddDataEdge(add1, 0, cast1, 0);
  builder.AddDataEdge(cast1, 0, conv2d, 0);
  builder.AddDataEdge(hcomallreduce2, 0, cast2, 0);
  builder.AddDataEdge(cast2, 0, conv2d2, 0);
  builder.AddControlEdge(hcomallreduce1, hcomallreduce2);
  return builder.GetGraph();
}

/*
 *            data1
 *           /     \
 *  hcomallreduce1   hcomallreduce2
 *         |           |
 *        add1        add2
 *         |           |
 *       cast1        cast2
 *         |           |
 *  hcomallreduce3    cast4
 *         |
 *       cast3
 *         |
 *       conv2d
 */
TEST_F(UtestHcclGroupPass, success) {
  auto g2 = BuildGraphPass2();
  g2->SetSessionID(0);
  HcclGroupPass hccl_group_pass;
  // check after run pass on data1
  auto hcomallreduce1 = g2->FindNode("hcomallreduce1");
  Status ret = hccl_group_pass.Run(hcomallreduce1);
  EXPECT_EQ(ret, SUCCESS);

  auto hcomallreduce2 = g2->FindNode("hcomallreduce2");
  ret = hccl_group_pass.Run(hcomallreduce2);
  EXPECT_EQ(ret, SUCCESS);

  vector<string> check_node_name_vec = {"data1","hcomallreduce1","add1","cast1","hcomallreduce3","cast3","conv2d","hcomallreduce2","add2","cast2"};
  vector<string> exp_group_id_vec_1 = {"","hcomallreduce1","hcomallreduce1","hcomallreduce1","","","","","",""};
  vector<string> exp_group_id_vec_2 = {"","hcomallreduce1","hcomallreduce1","hcomallreduce1","","","","hcomallreduce2","hcomallreduce2","hcomallreduce2"};
  auto compute_graph = BuildGraphPass1();
  compute_graph->SetSessionID(0);

  auto data1 = compute_graph->FindNode("data1");
  hccl_group_pass;
  ret = hccl_group_pass.Run(data1);
  EXPECT_EQ(ret, SUCCESS);
  // check after run pass on data1
  hcomallreduce1 = compute_graph->FindNode("hcomallreduce1");
  string group_id;
  EXPECT_EQ(false,AttrUtils::GetStr(hcomallreduce1->GetOpDesc(),"_hccl_fused_group",group_id));
  ret = hccl_group_pass.Run(hcomallreduce1);
  EXPECT_EQ(ret, SUCCESS);
  // check after run pass on hcomallreduce1
  NodePtr temp_node;
  hcomallreduce2 = compute_graph->FindNode("hcomallreduce2");
  ret = hccl_group_pass.Run(hcomallreduce2);
  EXPECT_EQ(ret, SUCCESS);
  // check after run pass on hcomallreduce2
  auto hcomallreduce3 = compute_graph->FindNode("hcomallreduce3");
  ret = hccl_group_pass.Run(hcomallreduce3);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestHcclGroupPass, node_is_null) {
  HcclGroupPass hccl_group_pass;
  // check after run pass on data1
  NodePtr tmp_node;
  Status ret = hccl_group_pass.Run(tmp_node);
  EXPECT_EQ(ret, PARAM_INVALID);
}

}