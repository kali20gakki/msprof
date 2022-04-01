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

#include "compute_graph.h"
#include "graph/compute_graph_impl.h"
#include "framework/common/types.h"
#include "utils/graph_utils.h"
#include "graph/passes/graph_builder_utils.h"

#define protected public
#define private public
#include "common/model/model_relation.h"
#undef private
#undef protected
#include "graph/debug/ge_attr_define.h"

namespace ge {
class ModelRelationTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}

 protected:

  void SetSubGraph(OpDesc &op_desc, const std::string &name) {
    auto subgraph = std::make_shared<ComputeGraph>(name);
    op_desc.AddSubgraphName(name);
    op_desc.SetSubgraphInstanceName(0, name);
  }

  ComputeGraphPtr BuildGraph() {
    auto builder = ut::GraphBuilder("g1");
    auto data1 = builder.AddNode("data1", DATA, 0, 1);
    auto data2 = builder.AddNode("data2", DATA, 0, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
    AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
    auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 2, 1);
    auto partitioned_call_2 = builder.AddNode("PartitionedCall2", PARTITIONEDCALL, 2, 1);
    auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);

    SetSubGraph(*partitioned_call_1->GetOpDesc(), "subgraph-1");
    SetSubGraph(*partitioned_call_2->GetOpDesc(), "subgraph-2");

    builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
    builder.AddDataEdge(data2, 0, partitioned_call_1, 1);
    builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_2, 0);
    builder.AddDataEdge(data2, 0, partitioned_call_2, 1);
    builder.AddDataEdge(partitioned_call_2, 0, net_output, 0);
    return builder.GetGraph();
  }

  ComputeGraphPtr BuildGraphWithNoInput() {
    auto builder = ut::GraphBuilder("g1");
    auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 0, 1);
    auto partitioned_call_2 = builder.AddNode("PartitionedCall2", PARTITIONEDCALL, 1, 1);
    auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);

    SetSubGraph(*partitioned_call_1->GetOpDesc(), "subgraph-1");
    SetSubGraph(*partitioned_call_2->GetOpDesc(), "subgraph-2");

    builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_2, 0);
    builder.AddDataEdge(partitioned_call_2, 0, net_output, 0);
    return builder.GetGraph();
  }

  ComputeGraphPtr BuildGraphWithManyToOnePartitionedCalls() {
    auto builder = ut::GraphBuilder("g1");
    auto data1 = builder.AddNode("data1", DATA, 0, 1);
    auto data2 = builder.AddNode("data2", DATA, 0, 1);
    AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
    AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
    auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 1, 1);
    auto partitioned_call_2 = builder.AddNode("PartitionedCall2", PARTITIONEDCALL, 1, 1);
    auto partitioned_call_3 = builder.AddNode("PartitionedCall3", PARTITIONEDCALL, 2, 1);
    auto net_output = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);

    SetSubGraph(*partitioned_call_1->GetOpDesc(), "subgraph-1");
    SetSubGraph(*partitioned_call_2->GetOpDesc(), "subgraph-2");
    SetSubGraph(*partitioned_call_3->GetOpDesc(), "subgraph-3");

    builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
    builder.AddDataEdge(data2, 0, partitioned_call_2, 0);
    builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_3, 0);
    builder.AddDataEdge(partitioned_call_2, 0, partitioned_call_3, 1);
    builder.AddDataEdge(partitioned_call_3, 0, net_output, 0);
    return builder.GetGraph();
  }
};

///      NetOutput
///         |
///         |
///        PC_2
///        |  \
///       PC_1 |
///     /     \
///    |      |
///  data1  data2
TEST_F(ModelRelationTest, TestBuildFromRootGraph) {
  ComputeGraphPtr graph = BuildGraph();
  std::unique_ptr<ModelRelation> model_relation;
  auto ret = ModelRelationBuilder().BuildFromRootGraph(*graph, model_relation);
  ASSERT_EQ(ret, SUCCESS);
  auto queue_defs = model_relation->queue_defs;
  ASSERT_EQ(queue_defs.size(), 4);
  EXPECT_EQ(model_relation->root_model_queue_info.input_queue_names.size(), 2);
  EXPECT_EQ(model_relation->root_model_queue_info.output_queue_names.size(), 1);
  EXPECT_EQ(model_relation->submodel_queue_infos.size(), 2);
}

///      NetOutput
///         |
///        PC_2
///        |
///       PC_1
TEST_F(ModelRelationTest, TestBuildFromRootGraphWithNoInput) {
  ComputeGraphPtr graph = BuildGraphWithNoInput();
  std::unique_ptr<ModelRelation> model_relation;
  auto ret = ModelRelationBuilder().BuildFromRootGraph(*graph, model_relation);
  ASSERT_EQ(ret, SUCCESS);
  auto queue_defs = model_relation->queue_defs;
  ASSERT_EQ(queue_defs.size(), 2);
  EXPECT_EQ(model_relation->root_model_queue_info.input_queue_names.size(), 0);
  EXPECT_EQ(model_relation->root_model_queue_info.output_queue_names.size(), 1);
  EXPECT_EQ(model_relation->submodel_queue_infos.size(), 2);
}

///     NetOutput
///         |
///       PC_3
///      /   \
///    PC_1  PC2
///    |      |
///  data1  data2
TEST_F(ModelRelationTest, TestBuildFromRootGraphWithManyToOnePartitionedCalls) {
  ComputeGraphPtr graph = BuildGraphWithManyToOnePartitionedCalls();
  std::unique_ptr<ModelRelation> model_relation;
  auto ret = ModelRelationBuilder().BuildFromRootGraph(*graph, model_relation);
  ASSERT_EQ(ret, SUCCESS);
  auto queue_defs = model_relation->queue_defs;
  ASSERT_EQ(queue_defs.size(), 5);
  EXPECT_EQ(model_relation->root_model_queue_info.input_queue_names.size(), 2);
  EXPECT_EQ(model_relation->root_model_queue_info.output_queue_names.size(), 1);
  EXPECT_EQ(model_relation->submodel_queue_infos.size(), 3);
}
}  // namespace ge