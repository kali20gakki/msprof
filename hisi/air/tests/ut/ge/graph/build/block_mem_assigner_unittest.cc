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
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "graph/build/memory/block_mem_assigner.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
namespace block_mem_ut {
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
  auto root_builder = block_mem_ut::GraphBuilder("root");
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(partitioncall_0, 0, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = block_mem_ut::GraphBuilder("partitioncall_0_sub");
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
  auto p2_sub_builder = block_mem_ut::GraphBuilder("partitioncall_1_sub");
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
  auto case_sub_builder = block_mem_ut::GraphBuilder("case_sub");
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

}  // namespace block_mem_ut
using namespace block_mem_ut;
class UtestBlockMemAssigner : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }

  class FakBlockMemAssigner : public BlockMemAssigner {
   public:
    FakBlockMemAssigner(ComputeGraphPtr compute_graph, const std::map<std::string, std::string> &anchor_to_symbol,
                   const std::map<std::string, std::list<NodeIndexIO>> &symbol_to_anchors):BlockMemAssigner(compute_graph, anchor_to_symbol, symbol_to_anchors){};

   public:
    virtual Status GetMemoryRanges(std::vector<int64_t> &ranges) override {
      ranges.push_back(1);
      return SUCCESS;
    };
  };
};

TEST_F(UtestBlockMemAssigner, Normal) {
    auto p1 = std::make_shared<MemoryBlock>(1024);
}

TEST_F(UtestBlockMemAssigner, AssignOutputMemoryWithReuse) {
    auto builder = std::make_shared<block_mem_ut::GraphBuilder>("graph");
    auto node = builder->AddNode("node", DATA, 1, 1);
    ComputeGraphPtr compute_graph = builder->GetGraph();
    std::map<std::string, std::string> anchor_to_symbol;
    std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
    auto p1 = std::make_shared<FakBlockMemAssigner>(compute_graph, anchor_to_symbol, symbol_to_anchors);
    std::vector<int64_t> ranges;
    p1->op_reuse_env_valid_ = true;
    EXPECT_EQ(p1->AssignOutputMemoryWithReuse(node, ranges), SUCCESS);
    std::vector<int64_t> memorys_type;
    ge::AttrUtils::SetListInt(node->GetOpDesc(), "_output_memory_type", memorys_type);
    EXPECT_EQ(p1->AssignOutputMemoryWithReuse(node, ranges), INTERNAL_ERROR);
}

TEST_F(UtestBlockMemAssigner, AssignMemoryWithReuse) {
    auto builder = std::make_shared<block_mem_ut::GraphBuilder>("graph");
    auto node = builder->AddNode("node", DATA, 1, 1);
    ComputeGraphPtr root_graph = builder->GetGraph();
    auto p1_sub_builder = block_mem_ut::GraphBuilder("partitioncall_0_sub");
    const auto &partitioncall_0_const1 = p1_sub_builder.AddNode("partitioncall_0_const1", CONSTANT, 0, 1);
    const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 1, 1);
    const auto &sub_graph = p1_sub_builder.GetGraph();
    sub_graph->SetParentNode(node);
    sub_graph->SetParentGraph(root_graph);
    root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);

    std::map<std::string, std::string> anchor_to_symbol;
    std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
    auto p1 = std::make_shared<FakBlockMemAssigner>(sub_graph, anchor_to_symbol, symbol_to_anchors);
    std::vector<int64_t> ranges;
    std::vector<int64_t> tvm_workspace_memory_type;
    tvm_workspace_memory_type.push_back(1);
    AttrUtils::SetListInt(partitioncall_0_const1->GetOpDesc(), "tvm_workspace_type", tvm_workspace_memory_type);
    p1->AssignMemoryWithReuse(ranges);

    auto p2 = std::make_shared<FakBlockMemAssigner>(sub_graph, anchor_to_symbol, symbol_to_anchors);
    p2->AssignMemoryWithReuse(ranges);
}

TEST_F(UtestBlockMemAssigner, Assign) {
    auto builder = std::make_shared<block_mem_ut::GraphBuilder>("graph");
    auto node = builder->AddNode("node", DATA, 1, 1);
    ComputeGraphPtr compute_graph = builder->GetGraph();
    std::map<std::string, std::string> anchor_to_symbol;
    std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
    auto p1 = std::make_shared<FakBlockMemAssigner>(compute_graph, anchor_to_symbol, symbol_to_anchors);
    EXPECT_EQ(p1->Assign(), SUCCESS);
}

TEST_F(UtestBlockMemAssigner, GetWorkSpaceMemoryType) {
    auto builder = std::make_shared<block_mem_ut::GraphBuilder>("graph");
    auto node = builder->AddNode("node", DATA, 1, 1);
    ComputeGraphPtr compute_graph = builder->GetGraph();
    std::map<std::string, std::string> anchor_to_symbol;
    std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
    auto p1 = std::make_shared<FakBlockMemAssigner>(compute_graph, anchor_to_symbol, symbol_to_anchors);
    std::vector<int64_t> workspace_memory_type;
    ge::AttrUtils::SetListInt(node->GetOpDesc(), "tvm_workspace_type", workspace_memory_type);
    size_t index = 10;
    uint64_t memory_type = 1;
    std::vector<bool> workspace_reuse_flag;
    EXPECT_EQ(p1->GetWorkSpaceMemoryType(node, index, memory_type, workspace_reuse_flag), false);
}

TEST_F(UtestBlockMemAssigner, IsZeroCopyBlock) {
  auto builder = std::make_shared<block_mem_ut::GraphBuilder>("graph");
  auto node = builder->AddNode("node", DATA, 1, 1);
  ComputeGraphPtr compute_graph = builder->GetGraph();
  std::map<std::string, std::string> anchor_to_symbol;
  std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
  auto p1 = std::make_shared<FakBlockMemAssigner>(compute_graph, anchor_to_symbol, symbol_to_anchors);
  ge::AttrUtils::SetBool(compute_graph, "_dynamic_shape_partitioned", true);
  EXPECT_EQ(p1->IsZeroCopyBlock(node, 0, false), true);
}

} // namespace ge