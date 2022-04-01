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

#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#include "common/util.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/passes/graph_builder_utils.h"
#include "framework/omg/omg_inner_types.h"
#define protected public
#define private public
#include "compute_graph.h"
#include "graph/compute_graph_impl.h"
#include "framework/common/types.h"
#include "utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "common/omg_util.h"
#include <gmock/gmock.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/tensor_utils.h"
#include "graph/operator_factory.h"
#include "graph/operator_reg.h"
#include "graph/partition/stage_partition.h"
#undef private
#undef protected

namespace ge {
namespace {
GeTensorDescPtr CreateTensorDesc(std::initializer_list<int64_t> shape, Format format = FORMAT_NCHW,
                                 DataType data_type = DT_FLOAT) {
  GeShape ge_shape{vector<int64_t>(shape)};
  GeTensorDescPtr tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(ge_shape);
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  return tensor_desc;
}

class NodeBuilder {
 public:
  NodeBuilder(const std::string &name, const std::string &type) { op_desc_ = std::make_shared<OpDesc>(name, type); }

  NodeBuilder &AddInputDesc(std::initializer_list<int64_t> shape = {1, 1, 224, 224}, Format format = FORMAT_NCHW,
                            DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(std::initializer_list<int64_t> shape = {1, 1, 224, 224}, Format format = FORMAT_NCHW,
                             DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(GeTensorDescPtr tensor_desc) {
    op_desc_->AddOutputDesc(tensor_desc->Clone());
    return *this;
  }

  NodePtr Build(const ComputeGraphPtr &graph) {
    NodePtr node = graph->AddNode(op_desc_);
    return node;
  }

 private:
  OpDescPtr op_desc_;
};
}  // namespace

class UtestStagePartition : public testing::Test {
  protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(UtestStagePartition, partition_test) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  StagePartitioner stagePartitioner(graph);
  auto ret = stagePartitioner.SplitStageLevel();
  ASSERT_EQ(ret, SUCCESS);
  std::map<uint32_t, std::set<NodePtr>> stage_nodes;
  map<string, vector<int64_t>> shape_map;
  ge::NodePtr data0 = NodeBuilder("data1", DATA)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .Build(graph);
  std::set<NodePtr> node_ptr_set;
  node_ptr_set.insert(data0);             
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{1,224,224,3}), FORMAT_NHWC, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NHWC, DT_INT64));
  node_ptr_set.insert(aipp);  
  stage_nodes.insert(std::pair<uint32_t, std::set<NodePtr>>(1,node_ptr_set));
  stagePartitioner.stage_nodes_ = stage_nodes;
  ret = stagePartitioner.SplitStageLevel();
  ASSERT_EQ(ret, SUCCESS);
}

TEST_F(UtestStagePartition, stage_partition_test) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  StagePartitioner stagePartitioner(graph);
  auto ret = stagePartitioner.StagePartition();
  ASSERT_EQ(ret, SUCCESS);
  std::map<uint32_t, std::set<NodePtr>> stage_nodes;
  map<string, vector<int64_t>> shape_map;
  ge::NodePtr data0 = NodeBuilder("data1", DATA)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .Build(graph);
  std::set<NodePtr> node_ptr_set;
  node_ptr_set.insert(data0);             
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{1,224,224,3}), FORMAT_NHWC, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NHWC, DT_INT64));
  node_ptr_set.insert(aipp);  
  stage_nodes.insert(std::pair<uint32_t, std::set<NodePtr>>(1,node_ptr_set));
  stagePartitioner.stage_nodes_ = stage_nodes;           
  ret = stagePartitioner.StagePartition();
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestStagePartition, partition_test_2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  StagePartitioner stagePartitioner(graph);
  auto ret = stagePartitioner.Partition();
  ASSERT_EQ(ret, SUCCESS);
  std::map<uint32_t, std::set<NodePtr>> stage_nodes;
  map<string, vector<int64_t>> shape_map;
  ge::NodePtr data0 = NodeBuilder("data1", DATA)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                .Build(graph);
  std::set<NodePtr> node_ptr_set;
  node_ptr_set.insert(data0);             
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{1,224,224,3}), FORMAT_NHWC, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NHWC, DT_INT64));
  node_ptr_set.insert(aipp);  
  stage_nodes.insert(std::pair<uint32_t, std::set<NodePtr>>(1,node_ptr_set));
  stagePartitioner.stage_nodes_ = stage_nodes;
  stagePartitioner.root_graph_ = computeGraph;
  ret = stagePartitioner.Partition();
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestStagePartition, Partition) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("node1", CONSTANT, 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  StagePartitioner stagePartitioner(computeGraph);
  AttrUtils::SetInt(data1->GetOpDesc(), "_stage_level", 1);

  EXPECT_EQ(stagePartitioner.Partition(), FAILED);
}

} // namespace ge