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
#define private public
#include <string>
#include "common/ge_inner_error_codes.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/node_adapter.h"
#include "graph/passes/merge_to_stream_merge_pass.h"
#include "graph/passes/multi_batch_pass.h"
#undef private

using namespace ge;

class UtestMultiBatchPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
public:
  void make_graph_no_switch_n(ComputeGraphPtr graph) {
    GeTensorDesc scalar_tensor(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);
    auto var_desc = std::make_shared<OpDesc>("var", VARIABLEV2);
    var_desc->AddOutputDesc(scalar_tensor);
    auto var_node = graph->AddNode(var_desc);
  }

  OpDescPtr CreateOpDesc(const std::string name, const std::string type, uint32_t input_num, uint32_t output_num) {
    OpDescPtr op_desc = shared_ptr<OpDesc>(new (std::nothrow) OpDesc(name, type));
    for (uint32_t i = 0; i < input_num; i++) {
      op_desc->AddInputDesc(GeTensorDesc());
    }
    for (uint32_t i = 0; i < output_num; i++) {
      op_desc->AddOutputDesc(GeTensorDesc());
    }
    return op_desc;
  }

  void make_graph_two_switch_n(ComputeGraphPtr graph) {
    GeTensorDesc int32_tensor(GeShape({1, 1, 1, 1}), ge::FORMAT_NCHW, ge::DT_INT32);
    GeTensorDesc int64_tensor(GeShape(), ge::FORMAT_NCHW, ge::DT_INT64);
    GeTensorDesc scalar_tensor(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);

    auto data_desc = std::make_shared<OpDesc>("data", DATA);
    data_desc->AddOutputDesc(scalar_tensor);
    auto data_node = graph->AddNode(data_desc);

    auto pred_value_desc = std::make_shared<OpDesc>("pred_value", VARIABLEV2);
    pred_value_desc->AddOutputDesc(int64_tensor);
    auto pred_value_node = graph->AddNode(pred_value_desc);

    GeTensorDesc output_desc_batch_1 = scalar_tensor;
    std::vector<int64_t> shape_batch_1 = {1, 1};
    AttrUtils::SetListInt(output_desc_batch_1, ATTR_NAME_SWITCHN_PRED_VALUE, shape_batch_1);
    AttrUtils::SetListInt(output_desc_batch_1, ATTR_NAME_COMBINED_DYNAMIC_DIMS, shape_batch_1);
    GeTensorDesc output_desc_batch_2= scalar_tensor;
    std::vector<int64_t> shape_batch_2 = {2, 2};
    AttrUtils::SetListInt(output_desc_batch_2, ATTR_NAME_SWITCHN_PRED_VALUE, shape_batch_2);
    AttrUtils::SetListInt(output_desc_batch_2, ATTR_NAME_COMBINED_DYNAMIC_DIMS, shape_batch_2);

    auto switch_n_desc_1 = std::make_shared<OpDesc>("switch_n_1", SWITCHN);
    switch_n_desc_1->AddInputDesc(scalar_tensor);
    switch_n_desc_1->AddInputDesc(int64_tensor);
    switch_n_desc_1->AddOutputDesc(output_desc_batch_1);
    switch_n_desc_1->AddOutputDesc(output_desc_batch_2);
    AttrUtils::SetInt(switch_n_desc_1, ATTR_DYNAMIC_TYPE, 1);
    std::vector<std::string> user_input_order = {"data1", "data2"};
    AttrUtils::SetListStr(switch_n_desc_1, ATTR_USER_DESIGNEATE_SHAPE_ORDER, user_input_order);
    auto switch_n_node_1 = graph->AddNode(switch_n_desc_1);

    auto switch_n_desc_2 = std::make_shared<OpDesc>("switch_n_2", SWITCHN);
    switch_n_desc_2->AddInputDesc(scalar_tensor);
    switch_n_desc_2->AddInputDesc(int64_tensor);
    switch_n_desc_2->AddOutputDesc(output_desc_batch_1);
    switch_n_desc_2->AddOutputDesc(output_desc_batch_2);
    AttrUtils::SetInt(switch_n_desc_2, ATTR_DYNAMIC_TYPE, 1);
    AttrUtils::SetListStr(switch_n_desc_2, ATTR_USER_DESIGNEATE_SHAPE_ORDER, user_input_order);
    auto switch_n_node_2 = graph->AddNode(switch_n_desc_2);

    auto add_desc_1 = std::make_shared<OpDesc>("add_1", ADD);
    add_desc_1->AddInputDesc(scalar_tensor);
    add_desc_1->AddInputDesc(scalar_tensor);
    add_desc_1->AddOutputDesc(scalar_tensor);
    auto add_node_1 = graph->AddNode(add_desc_1);

    auto add_desc_2 = std::make_shared<OpDesc>("add_2", ADD);
    add_desc_2->AddInputDesc(scalar_tensor);
    add_desc_2->AddInputDesc(scalar_tensor);
    add_desc_2->AddOutputDesc(scalar_tensor);
    auto add_node_2 = graph->AddNode(add_desc_2);

    auto identity_switch_1_batch_1 = std::make_shared<OpDesc>("identity_s1_b1", IDENTITY);
    identity_switch_1_batch_1->AddInputDesc(scalar_tensor);
    identity_switch_1_batch_1->AddOutputDesc(scalar_tensor);
    auto identity_node_s1_b1 = graph->AddNode(identity_switch_1_batch_1);

    auto identity_switch_1_batch_2 = std::make_shared<OpDesc>("identity_s1_b2", IDENTITY);
    identity_switch_1_batch_2->AddInputDesc(scalar_tensor);
    identity_switch_1_batch_2->AddOutputDesc(scalar_tensor);
    auto identity_node_s1_b2 = graph->AddNode(identity_switch_1_batch_2);

    auto identity_switch_2_batch_1 = std::make_shared<OpDesc>("identity_s2_b1", IDENTITY);
    identity_switch_2_batch_1->AddInputDesc(scalar_tensor);
    identity_switch_2_batch_1->AddOutputDesc(scalar_tensor);
    auto identity_node_s2_b1 = graph->AddNode(identity_switch_2_batch_1);

    auto identity_switch_2_batch_2 = std::make_shared<OpDesc>("identity_s2_b2", IDENTITY);
    identity_switch_2_batch_2->AddInputDesc(scalar_tensor);
    identity_switch_2_batch_2->AddOutputDesc(scalar_tensor);
    auto identity_node_s2_b2 = graph->AddNode(identity_switch_2_batch_2);

    auto merge_desc = std::make_shared<OpDesc>("Merge", MERGE);
    merge_desc->AddInputDesc(scalar_tensor);
    merge_desc->AddInputDesc(scalar_tensor);
    merge_desc->AddOutputDesc(scalar_tensor);
    merge_desc->AddOutputDesc(int32_tensor);
    AttrUtils::SetBool(merge_desc, ATTR_INSERT_BY_MBATCH, true);
    auto merge_node = graph->AddNode(merge_desc);

    (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), switch_n_node_1->GetInDataAnchor(SWITCH_DATA_INPUT));
    (void)GraphUtils::AddEdge(pred_value_node->GetOutDataAnchor(0), switch_n_node_1->GetInDataAnchor(SWITCH_PRED_INPUT));
    (void)GraphUtils::AddEdge(switch_n_node_1->GetOutDataAnchor(0), add_node_1->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(switch_n_node_1->GetOutDataAnchor(1), add_node_2->GetInDataAnchor(0));

    (void)GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), switch_n_node_2->GetInDataAnchor(SWITCH_DATA_INPUT));
    (void)GraphUtils::AddEdge(pred_value_node->GetOutDataAnchor(0), switch_n_node_2->GetInDataAnchor(SWITCH_PRED_INPUT));
    (void)GraphUtils::AddEdge(switch_n_node_2->GetOutDataAnchor(0), add_node_1->GetInDataAnchor(1));
    (void)GraphUtils::AddEdge(switch_n_node_2->GetOutDataAnchor(1), add_node_2->GetInDataAnchor(1));

    (void)GraphUtils::AddEdge(switch_n_node_1->GetOutDataAnchor(0), identity_node_s1_b1->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(switch_n_node_1->GetOutDataAnchor(1), identity_node_s1_b2->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(switch_n_node_2->GetOutDataAnchor(0), identity_node_s2_b1->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(switch_n_node_2->GetOutDataAnchor(1), identity_node_s2_b2->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(identity_node_s1_b1->GetOutControlAnchor(), add_node_1->GetInControlAnchor());
    (void)GraphUtils::AddEdge(identity_node_s1_b2->GetOutControlAnchor(), add_node_2->GetInControlAnchor());
    (void)GraphUtils::AddEdge(identity_node_s2_b1->GetOutControlAnchor(), add_node_1->GetInControlAnchor());
    (void)GraphUtils::AddEdge(identity_node_s2_b2->GetOutControlAnchor(), add_node_2->GetInControlAnchor());

    (void)GraphUtils::AddEdge(add_node_1->GetOutDataAnchor(0), merge_node->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(add_node_2->GetOutDataAnchor(0), merge_node->GetInDataAnchor(1));
  }

  void make_a_simple_graph(ComputeGraphPtr graph) {
    GeTensorDesc scalar_tensor(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT);

    auto data1_desc = std::make_shared<OpDesc>("data1", DATA);
    data1_desc->AddOutputDesc(scalar_tensor);
    auto data1_node = graph->AddNode(data1_desc);

    auto data2_desc = std::make_shared<OpDesc>("data2", DATA);
    data2_desc->AddOutputDesc(scalar_tensor);
    auto data2_node = graph->AddNode(data2_desc);

    auto add_desc = std::make_shared<OpDesc>("add", ADD);
    add_desc->AddInputDesc(scalar_tensor);
    add_desc->AddInputDesc(scalar_tensor);
    add_desc->AddOutputDesc(scalar_tensor);
    auto add_node = graph->AddNode(add_desc);

    auto output_desc = std::make_shared<OpDesc>("netoutput", NETOUTPUT);
    output_desc->AddInputDesc(scalar_tensor);
    auto output_node = graph->AddNode(output_desc);

    (void)GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
    (void)GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(1));
    (void)GraphUtils::AddEdge(add_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));
  }
};

TEST_F(UtestMultiBatchPass, Run0) {
  Status retStatus;

  DEF_GRAPH(g1) {
    CHAIN(NODE("arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Output", NETOUTPUT));
    CHAIN(NODE("arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  ComputeGraphPtr sgtGraph = ToComputeGraph(g1);

  DEF_GRAPH(g2) {
    CHAIN(NODE("sgt/arg_0", DATA)->NODE("sgt/conv2d", CONV2D)->NODE("sgt/Output", NETOUTPUT));
    CHAIN(NODE("sgt/arg_1", DATA)->NODE("sgt/conv2d"));
  };
  ComputeGraphPtr subGraph = ToComputeGraph(g2);

  const auto parentnode = sgtGraph->FindNode("PartitionedCall_0");
  EXPECT_NE(parentnode, nullptr);

  subGraph->SetParentGraph(sgtGraph);
  subGraph->SetParentNode(parentnode);

  MultiBatchPass multiBatchPass;
  retStatus = multiBatchPass.Run(subGraph);
  EXPECT_EQ(retStatus, SUCCESS);

}

TEST_F(UtestMultiBatchPass, no_switch_n_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_graph_no_switch_n(graph);

  MultiBatchPass multi_batch_pass;
  std::vector<std::pair<string, GraphPass*>> passes = { {"multi_batch_pass", &multi_batch_pass} };
  EXPECT_EQ(multi_batch_pass.Run(graph), domi::SUCCESS);
}

TEST_F(UtestMultiBatchPass, SetCaseLabel)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_graph_two_switch_n(graph);
  ge::OpDescPtr op = CreateOpDesc("While", WHILE, 2, 1);
  op->SetType(STREAMSWITCH);
  op->SetName("stream_switch");
  ge::NodePtr node = graph->AddNode(op);
  MultiBatchPass multiBatchPass;
  Status ret = multiBatchPass.SetCaseLabel(graph, node);
  EXPECT_EQ(ret, SUCCESS);

  AttrUtils::SetInt(op, ATTR_NAME_BATCH_NUM, 1);
  node->GetOpDesc()->AddSubgraphName("cond");
  node->GetOpDesc()->SetSubgraphInstanceName(1, "cond");
  node->GetOpDesc()->AddSubgraphName("body");
  node->GetOpDesc()->SetSubgraphInstanceName(1, "body");
  ComputeGraphPtr subgraph = make_shared<ComputeGraph>("body");
  graph->AddSubGraph(subgraph);
  ret = multiBatchPass.SetCaseLabel(graph, node);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestMultiBatchPass, skip_subgraph)
{
  ComputeGraphPtr root_graph = std::make_shared<ComputeGraph>("root_graph");
  make_a_simple_graph(root_graph);

  ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("sub_graph_test");
  make_a_simple_graph(sub_graph);

  auto add_node = root_graph->FindNode("add");
  EXPECT_NE(add_node, nullptr);

  sub_graph->SetParentGraph(root_graph);
  sub_graph->SetParentNode(add_node);
  add_node->GetOpDesc()->AddSubgraphName("sub_graph");
  add_node->GetOpDesc()->SetSubgraphInstanceName(0, "sub_graph_test");
  root_graph->AddSubgraph("sub_graph", sub_graph);

  MultiBatchPass multi_batch_pass;
  EXPECT_EQ(multi_batch_pass.Run(root_graph), ge::SUCCESS);
}
