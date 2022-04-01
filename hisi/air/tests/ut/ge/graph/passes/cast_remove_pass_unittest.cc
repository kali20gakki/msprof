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
#include <vector>

#define protected public
#define private public
#include "graph/passes/cast_remove_pass.h"
#undef protected
#undef private

#include "anchor.h"
#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/op/attr_value_util.h"
#include "common/types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "inc/pass_manager.h"
#include "graph_builder_utils.h"
#include <string>
#include <iostream>
#include <vector>
#include "opskernel_manager/ops_kernel_manager.h"
#include "omg/omg_inner_types.h"


using namespace testing;
using namespace ge;
using namespace std;

class UtestGraphPassesCastRemovePass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

/*
 *   data1    const1
 *      |       |
 *        add1
 *         |
 *     transdata1
 *         |
 *       cast1
 *         |
 *     transposed1
 *         |
 *       cast2
 *         |
 *     transdata2
 *         |
 *       conv2d
 */
ComputeGraphPtr BuildGraph1() {
  auto builder = ut::GraphBuilder("g1");
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto add1 = builder.AddNode("add1", ADD, 2, 1);
  auto transdata1 = builder.AddNode("transdata1", TRANSDATA, 1, 1);
  auto cast1 = builder.AddNode("cast1", CAST, 1, 1);
  auto transposed1 = builder.AddNode("transposed1", TRANSPOSED, 1, 1);
  auto cast2 = builder.AddNode("cast2", CAST, 1, 1);
  auto transdata2 = builder.AddNode("transdata2", TRANSDATA, 1, 1);
  auto conv2d = builder.AddNode("conv2d", CONV2D, 1, 1);

  builder.AddDataEdge(data1, 0, add1, 0);
  builder.AddDataEdge(const1, 0, add1, 1);
  builder.AddDataEdge(add1, 0, transdata1, 0);
  builder.AddDataEdge(transdata1, 0, cast1, 0);
  builder.AddDataEdge(cast1, 0, transposed1, 0);
  builder.AddDataEdge(transposed1, 0, cast2, 0);
  builder.AddDataEdge(cast2, 0, transdata2, 0);
  builder.AddDataEdge(transdata2, 0, conv2d, 0);

  return builder.GetGraph();
}

TEST_F(UtestGraphPassesCastRemovePass, DoFuseProcess) {
  std::vector<NodePtr> nodes_to_fuse;

  auto builder = ut::GraphBuilder("g1");
  auto data = builder.AddNode("data", DATA, 1, 1);
  auto cast1 = builder.AddNode("cast1", CAST, 1, 1);
  cast1->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT16);
  auto trans = builder.AddNode("trans", TRANSPOSE, 1, 1, FORMAT_NCHW, DT_FLOAT16);
  auto cast2 = builder.AddNode("cast2", CAST, 1, 1);
  cast2->GetOpDesc()->MutableInputDesc(0)->SetDataType(DT_FLOAT16);
  auto net = builder.AddNode("netout", NETOUTPUT, 1, 1);

  builder.AddDataEdge(data, 0, cast1, 0);
  builder.AddDataEdge(cast1, 0, trans, 0);
  builder.AddDataEdge(trans, 0, cast2, 0);
  builder.AddDataEdge(cast2, 0, net, 0);
  ComputeGraphPtr compute_graph = builder.GetGraph();

  map<string, string> options;

  CastRemovePass cast_remove_pass;
  DataType type = DT_FLOAT;
  nodes_to_fuse.emplace_back(cast1);
  nodes_to_fuse.emplace_back(trans);
  nodes_to_fuse.emplace_back(cast2);
  cast_remove_pass.RemoveCast(type, nodes_to_fuse);
  EXPECT_EQ(compute_graph->GetAllNodesSize(),3);
}

TEST_F(UtestGraphPassesCastRemovePass, success) {
  auto compute_graph = BuildGraph1();
  compute_graph->SetSessionID(0);

  auto data1 = compute_graph->FindNode("data1");
  CastRemovePass cast_pass;
  Status ret = cast_pass.Run(data1);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(compute_graph->GetDirectNodesSize(), 9);

  auto add1 = compute_graph->FindNode("add1");
  ret = cast_pass.Run(add1);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(compute_graph->GetDirectNodesSize(), 7);

  auto cast1 = compute_graph->FindNode("cast1");
  EXPECT_EQ(cast1, nullptr);

  auto transposed1 = compute_graph->FindNode("transposed1");
  EXPECT_EQ(transposed1, nullptr);

  NodePtr test_node;
  // test nullptr opdesc
  ret = cast_pass.Run(test_node);
  EXPECT_EQ(ret, PARAM_INVALID);

  test_node = nullptr;
  // test nullptr node
  ret = cast_pass.Run(test_node);
  EXPECT_EQ(ret, PARAM_INVALID);
}