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

#include <string>
#include <vector>
#include <gtest/gtest.h>

#define protected public
#define private public
#include "graph/build/label_allocator.h"
#undef protected
#undef private

#include "compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "utils/graph_utils.h"
#include "graph/passes/graph_builder_utils.h"

#include "framework/common/util.h"
#include "graph/utils/graph_utils.h"
#include "graph/label/label_maker.h"

namespace ge {
class LabelAllocatorTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(LabelAllocatorTest, TestSkipPipelinePartition) {
  auto builder = ut::GraphBuilder("g1");
  auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 0, 1);
  auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);

  auto op_desc = partitioned_call_1->GetOpDesc();
  auto subgraph = std::make_shared<ComputeGraph>("subgraph-1");
  op_desc->AddSubgraphName("subgraph-1");
  op_desc->SetSubgraphInstanceName(0, "subgraph-1");
  subgraph->SetParentNode(partitioned_call_1);

  builder.AddDataEdge(partitioned_call_1, 0, net_output, 0);
  LabelAllocator label_allocator(subgraph);
  std::set<NodePtr> functional_nodes;
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  ASSERT_EQ(functional_nodes.size(), 1);
  ASSERT_EQ(*functional_nodes.begin(), partitioned_call_1);

  auto root_graph = builder.GetGraph();
  functional_nodes.clear();
  AttrUtils::SetBool(root_graph, ATTR_NAME_PIPELINE_PARTITIONED, false);
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  ASSERT_EQ(functional_nodes.size(), 1);
  ASSERT_EQ(*functional_nodes.begin(), partitioned_call_1);

  functional_nodes.clear();
  AttrUtils::SetBool(root_graph, ATTR_NAME_PIPELINE_PARTITIONED, false);
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  ASSERT_EQ(functional_nodes.size(), 1);
  ASSERT_EQ(*functional_nodes.begin(), partitioned_call_1);

  functional_nodes.clear();
  AttrUtils::SetBool(root_graph, ATTR_NAME_PIPELINE_PARTITIONED, true);
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  ASSERT_EQ(functional_nodes.size(), 0);
  std::shared_ptr<ge::ComputeGraph> gp = nullptr;
  ASSERT_FALSE(label_allocator.CollectFunctionalNode(gp, functional_nodes));
  subgraph->SetGraphUnknownFlag(true);
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  subgraph->SetGraphUnknownFlag(false);
  auto node = subgraph->GetParentNode();
  node->GetOpDesc()->SetAttr("_ffts_sub_graph", AnyValue::CreateFrom<std::string>("sub_graph"));
  node->GetOpDesc()->SetAttr("_ffts_plus_sub_graph", AnyValue::CreateFrom<std::string>("plus_sub_graph"));
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
  auto owner = node->GetOwnerComputeGraph();
  owner->SetGraphUnknownFlag(true);
  ASSERT_TRUE(label_allocator.CollectFunctionalNode(subgraph, functional_nodes));
}


TEST_F(LabelAllocatorTest, AssignFunctionalLabels) {
  auto builder = ut::GraphBuilder("g1");
  auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 0, 1);
  auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);

  auto op_desc = partitioned_call_1->GetOpDesc();
  auto subgraph = std::make_shared<ComputeGraph>("subgraph-1");
  op_desc->AddSubgraphName("subgraph-1");
  op_desc->SetSubgraphInstanceName(0, "subgraph-1");
  subgraph->SetParentNode(partitioned_call_1);

  builder.AddDataEdge(partitioned_call_1, 0, net_output, 0);
  LabelAllocator label_allocator(subgraph);
  label_allocator.compute_graph_ = nullptr;
  EXPECT_EQ(label_allocator.AssignFunctionalLabels(), INTERNAL_ERROR);
}
}  // namespace ge