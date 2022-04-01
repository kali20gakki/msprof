/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/fusion_common/graph_node_map_util.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class GRAPH_NODE_MAP_UTIL_STEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(GRAPH_NODE_MAP_UTIL_STEST, ReCreateNodeTypeMapInGraph_suc1) {
  GraphNodeMapUtil graph_node_map_util;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);
  Status ret = graph_node_map_util.ReCreateNodeTypeMapInGraph(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}