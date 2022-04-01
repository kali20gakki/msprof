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
#include "graph/partition/graph_partition.h"
#include "graph/compute_graph.h"
#include "graph/graph.h"
#include "graph/node.h"
#undef private
#undef protected

namespace ge {
namespace airut {

class GraphBuilder {
 public:
  explicit GraphBuilder(const std::string &name) { graph_ = std::make_shared<ComputeGraph>(name); }
  NodePtr AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt,
                  Format format = FORMAT_NCHW, DataType data_type = DT_FLOAT,
                  std::vector<int64_t> shape = {1, 1, 224, 224});
  NodePtr AddNode(const std::string &name, const std::string &type,
                  std::initializer_list<std::string> input_names,
                  std::initializer_list<std::string> output_names,
                  Format format = FORMAT_NCHW, DataType data_type = DT_FLOAT,
                  std::vector<int64_t> shape = {1, 1, 224, 224});
  void AddDataEdge(const NodePtr &src_node, int src_idx, const NodePtr &dst_node, int dst_idx);
  void AddControlEdge(const NodePtr &src_node, const NodePtr &dst_node);
  ComputeGraphPtr GetGraph() {
    graph_->TopologicalSorting();
    return graph_;
  }

 private:
  ComputeGraphPtr graph_;
};

NodePtr GraphBuilder::AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt, Format format,
                              DataType data_type, std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape(shape));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginFormat(format);
  tensor_desc->SetOriginShape(GeShape(shape));
  tensor_desc->SetOriginDataType(data_type);

  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc);
}
void GraphBuilder::AddDataEdge(const NodePtr &src_node, int src_idx, const NodePtr &dst_node, int dst_idx) {
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(src_idx), dst_node->GetInDataAnchor(dst_idx));
}
void GraphBuilder::AddControlEdge(const NodePtr &src_node, const NodePtr &dst_node) {
  GraphUtils::AddEdge(src_node->GetOutControlAnchor(), dst_node->GetInControlAnchor());
}
NodePtr GraphBuilder::AddNode(const string &name, const string &type, std::initializer_list<std::string> input_names,
                              std::initializer_list<std::string> output_names, Format format, DataType data_type,
                              std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape(shape));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginFormat(format);
  tensor_desc->SetOriginShape(GeShape(shape));
  tensor_desc->SetOriginDataType(data_type);

  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (auto &input_name : input_names) {
    op_desc->AddInputDesc(input_name, tensor_desc->Clone());
  }
  for (auto &output_name :output_names) {
    op_desc->AddOutputDesc(output_name, tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc);
}

}  // namespace airut
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

/*                                  -------------------------
*                                  |  partitioncall_0_const1* |
*     partitioncall_0--------------|             |           |
*           |                      |          netoutput      |
*           |                      --------------------------
*           |                       ------------------         -------------
*           |                      |        data      |       |    data     |
*           |                      |          |       |       |     |       |
*     partitioncall_1--------------|        case -----|-------|   squeeze*  |
*                                  |          |       |       |     |       |
*                                  |      netoutput   |       |  netoutput  |
*                                   ------------------         -------------
*/
ComputeGraphPtr BuildGraphPartitionCall() {
  auto root_builder = airut::GraphBuilder("root");
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(partitioncall_0, 0, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = airut::GraphBuilder("partitioncall_0_sub");
  const auto &partitioncall_0_const1 = p1_sub_builder.AddNode("partitioncall_0_const1", CONSTANT, 0, 1);
  const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 1, 1);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  p1_sub_builder.AddDataEdge(partitioncall_0_const1, 0, partitioncall_0_netoutput, 0);
  const auto &sub_graph = p1_sub_builder.GetGraph();
  sub_graph->SetParentNode(partitioncall_0);
  sub_graph->SetParentGraph(root_graph);
  partitioncall_0->GetOpDesc()->AddSubgraphName("f");
  partitioncall_0->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_0_sub");

  // 2.build partitioncall_1 sub graph
  auto p2_sub_builder = airut::GraphBuilder("partitioncall_1_sub");
  const auto &partitioncall_1_data = p2_sub_builder.AddNode("partitioncall_1_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_1_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &partitioncall_1_case = p2_sub_builder.AddNode("partitioncall_1_case", "Case", 1, 1);
  const auto &partitioncall_1_netoutput = p2_sub_builder.AddNode("partitioncall_1_netoutput", NETOUTPUT, 1, 1);
  p2_sub_builder.AddDataEdge(partitioncall_1_data, 0, partitioncall_1_case, 0);
  p2_sub_builder.AddDataEdge(partitioncall_1_case, 0, partitioncall_1_netoutput, 0);
  const auto &sub_graph2 = p2_sub_builder.GetGraph();
  sub_graph2->SetParentNode(partitioncall_1);
  sub_graph2->SetParentGraph(root_graph);
  partitioncall_1->GetOpDesc()->AddSubgraphName("f");
  partitioncall_1->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_1_sub");

  // 2.1 build case sub graph
  auto case_sub_builder = airut::GraphBuilder("case_sub");
  const auto &case_data = case_sub_builder.AddNode("case_data", DATA, 0, 1);
  AttrUtils::SetInt(case_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &case_squeeze = case_sub_builder.AddNode("case_squeeze", SQUEEZE, 1, 1);
  const auto &case_netoutput = case_sub_builder.AddNode("case_netoutput", NETOUTPUT, 1, 1);
  case_sub_builder.AddDataEdge(case_data, 0, case_squeeze, 0);
  case_sub_builder.AddDataEdge(case_squeeze, 0, case_netoutput, 0);
  const auto &case_sub_graph = case_sub_builder.GetGraph();
  case_sub_graph->SetParentNode(partitioncall_1_case);
  case_sub_graph->SetParentGraph(sub_graph2);
  partitioncall_1_case->GetOpDesc()->AddSubgraphName("branches");
  partitioncall_1_case->GetOpDesc()->SetSubgraphInstanceName(0, "case_sub");

  root_graph->AddSubgraph(case_sub_graph->GetName(), case_sub_graph);
  root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);
  root_graph->AddSubgraph(sub_graph2->GetName(), sub_graph2);
  return root_graph;
}
}  // namespace

using namespace airut;
class UtestGraphPartition : public testing::Test {
  protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(UtestGraphPartition, check_if_end2pld_empty_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = nullptr;
  ComputeGraphPtr graph1 = std::make_shared<ComputeGraph>("default");
  EXPECT_NE(graphPartitioner.CheckIfEnd2PldEmpty(graph), SUCCESS);
  ge::PartitionMap partitionMap;
  partitionMap[graph] =  1;
  graphPartitioner.graph_info_.partitions_ = partitionMap;
  EXPECT_NE(graphPartitioner.CheckIfEnd2PldEmpty(graph), SUCCESS);
  ge::PartitionMap partitionMap1;
  partitionMap1[graph1] =  2;
  graphPartitioner.graph_info_.partitions_ = partitionMap1;
  EXPECT_EQ(graphPartitioner.CheckIfEnd2PldEmpty(graph), SUCCESS);
}

TEST_F(UtestGraphPartition, has_second_path_test) {
  size_t src = 0;
  size_t dst = 1;
  size_t upper_bound;
  GraphPartitioner graphPartitioner;
  string test = "test";
  ClusterPtr cluster = std::make_shared<Cluster>(1, test, test);
  ClusterSet out_set;
  out_set.insert(0);
  out_set.insert(1);
  ClusterSet in_set;
  in_set.insert(0);
  in_set.insert(1);
  cluster->out_clu_ = out_set;
  cluster->in_clu_ = in_set;
  ClusterPtr cluster1 = std::make_shared<Cluster>(1, test, test);
  ClusterSet out_set1;
  out_set1.insert(0);
  out_set1.insert(1);
  ClusterSet in_set1;
  in_set1.insert(0);
  in_set1.insert(1);
  cluster1->out_clu_ = out_set1;
  cluster1->in_clu_ = in_set1;
  std::unordered_map<size_t, ClusterPtr> clusters;
  clusters[src] = cluster;
  clusters[dst] = cluster1;
  graphPartitioner.graph_info_.clusters_ = clusters;
  EXPECT_NE(graphPartitioner.HasSecondPath(src, dst, upper_bound), SUCCESS);
}
TEST_F(UtestGraphPartition, merge_sub_graph_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  EXPECT_NE(graphPartitioner.MergeSubGraph(graph, graph1), SUCCESS);
  graph1 = std::make_shared<ComputeGraph>("default1");
  EXPECT_EQ(graphPartitioner.MergeSubGraph(graph, graph1), SUCCESS);
  AttrUtils::SetInt(graph1, "globleworkspace_status", 1);
  AttrUtils::SetInt(graph1, "globleworkspace_status_bytes", 1);
  EXPECT_EQ(graphPartitioner.MergeSubGraph(graph, graph1), SUCCESS);
}


TEST_F(UtestGraphPartition, merge_after_sub_graph_optimization_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  EXPECT_NE(graphPartitioner.MergeAfterSubGraphOptimization(graph, graph1), SUCCESS);
  graph1 = std::make_shared<ComputeGraph>("default1"); 
  EXPECT_EQ(graphPartitioner.MergeAfterSubGraphOptimization(graph, graph1), SUCCESS);
}

TEST_F(UtestGraphPartition, update_end_op_desc_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  EXPECT_NE(graphPartitioner.MergeAfterSubGraphOptimization(graph, graph1), SUCCESS);
  graph1 = std::make_shared<ComputeGraph>("default1");
  NodePtr node1 =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr add_n_node =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr node2 =
      NodeBuilder("node2", RELU).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), add_n_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  (void)AttrUtils::SetBool(add_n_node->GetOpDesc(), ATTR_SINGLE_OP_SCENE, true);

  NodePtr node3 = nullptr;
  EXPECT_NE(graphPartitioner.UpdateEndOpDesc(node3, 1, add_n_node->GetOpDesc()), SUCCESS);
  EXPECT_EQ(graphPartitioner.UpdateEndOpDesc(add_n_node, 1, add_n_node->GetOpDesc()), SUCCESS);
}

TEST_F(UtestGraphPartition, update_pld_op_desc_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  EXPECT_NE(graphPartitioner.MergeAfterSubGraphOptimization(graph, graph1), SUCCESS);
  graph1 = std::make_shared<ComputeGraph>("default1");
  NodePtr node1 =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr add_n_node =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr node2 =
      NodeBuilder("node2", RELU).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), add_n_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  (void)AttrUtils::SetBool(add_n_node->GetOpDesc(), ATTR_SINGLE_OP_SCENE, true);

  NodePtr node3 = nullptr;
  EXPECT_NE(graphPartitioner.UpdatePldOpDesc(node3, 1, add_n_node->GetOpDesc()), SUCCESS);
  EXPECT_EQ(graphPartitioner.UpdatePldOpDesc(add_n_node, 1, add_n_node->GetOpDesc()), SUCCESS);
}


TEST_F(UtestGraphPartition, link_input2_end_remove_orginal_linktest) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;  
  EXPECT_NE(graphPartitioner.MergeAfterSubGraphOptimization(graph, graph1), SUCCESS);
  graph1 = std::make_shared<ComputeGraph>("default1");
  NodePtr node1 =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr add_n_node =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr node2 =
      NodeBuilder("node2", RELU).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), add_n_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  (void)AttrUtils::SetBool(add_n_node->GetOpDesc(), ATTR_SINGLE_OP_SCENE, true);
  NodePtr node3 = nullptr;
  EXPECT_NE(graphPartitioner.LinkInput2EndRemoveOrginalLink(node3, graph, graph1), SUCCESS);
  EXPECT_EQ(graphPartitioner.LinkInput2EndRemoveOrginalLink(add_n_node, graph, graph1), SUCCESS);
}


TEST_F(UtestGraphPartition, put_input_nodes_in_sub_graph_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;  
  EXPECT_NE(graphPartitioner.PutInputNodesInSubGraph(graph, graph1), SUCCESS);  ;
  graph1 = std::make_shared<ComputeGraph>("default1");  
  EXPECT_EQ(graphPartitioner.PutInputNodesInSubGraph(graph, graph1), SUCCESS);
}

TEST_F(UtestGraphPartition, add_new_graph_to_partition_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  std::string engine_name = "test";
  graphPartitioner.AddNewGraphToPartition(graph1, engine_name);
  graphPartitioner.AddNewGraphToPartition(graph, engine_name);
}

TEST_F(UtestGraphPartition, add_partitions_to_graph_node_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  std::string engine_name = "test";
  std::vector<ge::SubGraphInfoPtr> output_subgraphs;
  auto ret = graphPartitioner.AddPartitionsToGraphNode(output_subgraphs, graph);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphPartition, split_sub_graphs_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  auto ret = graphPartitioner.SplitSubGraphs(graph);
  ASSERT_EQ(ret, SUCCESS);
  ret = graphPartitioner.SplitSubGraphs(graph1);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphPartition, add_place_holder_end_test) {
  GraphPartitioner graphPartitioner;
  AnchorPtr out_anchor;
  AnchorPtr in_anchor;
  auto ret = graphPartitioner.AddPlaceHolderEnd(out_anchor, in_anchor);
  ASSERT_NE(ret, SUCCESS);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ge::NodePtr aipp1 = NodeBuilder("aipp1", AIPP)
                  .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                  .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                  .Build(graph);
  ge::NodePtr data1 = NodeBuilder("data1", DATA)
                  .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                  .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                  .Build(graph);
  OutDataAnchorPtr outptr = std::make_shared<OutDataAnchor>(aipp1, 0);
  OutDataAnchorPtr inptr = std::make_shared<OutDataAnchor>(data1, 0);
  ret = graphPartitioner.AddPlaceHolderEnd(outptr->GetFirstPeerAnchor(), inptr->GetFirstPeerAnchor());
  ASSERT_NE(ret, SUCCESS);  
}

TEST_F(UtestGraphPartition, sort_sub_graphs_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  auto ret = graphPartitioner.SortSubGraphs(graph);
  ASSERT_EQ(ret, SUCCESS);
  ret = graphPartitioner.SortSubGraphs(graph1);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphPartition, get_end_in_anchor_test) {
  AnchorPtr out_anchor;
  NodePtr in_node;
  GraphPartitioner graphPartitioner;
  auto ret = graphPartitioner.GetEndInAnchor(out_anchor, in_node);
  ASSERT_EQ(ret, nullptr);
}

TEST_F(UtestGraphPartition, get_pld_out_anchor_test) {
  AnchorPtr in_anchor;
  GraphPartitioner graphPartitioner;
  NodePtr node3 = nullptr;
  auto ret = graphPartitioner.GetPldOutAnchor(node3, in_anchor);
  ASSERT_EQ(ret, nullptr);
}

TEST_F(UtestGraphPartition, add_end_pld_information_to_sub_graph_info_test) {
  GraphPartitioner graphPartitioner;
  ge::SubGraphInfoPtr subgraph_info = nullptr;
  graphPartitioner.AddEndPldInformationToSubGraphInfo(subgraph_info);
}

TEST_F(UtestGraphPartition, partition_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  GraphPartitioner::Mode mode = GraphPartitioner::Mode::kSecondPartitioning;
  auto ret = graphPartitioner.Partition(graph, mode);
  ASSERT_NE(ret, SUCCESS);
  ret = graphPartitioner.Partition(graph1, mode);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphPartition, partition_sub_graph_test) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr graph1 = nullptr;
  GraphPartitioner::Mode mode = GraphPartitioner::Mode::kSecondPartitioning;
  graph->SetOutputSize(0);
  auto ret = graphPartitioner.PartitionSubGraph(graph, mode);
  ASSERT_NE(ret, SUCCESS);
  ret = graphPartitioner.PartitionSubGraph(graph1, mode);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphPartition, RemoveNodeAndEdgeBetweenEndPld) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr output_merged_compute_graph = nullptr;
  std::vector<SubGraphInfoPtr> sub_graph_list;
  EXPECT_EQ(graphPartitioner.RemoveNodeAndEdgeBetweenEndPld(output_merged_compute_graph, sub_graph_list), FAILED);
}

TEST_F(UtestGraphPartition, MergeAfterSubGraphOptimization) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr output_merged_compute_graph = std::make_shared<ComputeGraph>("graph");
  ComputeGraphPtr original_compute_graph = BuildGraphPartitionCall();
  EXPECT_EQ(graphPartitioner.MergeAfterSubGraphOptimization(output_merged_compute_graph, original_compute_graph), FAILED);
}

TEST_F(UtestGraphPartition, HasNoInput) {
  GraphPartitioner graphPartitioner;
  NodePtr node = nullptr;
  EXPECT_EQ(graphPartitioner.HasNoInput(node), true);
}

TEST_F(UtestGraphPartition, TestInheritOriginalAttrSuccess) {
  GraphPartitioner graphPartitioner;
  ComputeGraphPtr output_merged_compute_graph = std::make_shared<ComputeGraph>("graph");
  ComputeGraphPtr original_compute_graph = BuildGraphPartitionCall();
  AttrUtils::SetBool(original_compute_graph, "_mem_realse_first_reuse_first", true);
  graphPartitioner.InheritOriginalAttr(original_compute_graph, output_merged_compute_graph);
  EXPECT_EQ(output_merged_compute_graph->HasAttr("_mem_realse_first_reuse_first"), true);
}
} // namespace ge