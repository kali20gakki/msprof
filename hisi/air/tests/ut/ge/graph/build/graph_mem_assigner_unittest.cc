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
#include <memory>

#define protected public
#define private public
#include "graph/build/memory/binary_block_mem_assigner.h"
#include "graph/build/memory/graph_mem_assigner.h"
#include "graph/build/memory/hybrid_mem_assigner.h"
#include "graph/build/memory/max_block_mem_assigner.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/anchor.h"
#include "graph/op_desc.h"
#include "graph/op_desc_impl.h"
#include "graph/ge_tensor_impl.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "omg/omg_inner_types.h"
#include "../passes/graph_builder_utils.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
using domi::GetContext;

namespace ge {

class UtestGraphMemAssigner : public testing::Test {
 public:
    ge::ComputeGraphPtr BuildGraphWithVar(int64_t session_id) {
      // init
      MemManager::Instance().Initialize(std::vector<rtMemType_t>({RT_MEMORY_HBM}));
      VarManager::Instance(session_id)->Init(0, 0, 0, 0);
      ge::ut::GraphBuilder builder("graph");
      auto var_input = builder.AddNode("var", "Variable", 1, 1);
      auto const_input = builder.AddNode("const", "Const", 1, 1);
      auto assign = builder.AddNode("assgin", "Assign", 2, 1);
      // add link
      builder.AddDataEdge(var_input, 0, assign, 0);
      builder.AddDataEdge(const_input, 0, assign, 1);
      // set offset
      var_input->GetOpDesc()->SetOutputOffset({10000});
      const_input->GetOpDesc()->SetOutputOffset({1000});
      assign->GetOpDesc()->SetInputOffset({10100, 1000});
      assign->GetOpDesc()->SetOutputOffset({10100});
      // set inner offset
      int64_t inner_offset = 100;
      ge::AttrUtils::SetInt(assign->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset);
      ge::AttrUtils::SetInt(assign->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset);
      // add var addr
      VarManager::Instance(session_id)->var_resource_->var_offset_map_.emplace(10000, RT_MEMORY_HBM);

      return builder.GetGraph();
    }

protected:
    void SetUp() {}
    void TearDown() {}
};


NodePtr UtAddNode(ComputeGraphPtr &graph, std::string name, std::string type, int in_cnt, int out_cnt){
    auto tensor_desc = std::make_shared<GeTensorDesc>();
    std::vector<int64_t> shape = {1, 1, 224, 224};
    tensor_desc->SetShape(GeShape(shape));
    tensor_desc->SetFormat(FORMAT_NCHW);
    tensor_desc->SetDataType(DT_FLOAT);
    tensor_desc->SetOriginFormat(FORMAT_NCHW);
    tensor_desc->SetOriginShape(GeShape(shape));
    tensor_desc->SetOriginDataType(DT_FLOAT);
    auto op_desc = std::make_shared<OpDesc>(name, type);
    for (int i = 0; i < in_cnt; ++i) {
        op_desc->AddInputDesc(tensor_desc->Clone());
    }
    for (int i = 0; i < out_cnt; ++i) {
        op_desc->AddOutputDesc(tensor_desc->Clone());
    }
    return graph->AddNode(op_desc);
}

TEST_F(UtestGraphMemAssigner, graph_memory_assign_fail_case) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("");
  GraphMemoryAssigner graph_mem_assigner(compute_graph);
  MemoryOffset mem_offset(2, 10000);
  graph_mem_assigner.memory_offset_.insert({2, mem_offset});
  VarManager::Instance(0)->graph_mem_max_size_ = 0;

  map<uint64_t, size_t> mem_type_to_offset = {};
  Status ret = graph_mem_assigner.ReAssignMemory(mem_type_to_offset);
  EXPECT_EQ(ret, ACL_ERROR_GE_MEMORY_ALLOCATION);
}

TEST_F(UtestGraphMemAssigner, graph_memory_get_type_case) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("");
  GraphMemoryAssigner graph_mem_assigner(compute_graph);
  ge::ut::GraphBuilder builder("graph");
  NodePtr node_one = builder.AddNode("node_one", ATTR_NAME_CONTINUOUS_INPUT, 1, 1);
  std::vector<int64_t> mem_type_list;
  mem_type_list.emplace_back(55);
  mem_type_list.emplace_back(66);
  ge::AttrUtils::SetListInt(node_one->GetOpDesc(), ATTR_NAME_INPUT_MEM_TYPE_LIST, mem_type_list);
  compute_graph->AddNode(node_one);
  int64_t memory_type = 0;
  Status ret = graph_mem_assigner.GetNodeMemoryType(node_one, memory_type, "input");
  EXPECT_EQ(ret, FAILED);

  NodePtr node_two = builder.AddNode("node_two", ATTR_NAME_CONTINUOUS_OUTPUT, 1, 1);
  ge::AttrUtils::SetListInt(node_two->GetOpDesc(), ATTR_NAME_OUTPUT_MEM_TYPE_LIST, mem_type_list);
  compute_graph->AddNode(node_two);
  ret = graph_mem_assigner.GetNodeMemoryType(node_one, memory_type, "output");
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestGraphMemAssigner, Assign) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  ge::VariableMemoryAssigner vma(graph);
  vma.compute_graph_ = nullptr;
  EXPECT_EQ(vma.Assign(), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  ge::GraphMemoryAssigner gma(graph);
  EXPECT_EQ(gma.AssignMemory(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousInputMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "data1", "DATA", 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  int64_t continuous_mem_start = 0;
  int64_t continuous_mem_size = 0;
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemory(node1, continuous_mem_start, continuous_mem_size, 1, 2, true), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousInputMemoryFailedIdx) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "data1", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "data2", DATA, 1, 1);
  node2->GetOutDataAnchor(0)->SetIdx(10);
  EXPECT_EQ(node1->GetInDataAnchor(0)->LinkFrom(node2->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  std::vector<int64_t> offsets_of_fusion = {1,2,3};
  AttrUtils::SetListInt(node2->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, offsets_of_fusion);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(1, MemoryOffset((rtMemType_t)1, (size_t)1024)));
  int64_t continuous_mem_start = 0;
  int64_t continuous_mem_size = 0;
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemory(node1, continuous_mem_start, continuous_mem_size, 1, 2, true), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousInputMemoryFailed) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "data1", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "data2", DATA, 1, 1);
  EXPECT_EQ(node1->GetInDataAnchor(0)->LinkFrom(node2->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  std::vector<int64_t> offsets_of_fusion = {1,2,3};
  AttrUtils::SetListInt(node2->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, offsets_of_fusion);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(1, MemoryOffset((rtMemType_t)1, (size_t)1024)));
  int64_t continuous_mem_start = 0;
  int64_t continuous_mem_size = 0;
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemory(node1, continuous_mem_start, continuous_mem_size, 1, 2, true), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousInputMemoryWithAtomicProcessDirectly) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "data1", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "var2", VARIABLE, 1, 1);
  EXPECT_EQ(node1->GetInDataAnchor(0)->LinkFrom(node2->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto nmap = std::map<NodePtr, uint32_t>();
  nmap[node1] = 4;
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemoryWithAtomicProcessDirectly(node1, nmap), true);
  auto node3 = UtAddNode(graph, "data3", DATA, 1, 1);
  auto node4 = UtAddNode(graph, "data4", DATA, 1, 1);
  EXPECT_EQ(node3->GetInDataAnchor(0)->LinkFrom(node4->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  nmap[node3] = 1U;
  nmap[node4] = 2U;
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemoryWithAtomicProcessDirectly(node3, nmap), false);
  EXPECT_EQ(ge::AttrUtils::SetBool(node3->GetOpDesc(), ATTR_NAME_CONTINUOUS_INPUT, true), true);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousInputMemoryWithAtomicProcessDirectly(node4, nmap), false);
}

TEST_F(UtestGraphMemAssigner, CheckOffset) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "data1", DATA, 1, 1);
  auto node4 = UtAddNode(graph, "data4", DATA, 1, 1);
  EXPECT_EQ(node1->GetInDataAnchor(0)->LinkFrom(node4->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.CheckOffset(), FAILED);
  ge::ComputeGraphPtr graph2 = std::make_shared<ge::ComputeGraph>("graph2");
  auto node2 = UtAddNode(graph2, "data2", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner2(graph2);
  auto vec1 = std::vector<int64_t>();
  auto vec2 = std::vector<int64_t>();
  vec1.push_back(1);
  vec2.push_back(-1);
  node2->GetOpDesc()->SetInputOffset(vec1);
  node2->GetOpDesc()->SetOutputOffset(vec2);
  EXPECT_EQ(graph_mem_assigner2.CheckOffset(), FAILED);
  vec1.push_back(-1);
  node2->GetOpDesc()->SetInputOffset(vec1);
  EXPECT_EQ(graph_mem_assigner2.CheckOffset(), FAILED);
  vec1.clear();
  vec2.clear();
  vec1.push_back(1);
  vec2.push_back(1);
  ge::ComputeGraphPtr graph3 = std::make_shared<ge::ComputeGraph>("graph3");
  auto node3 = UtAddNode(graph3, "iden3", IDENTITY, 1, 1);
  GraphMemoryAssigner graph_mem_assigner3(graph3);
  vec1.push_back(1);
  vec2.push_back(1);
  node3->GetOpDesc()->SetInputOffset(vec1);
  node3->GetOpDesc()->SetOutputOffset(vec2);
  EXPECT_EQ(graph_mem_assigner3.CheckOffset(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, CheckOffset_Workspace) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto vec1 = std::vector<int64_t>();
  auto vec2 = std::vector<int64_t>();
  auto vec3 = std::vector<int64_t>();
  vec1.push_back(1);
  vec2.push_back(1);
  vec3.push_back(-1);
  node->GetOpDesc()->SetInputOffset(vec1);
  node->GetOpDesc()->SetOutputOffset(vec2);
  node->GetOpDesc()->SetWorkspace(vec3);
  EXPECT_EQ(graph_mem_assigner.CheckOffset(), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousOutputMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "data2", DATA, 0, 0);
  auto node3 = UtAddNode(graph, "data3", DATA, 1, 1);
  EXPECT_EQ(node->GetInDataAnchor(0)->LinkFrom(node3->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node, 1, 8), FAILED);
  auto output = vector<int64_t>();
  output.push_back(1);
  node->GetOpDesc()->SetOutputOffset(output);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node, 1, 127), FAILED);
  auto output3 = vector<int64_t>();
  output3.push_back(1);
  output3.push_back(2);
  output3.push_back(3);
  node3->GetOpDesc()->SetOutputOffset(output3);
  EXPECT_EQ(node3->GetOpDesc()->GetOutputOffset().size(), 3);
  EXPECT_EQ(node3->GetOutDataAnchor(0)->GetIdx(), 0);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node, 1, 127), FAILED);
  AttrUtils::SetBool(node->GetOpDesc(), "reference", true);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node, 1, 4), SUCCESS);
  auto output2 = vector<int64_t>();
  output2.push_back(1);
  node2->GetOpDesc()->SetOutputOffset(output2);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node2, 1, 127), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignContinuousOutputMemoryCal) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  auto node3 = UtAddNode(graph, "data3", DATA, 1, 1);
  EXPECT_EQ(node->GetInDataAnchor(0)->LinkFrom(node3->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto output = vector<int64_t>();
  output.push_back(1);
  output.push_back(2);
  output.push_back(3);
  node->GetOpDesc()->SetOutputOffset(output);
  auto output3 = vector<int64_t>();
  output3.push_back(1);
  output3.push_back(2);
  output3.push_back(3);
  node3->GetOpDesc()->SetOutputOffset(output3);
  EXPECT_EQ(node->GetOpDesc()->GetOutputOffset().size(), 3);
  EXPECT_EQ(node->GetOutDataAnchor(0)->GetIdx(), 0);
  ge::AttrUtils::SetInt(node->GetOpDesc(), "_reuse_input_on_dim_index", 2);
  ConstGeTensorDescPtr output_desc = node->GetOpDesc()->GetOutputDescPtr(node->GetOutDataAnchor(0)->GetIdx());
  EXPECT_EQ(output_desc->GetShape().GetDims().size(), 4);
  EXPECT_EQ(graph_mem_assigner.AssignContinuousOutputMemory(node, 1, 15), SUCCESS);
}


TEST_F(UtestGraphMemAssigner, AssignConnectNetOutputAtomicMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  std::vector<NodePtr> connect_netoutput_nodes;
  EXPECT_EQ(graph_mem_assigner.AssignConnectNetOutputAtomicMemory(connect_netoutput_nodes), FAILED);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  EXPECT_EQ(graph_mem_assigner.AssignConnectNetOutputAtomicMemory(connect_netoutput_nodes), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, AssignConnectNetOutputAtomicMemoryNodes) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  std::vector<NodePtr> connect_netoutput_nodes;
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  connect_netoutput_nodes.push_back(node);
  EXPECT_EQ(graph_mem_assigner.AssignConnectNetOutputAtomicMemory(connect_netoutput_nodes), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, CheckInputIsSupportAtomic) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.CheckInputIsSupportAtomic(node), true);
  auto node2 = UtAddNode(graph, "data2", DATA, 1, 1);
  EXPECT_EQ(node->GetInDataAnchor(0)->LinkFrom(node2->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  node2->GetOpDesc()->SetType(VARIABLE);
  EXPECT_EQ(graph_mem_assigner.CheckInputIsSupportAtomic(node), false);
}

TEST_F(UtestGraphMemAssigner, SetIndependentAtomicAttr) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto mem_offset_end = std::vector<int64_t>();
  int64_t atomic_mem_start = 0;
  EXPECT_EQ(graph_mem_assigner.SetIndependentAtomicAttr(node, (int64_t)0, mem_offset_end, (int64_t)RT_MEMORY_HBM), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, SetAtomicCleanAttr) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto atomic_mem_start = std::vector<int64_t>();
  auto atomic_mem_size = std::vector<int64_t>();
  EXPECT_EQ(graph_mem_assigner.SetAtomicCleanAttr(node, atomic_mem_start, atomic_mem_size, RT_MEMORY_HBM), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, CheckContinuousMemType) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  auto mem_type_list = std::vector<int64_t>();
  EXPECT_EQ(graph_mem_assigner.CheckContinuousMemType(mem_type_list), true);
}

TEST_F(UtestGraphMemAssigner, PrintMemoryOffset) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  graph_mem_assigner.PrintMemoryOffset();
}

TEST_F(UtestGraphMemAssigner, ReAssignAtomicMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "atom1", ATOMICADDRCLEAN, 1, 1);
  auto node2 = UtAddNode(graph, "atom2", ATOMICADDRCLEAN, 1, 1);
  EXPECT_EQ(node1->GetOutControlAnchor()->LinkTo(node2->GetInControlAnchor()), GRAPH_SUCCESS);
  AttrUtils::SetBool(node2->GetOpDesc(), "is_atomic_node", true);
  std::vector<uint32_t> value;
  value.push_back(1);
  value.push_back(2);
  value.push_back(3);
  value.push_back(4);
  value.push_back(5);
  AttrUtils::SetListInt(node2->GetOpDesc(), "atomic_output_index", value);
  EXPECT_EQ(node2->GetOpDesc()->GetOutputsSize(), 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.CheckAtomicNodeIsSupportRef(node2->GetOpDesc()), true);
  graph_mem_assigner.ReAssignAtomicMemory();
}

TEST_F(UtestGraphMemAssigner, ReAssignAtomicMemoryFilter) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node1 = UtAddNode(graph, "atom1", ATOMICADDRCLEAN, 1, 1);
  auto node2 = UtAddNode(graph, "atom2", ATOMICADDRCLEAN, 1, 1);
  EXPECT_EQ(node1->GetOutControlAnchor()->LinkTo(node2->GetInControlAnchor()), GRAPH_SUCCESS);
  AttrUtils::SetBool(node2->GetOpDesc(), "is_atomic_node", true);
  std::vector<uint32_t> value;
  value.push_back(1);
  value.push_back(2);
  value.push_back(3);
  value.push_back(4);
  value.push_back(5);
  AttrUtils::SetListInt(node2->GetOpDesc(), "atomic_output_index", value);
  EXPECT_EQ(node2->GetOpDesc()->GetOutputsSize(), 1);
  AttrUtils::SetBool(node2->GetOpDesc(), "is_atomic_node", true);
  AttrUtils::SetStr(node2->GetOpDesc(), "_batch_label", "batch_label");
  std::vector<int32_t> is_connecting_output;
  AttrUtils::SetListInt(node2->GetOpDesc(), "_is_connected_to_netoutput", is_connecting_output);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  EXPECT_EQ(graph_mem_assigner.CheckAtomicNodeIsSupportRef(node2->GetOpDesc()), true);
  EXPECT_NE(graph_mem_assigner.ReAssignAtomicMemory(), SUCCESS);
  is_connecting_output.push_back(1);
  AttrUtils::SetListInt(node2->GetOpDesc(), "_is_connected_to_netoutput", is_connecting_output);
  EXPECT_NE(graph_mem_assigner.ReAssignAtomicMemory(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, ReAssignMemoryFailed) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.clear();
  std::map<uint64_t, size_t> mem_type_to_offset;
  EXPECT_EQ(graph_mem_assigner.ReAssignMemory(mem_type_to_offset), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignReferenceMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_REFERENCE, true);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_NE(graph_mem_assigner.AssignReferenceMemory(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, AssignReferenceMemoryOutput) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  std::vector<int64_t> output_list;
  output_list.push_back(1);
  output_list.push_back(2);
  node->GetOpDesc()->SetOutputOffset(output_list);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.AssignReferenceMemory(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, AssignReferenceMemoryOutputGood) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_REFERENCE, true);
  std::vector<int64_t> output_list;
  output_list.push_back(1);
  output_list.push_back(2);
  node->GetOpDesc()->SetOutputOffset(output_list);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.AssignReferenceMemory(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, GetMemoryAssignmentStatus) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "data2", DATA, 1, 1);
  EXPECT_EQ(node->GetOutDataAnchor(0)->LinkTo(node2->GetInDataAnchor(0)), GRAPH_SUCCESS);
  int64_t output_index = 10;
  bool is_mem_assigned = true;
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.GetMemoryAssignmentStatus(node, output_index, is_mem_assigned), PARAM_INVALID);
  output_index = 0;
  EXPECT_EQ(graph_mem_assigner.GetMemoryAssignmentStatus(node, output_index, is_mem_assigned), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, GetNodeListMemoryType) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  auto node2 = UtAddNode(graph, "data2", DATA, 1, 1);
  std::vector<NodePtr> nodes;
  nodes.push_back(node);
  nodes.push_back(node2);
  int32_t mem_reuse_model = 0;
  int64_t memory_type = 0;
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_NE(graph_mem_assigner.GetNodeListMemoryType(nodes, mem_reuse_model, memory_type), SUCCESS);
  mem_reuse_model = 1;
  EXPECT_NE(graph_mem_assigner.GetNodeListMemoryType(nodes, mem_reuse_model, memory_type), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, CheckContinuousMemTypeAll) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  std::vector<int64_t> mem_type_list;
  mem_type_list.push_back(1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.CheckContinuousMemType(mem_type_list), false);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(1, MemoryOffset(1, 0)));
  EXPECT_EQ(graph_mem_assigner.CheckContinuousMemType(mem_type_list), true);
  mem_type_list.push_back(2);
  EXPECT_EQ(graph_mem_assigner.CheckContinuousMemType(mem_type_list), false);
}

TEST_F(UtestGraphMemAssigner, AssignAtomicOutputMemory) {
  std::vector<int64_t> mem_offset_end;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  std::vector<int64_t> output_list;
  output_list.push_back(1);
  node->GetOpDesc()->SetOutputOffset(output_list);
  std::vector<int64_t> atomic_output_index;
  atomic_output_index.push_back(1);
  ge::AttrUtils::SetListInt(node->GetOpDesc(), "atomic_output_index", atomic_output_index);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.AssignAtomicOutputMemory(node, mem_offset_end), FAILED);
  atomic_output_index.push_back(2);
  ge::AttrUtils::SetListInt(node->GetOpDesc(), "atomic_output_index", atomic_output_index);
  EXPECT_EQ(graph_mem_assigner.AssignAtomicOutputMemory(node, mem_offset_end), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignAtomicOutputMemoryParamInvalid) {
  std::vector<int64_t> mem_offset_end;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  std::vector<int64_t> output_list;
  output_list.push_back(1);
  node->GetOpDesc()->SetOutputOffset(output_list);
  std::vector<int64_t> atomic_output_index;
  atomic_output_index.push_back(10);
  ge::AttrUtils::SetListInt(node->GetOpDesc(), "atomic_output_index", atomic_output_index);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  EXPECT_EQ(graph_mem_assigner.AssignAtomicOutputMemory(node, mem_offset_end), PARAM_INVALID);
}

TEST_F(UtestGraphMemAssigner, AssignOrdinaryAtomicWorkspaceMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  std::map<std::string, std::map<int64_t, int64_t>> workspace_info;
  std::vector<int64_t> mem_offset_end;
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(2, MemoryOffset(1, 0)));
  workspace_info["noname"] = std::map<int64_t, int64_t>();
  EXPECT_EQ(graph_mem_assigner.AssignOrdinaryAtomicWorkspaceMemory(node->GetOpDesc(), workspace_info, mem_offset_end), PARAM_INVALID);
  graph_mem_assigner.memory_offset_.clear();
  EXPECT_EQ(graph_mem_assigner.AssignOrdinaryAtomicWorkspaceMemory(node->GetOpDesc(), workspace_info, mem_offset_end), FAILED);
}

TEST_F(UtestGraphMemAssigner, AssignFusionAtomicWorkspaceMemory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  std::map<std::string, std::map<int64_t, int64_t>> workspace_info;
  std::vector<int64_t> mem_offset_end;
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.AssignFusionAtomicWorkspaceMemory(node->GetOpDesc(), workspace_info, mem_offset_end), FAILED);
}

TEST_F(UtestGraphMemAssigner, SetInputOffset) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "data", DATA, 1, 1);
  GraphMemoryAssigner graph_mem_assigner(graph);
  graph_mem_assigner.memory_offset_.insert(std::make_pair<int64_t, MemoryOffset>(RT_MEMORY_HBM, MemoryOffset(1, 1000)));
  EXPECT_EQ(graph_mem_assigner.SetInputOffset(), SUCCESS);
}

TEST_F(UtestGraphMemAssigner, UpdateOpInputOffset) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  auto node = UtAddNode(graph, "hcom", HCOMBROADCAST , 3, 1);
  auto node1 = UtAddNode(graph, "hcom1", HCOMBROADCAST , 1, 1);
  auto node2 = UtAddNode(graph, "var2", VARIABLE , 1, 1);
  EXPECT_EQ(node->GetInDataAnchor(0)->LinkFrom(node1->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  EXPECT_EQ(node->GetInDataAnchor(1)->LinkFrom(node2->GetOutDataAnchor(0)), GRAPH_SUCCESS);
  GraphMemoryAssigner graph_mem_assigner(graph);
  EXPECT_EQ(graph_mem_assigner.UpdateOpInputOffset(node), SUCCESS);
}


} // namespace ge