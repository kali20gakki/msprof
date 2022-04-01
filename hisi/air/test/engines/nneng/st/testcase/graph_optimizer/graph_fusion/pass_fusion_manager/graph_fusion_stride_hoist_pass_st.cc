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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/stride_hoist_pass.h"
#include "common/pass_manager.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "graph_optimizer/node_optimizer/split_n_optimizer.h"

#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;

namespace fe
{

class NODE_OPTIMIZER_ST : public testing::Test
{
public:
    ComputeGraphPtr graph;

protected:
    void SetUp()
    {
        setenv("DUMP_GE_GRAPH", "2", 2);
    }

    void TearDown()
    {
        unsetenv("DUMP_GE_GRAPH");
    }
};

TEST_F(NODE_OPTIMIZER_ST, stride_hoist_pass_quant_success)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/st/testcase/graph_optimizer/graph_fusion/pass_fusion_manager/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector<fe::GraphPass *> passes = {&pass};
    fe::Status status = fe::PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_ST, stride_hoist_pass_no_quant_success)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/st/testcase/graph_optimizer/graph_fusion/pass_fusion_manager/ge_proto_00017_OptimizeOriginalGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector<fe::GraphPass *> passes = {&pass};
    fe::Status status = fe::PassManager::Run(*graph, passes);
    
    EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_ST, stride_hoist_pass_conv_quant_success)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/st/testcase/graph_optimizer/graph_fusion/pass_fusion_manager/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    ge::NodePtr node = graph->FindNode("res3a_branch2c");
    ge::OpDescPtr op_desc = node->GetOpDesc();
    vector<int64_t> strides;
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    strides[2] = 2;
    strides[3] = 2;
    ge::AttrUtils::SetListInt(op_desc, "strides", strides);

    fe::StrideHoistingPass pass;
    vector<fe::GraphPass *> passes = {&pass};
    fe::Status status = fe::PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,1,1}));
    node = graph->FindNode("res3a_branch2b");
    op_desc = node->GetOpDesc();
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,2,2}));
}

TEST_F(NODE_OPTIMIZER_ST, stride_hoist_pass_conv_no_quant_success)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/st/testcase/graph_optimizer/graph_fusion/pass_fusion_manager/ge_proto_00017_OptimizeOriginalGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }

    ge::NodePtr node = graph->FindNode("res3a_branch2c");
    ge::OpDescPtr op_desc= node->GetOpDesc();
    vector<int64_t> strides;
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    strides[2] = 2;
    strides[3] = 2;
    ge::AttrUtils::SetListInt(op_desc, "strides", strides);
    
    fe::StrideHoistingPass pass;
    vector<fe::GraphPass *> passes = {&pass};
    fe::Status status = fe::PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,1,1}));
    node = graph->FindNode("res3a_branch2b");
    op_desc = node->GetOpDesc();
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,2,2}));
}

void CreateGraph(ge::ComputeGraphPtr &graph, ge::NodePtr &node, string next_op_type) {
  graph = std::make_shared<ComputeGraph>("TestGraph");

  vector<int64_t> dims{1, 2, 3, 4};
  ge::GeShape shape(dims);

  ge::GeTensorDesc tensor(shape);
  ge::OpDescPtr op = std::make_shared<OpDesc>("test", "Test");
  op->AddInputDesc(tensor);
  op->AddOutputDesc(tensor);

  ge::OpDescPtr next_op = std::make_shared<OpDesc>("next", next_op_type);
  next_op->AddInputDesc(tensor);

  node = graph->AddNode(op);
  auto next_node = graph->AddNode(next_op);

  ge::GraphUtils::AddEdge(node->GetOutDataAnchor(0), next_node->GetInDataAnchor(0));
}

TEST_F(NODE_OPTIMIZER_ST, coverage_01) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;
  ge::NodePtr node;

  CreateGraph(test_graph, node, "NetOutput");
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_ST, coverage_02) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_ST, coverage_03) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), true);
}

TEST_F(NODE_OPTIMIZER_ST, coverage_04) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);
      string fusion_virtual_op ="1";
      (void)ge::AttrUtils::SetStr(next_node->GetOpDesc(), ge::ATTR_NAME_FUSION_VIRTUAL_OP, fusion_virtual_op);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_ST, coverage_05) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);

      (void)ge::AttrUtils::SetBool(next_node->GetOpDesc(), ge::ATTR_NAME_CONTINUOUS_INPUT, true);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_ST, coverage_06) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);

      vector<int64_t> output_index = {1,2};
      (void)ge::AttrUtils::SetListInt(next_node->GetOpDesc(), ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}
} // namespace fe