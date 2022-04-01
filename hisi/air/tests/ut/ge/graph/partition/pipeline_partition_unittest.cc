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

#define private public
#define protected public
#include "graph/partition/pipeline_partition.h"
#include "compute_graph.h"
#include "graph/compute_graph_impl.h"
#include "utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "common/omg_util.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/manager/graph_manager.h"

namespace {
    const char *const ATTR_OUTPUT_PIPELINE = "_output_pipeline";  // just for compile, need remove it and add it in metadef!
}

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

class UtestPipelinePartition : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

/*********************************************************************************************************************
 *              Data1   Data2   Data3
 *                 \     /  \    /
 *                  \   /    \  /
 *                  *Add1*   Add2
 *                      \     /
 *                       \   /
 *                        Add3
 *                          |
 *                      NETOUTPUT
*********************************************************************************************************************/
TEST_F(UtestPipelinePartition, pipeline_partition_test1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_data2", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_data3", DATA).AddOutputDesc({10}).Build(graph);

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));

  (void)AttrUtils::SetStr(node_add1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline1");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}


/*********************************************************************************************************************
 *              Data1   Const2   Data3
 *                 \     /  \    /
 *                  \   /    \  /
 *                  *Add1*   Add2
 *                      \     /
 *                       \   /
 *                        Add3
 *                          |
 *                      NETOUTPUT
*********************************************************************************************************************/
TEST_F(UtestPipelinePartition, pipeline_partition_test2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_data3", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));

  (void)AttrUtils::SetStr(node_add1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}

/*********************************************************************************************************************
 *              Data1   Const2  Const3  Data4
 *                 \     /  \    /  \     /
 *                  \   /    \  /    \   /
 *                  *Add1*  *Add2*    Add3
 *                      \     / \      /
 *                       \   /   \    /
 *                       *Add4*   Add5
 *                          \      /
 *                           \    /
 *                          NETOUTPUT
*********************************************************************************************************************/
TEST_F(UtestPipelinePartition, pipeline_partition_test3) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_const3", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input4 = NodeBuilder("input_data4", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});
  float_t weight1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc1(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor1 = std::make_shared<GeTensor>(weight_desc1, (uint8_t *)weight1, sizeof(weight1));
  OpDescUtils::SetWeights(node_input3, {tensor1});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add4 = NodeBuilder("Add4", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add5 = NodeBuilder("Add5", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input4->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));

  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add4->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add4->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add5->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_add5->GetInDataAnchor(1));

  GraphUtils::AddEdge(node_add4->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add5->GetOutDataAnchor(0), node_output->GetInDataAnchor(1));

  (void)AttrUtils::SetStr(node_add1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");
  (void)AttrUtils::SetStr(node_add2->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_2");
  (void)AttrUtils::SetStr(node_add4->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_3");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}


/*********************************************************************************************************************
 *              Data1   Const2   Data3                          PartitonedCall Subgraph
 *                 \     /  \    /
 *                  \   /    \  /                                     Data     Data
 *                  *Add1*  PartitionedCall                             \       /
 *                      \     /                                          \     /
 *                       \   /                                           NETOUTPUT
 *                      NETOUTPUT
*********************************************************************************************************************/
TEST_F(UtestPipelinePartition, pipeline_partition_test4) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_data3", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_partitionedcall1 = NodeBuilder("PartitionedCall1", PARTITIONEDCALL).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  (void)node_partitionedcall1->GetOpDesc()->SetAttr(ATTR_STAGE_LEVEL, GeAttrValue());

  ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("subgraph");
  auto sub_graph_node_input1 = NodeBuilder("sub_graph_node_data1", DATA).AddOutputDesc({10}).Build(sub_graph);
  auto sub_graph_node_input2 = NodeBuilder("sub_graph_node_data2", DATA).AddOutputDesc({10}).Build(sub_graph);
  auto sub_graph_node_output = NodeBuilder("sub_graph_node_netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(sub_graph);
  GraphUtils::AddEdge(sub_graph_node_input1->GetOutDataAnchor(0), sub_graph_node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(sub_graph_node_input2->GetOutDataAnchor(0), sub_graph_node_output->GetInDataAnchor(1));
  sub_graph->SetParentGraph(graph);
  sub_graph->SetParentNode(node_partitionedcall1);
  node_partitionedcall1->GetOpDesc()->AddSubgraphName("subgraph");
  node_partitionedcall1->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph");
  graph->AddSubgraph("subgraph", sub_graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_partitionedcall1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_partitionedcall1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_partitionedcall1->GetOutDataAnchor(0), node_output->GetInDataAnchor(1));

  (void)AttrUtils::SetStr(node_partitionedcall1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}

TEST_F(UtestPipelinePartition, pipeline_partition_test5) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto q = NodeBuilder("q", DATA).AddOutputDesc({1}).Build(graph);
  auto buckets_base_distance = NodeBuilder("buckets_base_distance", DATA).AddOutputDesc({1}).Build(graph);
  auto bucket_list = NodeBuilder("bucket_list", DATA).AddOutputDesc({1}).Build(graph);
  auto ifv_offsets = NodeBuilder("ifv_offsets", DATA).AddOutputDesc({1}).Build(graph);
  auto ifv_counts = NodeBuilder("ifv_counts", DATA).AddOutputDesc({1}).Build(graph);
  auto CalcBestLimit = NodeBuilder("CalcBestLimit", ADDN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemConcatConcat = NodeBuilder("MemConcatConcat", CONCAT).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);

  GraphUtils::AddEdge(q->GetOutDataAnchor(0), MemConcatConcat->GetInDataAnchor(0));
  GraphUtils::AddEdge(buckets_base_distance->GetOutDataAnchor(0), MemConcatConcat->GetInDataAnchor(1));
  GraphUtils::AddEdge(bucket_list->GetOutDataAnchor(0), MemConcatConcat->GetInDataAnchor(2));
  GraphUtils::AddEdge(bucket_list->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(0));
  GraphUtils::AddEdge(ifv_offsets->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(1));
  GraphUtils::AddEdge(ifv_counts->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(2));
  GraphUtils::AddEdge(CalcBestLimit->GetOutDataAnchor(0), MemConcatConcat->GetInDataAnchor(3));

  auto ifv = NodeBuilder("ifv", DATA).AddOutputDesc({1}).Build(graph);
  auto DeQSplit1 = NodeBuilder("DeQSplit1", SPLIT).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto mabiao = NodeBuilder("mabiao", DATA).AddOutputDesc({1}).Build(graph);
  auto zhixin = NodeBuilder("zhixin", DATA).AddOutputDesc({1}).Build(graph);
  auto DeQSplit2 = NodeBuilder("DeQSplit2", SPLIT).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto ScanPQCodes1 = NodeBuilder("ScanPQCodes1", SHAPEN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto GenADC1 = NodeBuilder("GenADC1", ADDN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto ScanPQCodes2 = NodeBuilder("ScanPQCodes2", SHAPEN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto GenADC2 = NodeBuilder("GenADC2", ADDN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemConcat1 = NodeBuilder("MemConcat1", CONCAT).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemConcat2 = NodeBuilder("MemConcat2", CONCAT).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemConcat3 = NodeBuilder("MemConcat3", CONCAT).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);

  GraphUtils::AddEdge(MemConcatConcat->GetOutDataAnchor(0), DeQSplit1->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemConcatConcat->GetOutDataAnchor(0), DeQSplit2->GetInDataAnchor(0));
  GraphUtils::AddEdge(ifv->GetOutDataAnchor(0), ScanPQCodes1->GetInDataAnchor(0));
  GraphUtils::AddEdge(ifv->GetOutDataAnchor(0), ScanPQCodes2->GetInDataAnchor(0));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(0), ScanPQCodes1->GetInDataAnchor(1));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(1), ScanPQCodes1->GetInDataAnchor(2));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(2), ScanPQCodes1->GetInDataAnchor(3));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(3), ScanPQCodes1->GetInDataAnchor(4));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(4), GenADC1->GetInDataAnchor(0));
  GraphUtils::AddEdge(DeQSplit1->GetOutDataAnchor(5), GenADC1->GetInDataAnchor(1));
  GraphUtils::AddEdge(mabiao->GetOutDataAnchor(0), GenADC1->GetInDataAnchor(2));
  GraphUtils::AddEdge(mabiao->GetOutDataAnchor(0), GenADC2->GetInDataAnchor(2));
  GraphUtils::AddEdge(zhixin->GetOutDataAnchor(0), GenADC1->GetInDataAnchor(3));
  GraphUtils::AddEdge(zhixin->GetOutDataAnchor(0), GenADC2->GetInDataAnchor(3));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(0), ScanPQCodes2->GetInDataAnchor(1));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(1), ScanPQCodes2->GetInDataAnchor(2));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(2), ScanPQCodes2->GetInDataAnchor(3));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(3), ScanPQCodes2->GetInDataAnchor(4));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(4), GenADC2->GetInDataAnchor(0));
  GraphUtils::AddEdge(DeQSplit2->GetOutDataAnchor(5), GenADC2->GetInDataAnchor(1));
  GraphUtils::AddEdge(GenADC1->GetOutDataAnchor(0), ScanPQCodes1->GetInDataAnchor(5));
  GraphUtils::AddEdge(GenADC2->GetOutDataAnchor(0), ScanPQCodes2->GetInDataAnchor(5));
  GraphUtils::AddEdge(ScanPQCodes1->GetOutDataAnchor(0), MemConcat1->GetInDataAnchor(0));
  GraphUtils::AddEdge(ScanPQCodes1->GetOutDataAnchor(1), MemConcat1->GetInDataAnchor(1));
  GraphUtils::AddEdge(ScanPQCodes1->GetOutDataAnchor(2), MemConcat1->GetInDataAnchor(2));
  GraphUtils::AddEdge(ScanPQCodes1->GetOutDataAnchor(3), MemConcat1->GetInDataAnchor(3));
  GraphUtils::AddEdge(ScanPQCodes1->GetOutDataAnchor(4), MemConcat1->GetInDataAnchor(4));
  GraphUtils::AddEdge(ScanPQCodes2->GetOutDataAnchor(0), MemConcat2->GetInDataAnchor(0));
  GraphUtils::AddEdge(ScanPQCodes2->GetOutDataAnchor(1), MemConcat2->GetInDataAnchor(1));
  GraphUtils::AddEdge(ScanPQCodes2->GetOutDataAnchor(2), MemConcat2->GetInDataAnchor(2));
  GraphUtils::AddEdge(ScanPQCodes2->GetOutDataAnchor(3), MemConcat2->GetInDataAnchor(3));
  GraphUtils::AddEdge(ScanPQCodes2->GetOutDataAnchor(4), MemConcat2->GetInDataAnchor(4));
  GraphUtils::AddEdge(MemConcat1->GetOutDataAnchor(0), MemConcat3->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemConcat2->GetOutDataAnchor(0), MemConcat3->GetInDataAnchor(1));

  auto MemSplit1 = NodeBuilder("MemSplit1", SPLIT).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemSplit2 = NodeBuilder("MemSplit2", SPLIT).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto MemSplit3 = NodeBuilder("MemSplit3", SPLIT).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto TopKPQDistance = NodeBuilder("TopKPQDistance", SHAPEN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto output = NodeBuilder("output", NETOUTPUT).AddInputDesc({1}).AddOutputDesc({1}).AddInputDesc({1}).Build(graph);

  GraphUtils::AddEdge(MemConcat3->GetOutDataAnchor(0), MemSplit1->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemSplit1->GetOutDataAnchor(0), MemSplit2->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemSplit1->GetOutDataAnchor(1), MemSplit3->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemSplit2->GetOutDataAnchor(0), TopKPQDistance->GetInDataAnchor(0));
  GraphUtils::AddEdge(MemSplit2->GetOutDataAnchor(1), TopKPQDistance->GetInDataAnchor(1));
  GraphUtils::AddEdge(MemSplit2->GetOutDataAnchor(2), TopKPQDistance->GetInDataAnchor(2));
  GraphUtils::AddEdge(MemSplit2->GetOutDataAnchor(3), TopKPQDistance->GetInDataAnchor(3));
  GraphUtils::AddEdge(MemSplit2->GetOutDataAnchor(4), TopKPQDistance->GetInDataAnchor(4));
  GraphUtils::AddEdge(MemSplit3->GetOutDataAnchor(0), TopKPQDistance->GetInDataAnchor(5));
  GraphUtils::AddEdge(MemSplit3->GetOutDataAnchor(1), TopKPQDistance->GetInDataAnchor(6));
  GraphUtils::AddEdge(MemSplit3->GetOutDataAnchor(2), TopKPQDistance->GetInDataAnchor(7));
  GraphUtils::AddEdge(MemSplit3->GetOutDataAnchor(3), TopKPQDistance->GetInDataAnchor(8));
  GraphUtils::AddEdge(MemSplit3->GetOutDataAnchor(4), TopKPQDistance->GetInDataAnchor(9));
  GraphUtils::AddEdge(TopKPQDistance->GetOutDataAnchor(0), output->GetInDataAnchor(0));
  GraphUtils::AddEdge(TopKPQDistance->GetOutDataAnchor(1), output->GetInDataAnchor(1));
  GraphUtils::AddEdge(TopKPQDistance->GetOutDataAnchor(2), output->GetInDataAnchor(2));

  (void)AttrUtils::SetStr(MemConcatConcat->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");
  (void)AttrUtils::SetStr(MemConcat3->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_2");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}

TEST_F(UtestPipelinePartition, pipeline_partition_test6) {
ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");

auto buckets_list = NodeBuilder("bucket_list", DATA).AddOutputDesc({1}).Build(graph);
auto ifv_offsets = NodeBuilder("ifv_offsets", CONSTANT).AddOutputDesc({1}).Build(graph);
auto ifv_counts = NodeBuilder("ifv_counts", CONSTANT).AddOutputDesc({1}).Build(graph);
auto CalcBestLimit = NodeBuilder("CalcBestLimit", ADDN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);

GraphUtils::AddEdge(buckets_list->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(0));
GraphUtils::AddEdge(ifv_offsets->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(1));
GraphUtils::AddEdge(ifv_counts->GetOutDataAnchor(0), CalcBestLimit->GetInDataAnchor(2));

auto buckets_base_distance = NodeBuilder("buckets_base_distance", DATA).AddOutputDesc({1}).Build(graph);
auto ScanPQCodes_AIC = NodeBuilder("ScanPQCode_AIC", SHAPEN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
auto ifv = NodeBuilder("ifv", CONSTANT).AddOutputDesc({1}).Build(graph);
auto ScanPQCodes_AIV = NodeBuilder("ScanPQCode_AIV", SHAPEN).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
auto q = NodeBuilder("q", DATA).AddOutputDesc({1}).Build(graph);
auto GenADC = NodeBuilder("GenADC", SPLIT).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).AddOutputDesc({1}).Build(graph);
auto mabiao = NodeBuilder("mabiao", CONSTANT).AddOutputDesc({1}).Build(graph);
auto zhixin = NodeBuilder("zhixin", CONSTANT).AddOutputDesc({1}).Build(graph);

GraphUtils::AddEdge(buckets_list->GetOutDataAnchor(0), ScanPQCodes_AIC->GetInDataAnchor(1));
GraphUtils::AddEdge(buckets_list->GetOutDataAnchor(0), ScanPQCodes_AIV->GetInDataAnchor(1));
GraphUtils::AddEdge(CalcBestLimit->GetOutDataAnchor(0), ScanPQCodes_AIC->GetInDataAnchor(2));
GraphUtils::AddEdge(CalcBestLimit->GetOutDataAnchor(0), ScanPQCodes_AIV->GetInDataAnchor(2));
GraphUtils::AddEdge(CalcBestLimit->GetOutDataAnchor(1), ScanPQCodes_AIC->GetInDataAnchor(3));
GraphUtils::AddEdge(CalcBestLimit->GetOutDataAnchor(1), ScanPQCodes_AIV->GetInDataAnchor(3));
GraphUtils::AddEdge(buckets_base_distance->GetOutDataAnchor(0), ScanPQCodes_AIC->GetInDataAnchor(0));
GraphUtils::AddEdge(buckets_base_distance->GetOutDataAnchor(0), ScanPQCodes_AIV->GetInDataAnchor(0));
GraphUtils::AddEdge(q->GetOutDataAnchor(0), GenADC->GetInDataAnchor(0));
GraphUtils::AddEdge(mabiao->GetOutDataAnchor(0), GenADC->GetInDataAnchor(1));
GraphUtils::AddEdge(zhixin->GetOutDataAnchor(0), GenADC->GetInDataAnchor(2));
GraphUtils::AddEdge(GenADC->GetOutDataAnchor(0), ScanPQCodes_AIC->GetInDataAnchor(4));
GraphUtils::AddEdge(GenADC->GetOutDataAnchor(1), ScanPQCodes_AIV->GetInDataAnchor(4));
GraphUtils::AddEdge(ifv->GetOutDataAnchor(0), ScanPQCodes_AIC->GetInDataAnchor(5));
GraphUtils::AddEdge(ifv->GetOutDataAnchor(0), ScanPQCodes_AIV->GetInDataAnchor(5));

auto TOPKPQDistance = NodeBuilder("TOPKPQDistance", CONCAT).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
auto netoutput = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({1}).Build(graph);


GraphUtils::AddEdge(ScanPQCodes_AIC->GetOutDataAnchor(0), TOPKPQDistance->GetInDataAnchor(0));
GraphUtils::AddEdge(ScanPQCodes_AIC->GetOutDataAnchor(1), TOPKPQDistance->GetInDataAnchor(1));
GraphUtils::AddEdge(ScanPQCodes_AIC->GetOutDataAnchor(2), TOPKPQDistance->GetInDataAnchor(2));
GraphUtils::AddEdge(ScanPQCodes_AIC->GetOutDataAnchor(3), TOPKPQDistance->GetInDataAnchor(3));
GraphUtils::AddEdge(ScanPQCodes_AIC->GetOutDataAnchor(4), TOPKPQDistance->GetInDataAnchor(4));
GraphUtils::AddEdge(ScanPQCodes_AIV->GetOutDataAnchor(0), TOPKPQDistance->GetInDataAnchor(5));
GraphUtils::AddEdge(ScanPQCodes_AIV->GetOutDataAnchor(1), TOPKPQDistance->GetInDataAnchor(6));
GraphUtils::AddEdge(ScanPQCodes_AIV->GetOutDataAnchor(2), TOPKPQDistance->GetInDataAnchor(7));
GraphUtils::AddEdge(ScanPQCodes_AIV->GetOutDataAnchor(3), TOPKPQDistance->GetInDataAnchor(8));
GraphUtils::AddEdge(ScanPQCodes_AIV->GetOutDataAnchor(4), TOPKPQDistance->GetInDataAnchor(9));

GraphUtils::AddEdge(TOPKPQDistance->GetOutDataAnchor(0), netoutput->GetInDataAnchor(0));

(void)AttrUtils::SetStr(CalcBestLimit->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");
(void)AttrUtils::SetStr(ScanPQCodes_AIC->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_2");

PipelinePartitioner pipeline_partitioner(graph);
EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}



/*TEST_F(UtestPipelinePartition, pipeline_and_dynamic_shape_partition_success7) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_const3", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input4 = NodeBuilder("input_data4", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});
  float_t weight1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc1(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor1 = std::make_shared<GeTensor>(weight_desc1, (uint8_t *)weight1, sizeof(weight1));
  OpDescUtils::SetWeights(node_input3, {tensor1});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add4 = NodeBuilder("Add4", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add5 = NodeBuilder("Add5", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input4->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));

  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add4->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add4->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add5->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_add5->GetInDataAnchor(1));

  GraphUtils::AddEdge(node_add4->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add5->GetOutDataAnchor(0), node_output->GetInDataAnchor(1));

  bool is_pipeline_needed = true;
  (void)AttrUtils::SetBool(node_add1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);
  (void)AttrUtils::SetBool(node_add2->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);
  (void)AttrUtils::SetBool(node_add4->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);
  //(void)AttrUtils::SetBool(node_add2->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);
  //(void)AttrUtils::SetBool(node_add5->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  GraphManager graph_manager;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.OptimizeSubgraph(graph_node, graph, session_id), SUCCESS);
}*/


/*********************************************************************************************************************
 *              Data1   Const2   Data3                          PartitonedCall Subgraph
 *                 \     /  \    /
 *                  \   /    \  /                                     Data     Data
 *                  Add1 *PartitionedCall*                              \       /
 *                      \     /                                          \     /
 *                       \   /                                           NETOUTPUT
 *                     NETOUTPUT
*********************************************************************************************************************/
/*TEST_F(UtestPipelinePartition, pipeline_and_dynamic_shape_partition_success8) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_data3", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_partitionedcall1 = NodeBuilder("PartitionedCall1", PARTITIONEDCALL).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  (void)node_partitionedcall1->GetOpDesc()->SetAttr(ATTR_STAGE_LEVEL, GeAttrValue());

  ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("subgraph");
  auto sub_graph_node_input1 = NodeBuilder("sub_graph_node_data1", DATA).AddOutputDesc({10}).Build(sub_graph);
  auto sub_graph_node_input2 = NodeBuilder("sub_graph_node_data2", DATA).AddOutputDesc({10}).Build(sub_graph);
  auto sub_graph_node_output = NodeBuilder("sub_graph_node_netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(sub_graph);
  GraphUtils::AddEdge(sub_graph_node_input1->GetOutDataAnchor(0), sub_graph_node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(sub_graph_node_input2->GetOutDataAnchor(0), sub_graph_node_output->GetInDataAnchor(1));
  sub_graph->SetParentGraph(graph);
  sub_graph->SetParentNode(node_partitionedcall1);
  node_partitionedcall1->GetOpDesc()->AddSubgraphName("subgraph");
  node_partitionedcall1->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph");
  graph->AddSubgraph("subgraph", sub_graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_partitionedcall1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_partitionedcall1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_partitionedcall1->GetOutDataAnchor(0), node_output->GetInDataAnchor(1));

  bool is_pipeline_needed = true;
  (void)AttrUtils::SetBool(node_partitionedcall1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, is_pipeline_needed);

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  GraphManager graph_manager;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.OptimizeSubgraph(graph_node, graph, session_id), SUCCESS);
}*/


/*********************************************************************************************************************
 *             Const1   Const3  Const2  Const4
 *                 \     /         \      /
 *                  \   /           \    /
 *                  *Add1*           Add2
 *                      \             / 
 *                       \           /
 *                        \         /
 *                         \       /
 *                            Add3
 *                             |
 *                         NETOUTPUT
*********************************************************************************************************************/
// 当前const按照拓扑序理论上分到modelA上(SUB_MODEL_INDEX属性为A)，但是其实际上只连到了modelB的某个节点上，此时需要修改该const的ATTR_SUBMODEL_INDEX为B.
// Const2在InputNodeSplit之前SUBMODEL_INDEX是0，但是Const2所连的Add2的SUBMODEL_INDEX是1，在InputNodeSplit之后，Const2的SUB_MODEL_INDEX会被改成1.
TEST_F(UtestPipelinePartition, pipeline_partition_test7) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_const1", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_const3", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input4 = NodeBuilder("input_const4", CONSTANT).AddOutputDesc({10}).Build(graph);

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input4->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));

  (void)AttrUtils::SetStr(node_add1->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), SUCCESS);
}

/*********************************************************************************************************************
 *              Data1   Const2   Data3
 *                 \     /  \    /
 *                  \   /    \  /
 *                   Add1    Add2
 *                      \     /
 *                       \   /
 *                        Add3
 *                          |
 *                      NETOUTPUT
*********************************************************************************************************************/
// Split node on NETOUTPUT, just have one sub model, can not pipeline, return fail.
TEST_F(UtestPipelinePartition, pipeline_partition_test8) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  auto node_input1 = NodeBuilder("input_data1", DATA).AddOutputDesc({10}).Build(graph);
  auto node_input2 = NodeBuilder("input_const2", CONSTANT).AddOutputDesc({10}).Build(graph);
  auto node_input3 = NodeBuilder("input_data3", DATA).AddOutputDesc({10}).Build(graph);

  float_t weight[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  GeTensorDesc weight_desc(GeShape({10}), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
  OpDescUtils::SetWeights(node_input2, {tensor});

  auto node_add1 = NodeBuilder("Add1", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add2 = NodeBuilder("Add2", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);
  auto node_add3 = NodeBuilder("Add3", ADD).AddInputDesc({10}).AddInputDesc({10}).AddOutputDesc({10}).Build(graph);

  auto node_output = NodeBuilder("netoutput", NETOUTPUT).AddInputDesc({10}).Build(graph);

  GraphUtils::AddEdge(node_input1->GetOutDataAnchor(0), node_add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_input2->GetOutDataAnchor(0), node_add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_input3->GetOutDataAnchor(0), node_add2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add1->GetOutDataAnchor(0), node_add3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_add2->GetOutDataAnchor(0), node_add3->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_add3->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));

  (void)AttrUtils::SetStr(node_output->GetOpDesc(), ATTR_OUTPUT_PIPELINE, "pipeline_1");

  PipelinePartitioner pipeline_partitioner(graph);
  EXPECT_EQ(pipeline_partitioner.Partition(), FAILED);
}

} // namespace ge
