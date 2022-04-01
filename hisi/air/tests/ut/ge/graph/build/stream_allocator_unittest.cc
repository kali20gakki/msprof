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

#include <string>
#include <vector>
#include <gtest/gtest.h>

#define protected public
#define private public
#include "graph/build/stream_allocator.h"
#undef protected
#undef private

#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/graph_builder_utils.h"

namespace ge {
class UtestStreamAllocator : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}

 public:
  ///
  ///    A
  ///   / \.
  ///  B   C
  ///  |   |
  ///  D  400
  ///  |   |
  ///  |   E
  ///   \ /
  ///    F
  ///
  void make_graph_active(const ComputeGraphPtr &graph) {
    const auto &a_desc = std::make_shared<OpDesc>("A", DATA);
    a_desc->AddInputDesc(GeTensorDesc());
    a_desc->AddOutputDesc(GeTensorDesc());
    a_desc->SetStreamId(0);
    const auto &a_node = graph->AddNode(a_desc);

    const auto &b_desc = std::make_shared<OpDesc>("B", "testa");
    b_desc->AddInputDesc(GeTensorDesc());
    b_desc->AddOutputDesc(GeTensorDesc());
    b_desc->SetStreamId(1);
    AttrUtils::SetListStr(b_desc, ATTR_NAME_ACTIVE_LABEL_LIST, {"1"});
    const auto &b_node = graph->AddNode(b_desc);

    const auto &c_desc = std::make_shared<OpDesc>("C", "testa");
    c_desc->AddInputDesc(GeTensorDesc());
    c_desc->AddOutputDesc(GeTensorDesc());
    c_desc->SetStreamId(2);
    AttrUtils::SetStr(c_desc, ATTR_NAME_STREAM_LABEL, "1");
    const auto &c_node = graph->AddNode(c_desc);

    const auto &d_desc = std::make_shared<OpDesc>("D", "testa");
    d_desc->AddInputDesc(GeTensorDesc());
    d_desc->AddOutputDesc(GeTensorDesc());
    d_desc->SetStreamId(1);
    const auto &d_node = graph->AddNode(d_desc);

    const auto &e_desc = std::make_shared<OpDesc>("E", "testa");
    e_desc->AddInputDesc(GeTensorDesc());
    e_desc->AddOutputDesc(GeTensorDesc());
    e_desc->SetStreamId(2);
    const auto &e_node = graph->AddNode(e_desc);

    const auto &f_desc = std::make_shared<OpDesc>("F", "testa");
    f_desc->AddInputDesc(GeTensorDesc());
    f_desc->AddInputDesc(GeTensorDesc());
    f_desc->AddOutputDesc(GeTensorDesc());
    f_desc->SetStreamId(2);
    const auto &f_node = graph->AddNode(f_desc);

    std::vector<NodePtr> node_list(400);
    for (int i = 0; i < 400; i++) {
      const auto &op_desc = std::make_shared<OpDesc>("X", DATA);
      op_desc->AddInputDesc(GeTensorDesc());
      op_desc->AddOutputDesc(GeTensorDesc());
      op_desc->SetStreamId(2);
      node_list[i] = graph->AddNode(op_desc);
    }

    GraphUtils::AddEdge(a_node->GetOutDataAnchor(0), b_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(a_node->GetOutDataAnchor(0), c_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(b_node->GetOutDataAnchor(0), d_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(d_node->GetOutDataAnchor(0), f_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(c_node->GetOutDataAnchor(0), node_list[0]->GetInDataAnchor(0));
    for (uint32_t i = 0; i < 399; i++) {
      GraphUtils::AddEdge(node_list[i]->GetOutDataAnchor(0), node_list[i + 1]->GetInDataAnchor(0));
    }
    GraphUtils::AddEdge(node_list[399]->GetOutDataAnchor(0), e_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(e_node->GetOutDataAnchor(0), f_node->GetInDataAnchor(1));
  }
};

namespace {
ComputeGraphPtr MakeFunctionGraph(const std::string &func_node_name, const std::string &func_node_type) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE(func_node_name, func_node_type)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE(func_node_name));
    CHAIN(NODE("_arg_2", DATA)->NODE(func_node_name));
  };
  return ToComputeGraph(g1);
}
ComputeGraphPtr MakeSubGraph(const std::string &prefix) {
  DEF_GRAPH(g2, prefix.c_str()) {
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    auto conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
    auto relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU));
    auto add_0 = OP_CFG(ADD).Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU));
    CHAIN(NODE(prefix + "_arg_0", data_0)
              ->EDGE(0, 0)
              ->NODE(prefix + "Conv2D", conv_0)
              ->EDGE(0, 0)
              ->NODE(prefix + "Relu", relu_0)
              ->EDGE(0, 0)
              ->NODE(prefix + "Add", add_0)
              ->EDGE(0, 0)
              ->NODE(prefix + "Node_Output", NETOUTPUT));
    CHAIN(NODE(prefix + "_arg_1", data_1)->EDGE(0, 1)->NODE(prefix + "Conv2D", conv_0));
    CHAIN(NODE(prefix + "_arg_2", data_2)->EDGE(0, 1)->NODE(prefix + "Add", add_0));
    CHAIN(NODE(prefix + "_arg_2")->Ctrl()->NODE(prefix + "Add"));
  };
  return ToComputeGraph(g2);
}

void MakeSwitchGraph(ge::ComputeGraphPtr graph, const string &stream_label) {
  auto desc_temp_ptr = make_shared<ge::GeTensorDesc>();
  auto desc_temp = *desc_temp_ptr;
  ge::OpDescPtr op_a = make_shared<ge::OpDesc>("A", DATA);
  op_a->AddInputDesc(desc_temp);
  op_a->AddOutputDesc(desc_temp);
  ge::OpDescPtr op_b = make_shared<ge::OpDesc>("switch", STREAMSWITCH);
  op_b->AddInputDesc(desc_temp);
  op_b->AddOutputDesc(desc_temp);
  ge::OpDescPtr op_c = make_shared<ge::OpDesc>("C", "testa");
  op_c->AddInputDesc(desc_temp);
  op_c->AddOutputDesc(desc_temp);

  ge::NodePtr node_a = graph->AddNode(op_a);
  ge::NodePtr node_b = graph->AddNode(op_b);
  ge::NodePtr node_c = graph->AddNode(op_c);

  ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_b->GetOutControlAnchor(), node_c->GetInControlAnchor());

  node_a->GetOpDesc()->SetStreamId(0);
  node_b->GetOpDesc()->SetStreamId(1);
  node_c->GetOpDesc()->SetStreamId(2);

  (void)AttrUtils::SetListStr(node_b->GetOpDesc(), ATTR_NAME_ACTIVE_LABEL_LIST, {"label1"});
  (void)AttrUtils::SetStr(node_c->GetOpDesc(), ATTR_NAME_STREAM_LABEL, stream_label);
}

void MakeContinuousStreameLableGraph(const ge::ComputeGraphPtr &graph, int node_num, bool diff_stream_flag,
                                     const string &continuous_stream_label) {
  auto desc_temp_ptr = make_shared<ge::GeTensorDesc>();
  auto desc_temp = *desc_temp_ptr;

  std::vector<ge::OpDescPtr> op_desc_list(node_num);

  for (int i = 0; i < node_num; i++) {
    op_desc_list[i] = make_shared<ge::OpDesc>("A", DATA);
    op_desc_list[i]->AddInputDesc(desc_temp);
    op_desc_list[i]->AddOutputDesc(desc_temp);
    if ((i + 10) > node_num) {
      ge::AttrUtils::SetStr(op_desc_list[i], ge::ATTR_NAME_CONTINUOUS_STREAM_LABEL, continuous_stream_label);
    }
  }

  std::vector<ge::NodePtr> node_list(node_num);
  for (int i = 0; i < node_num; i++) {
    node_list[i] = graph->AddNode(op_desc_list[i]);
  }
  for (int i = 0; i <= (node_num - 2); i++) {
    ge::GraphUtils::AddEdge(node_list[i]->GetOutDataAnchor(0), node_list[i + 1]->GetInDataAnchor(0));
  }
  for (int i = 0; i < node_num; i++) {
    if (diff_stream_flag) {
      node_list[i]->GetOpDesc()->SetStreamId(i);
    } else {
      node_list[i]->GetOpDesc()->SetStreamId(0);
    }
  }
}

/*
 * stream id: 0            A--> Send
 *                        /
 * stream id: 1       StreamActiveA
 *                       /
 * stream id: 2      StreamActiveB
 *                       \
 * stream id: 3    Recv-->C
 */

void MakeActiveStreamOptimizeIndirectlyGraph(const ge::ComputeGraphPtr &graph) {
  ge::OpDescPtr op_a = make_shared<ge::OpDesc>("A", DATA);
  auto desc_temp_ptr = make_shared<ge::GeTensorDesc>();
  auto desc_temp = *desc_temp_ptr;
  op_a->AddInputDesc(desc_temp);
  op_a->AddOutputDesc(desc_temp);
  ge::OpDescPtr op_stream_active_a = make_shared<ge::OpDesc>("StreamActiveA", "testa");
  op_stream_active_a->AddInputDesc(desc_temp);
  op_stream_active_a->AddOutputDesc(desc_temp);
  ge::OpDescPtr op_stream_active_b = make_shared<ge::OpDesc>("StreamActiveB", "testa");
  op_stream_active_b->AddInputDesc(desc_temp);
  op_stream_active_b->AddOutputDesc(desc_temp);

  ge::OpDescPtr op_c = make_shared<ge::OpDesc>("C", "testa");
  op_c->AddInputDesc(desc_temp);
  op_c->AddOutputDesc(desc_temp);

  ge::NodePtr node_a = graph->AddNode(op_a);
  ge::NodePtr node_stream_active_a = graph->AddNode(op_stream_active_a);
  ge::NodePtr node_stream_active_b = graph->AddNode(op_stream_active_b);
  ge::NodePtr node_c = graph->AddNode(op_c);

  ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_stream_active_a->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_stream_active_a->GetOutDataAnchor(0), node_stream_active_b->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_stream_active_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));

  node_a->GetOpDesc()->SetStreamId(0);
  node_stream_active_a->GetOpDesc()->SetStreamId(1);
  node_stream_active_b->GetOpDesc()->SetStreamId(2);
  node_c->GetOpDesc()->SetStreamId(3);

  (void)AttrUtils::SetListStr(node_stream_active_a->GetOpDesc(), ATTR_NAME_ACTIVE_LABEL_LIST, {"label2"});
  (void)AttrUtils::SetListStr(node_stream_active_b->GetOpDesc(), ATTR_NAME_ACTIVE_LABEL_LIST, {"label3"});
  (void)AttrUtils::SetStr(node_stream_active_a->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label1");
  (void)AttrUtils::SetStr(node_stream_active_b->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label2");
  (void)AttrUtils::SetStr(node_c->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "label3");
}
}  // namespace

TEST_F(UtestStreamAllocator, SetActiveStreamsForSubgraphs_first_active_node_success) {
  // construct graph
  std::string func_node_name = "PartitionedCall_0";
  const auto &root_graph = MakeFunctionGraph(func_node_name, PARTITIONEDCALL);
  const auto &sub_graph = MakeSubGraph("");
  // set stream_id
  for (int i = 0; i < sub_graph->GetDirectNodesSize(); ++i) {
    auto node = sub_graph->GetDirectNode().at(i);
    if (node->GetName() == "Conv2D") {  // set attr
      (void)AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_SUBGRAPH_FIRST_ACTIVE, true);
    }
    node->GetOpDesc()->SetStreamId(i);
  }

  ut::GraphBuilder::AddPartitionedCall(root_graph, func_node_name, sub_graph);
  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(root_graph, subgraphs);
  EXPECT_EQ(allocator.SetActiveStreamsForSubgraphs(), SUCCESS);
}

TEST_F(UtestStreamAllocator, InsertSyncEvents_success) {
  // construct graph
  std::string func_node_name = "PartitionedCall_0";
  const auto &root_graph = MakeFunctionGraph(func_node_name, PARTITIONEDCALL);
  const auto &sub_graph = MakeSubGraph("");
  // set stream_id
  std::string stream_label = "label0";
  for (int i = 0; i < sub_graph->GetDirectNodesSize(); ++i) {
    auto node = sub_graph->GetDirectNode().at(i);
    node->GetOpDesc()->SetStreamId(i);
  }

  ut::GraphBuilder::AddPartitionedCall(root_graph, func_node_name, sub_graph);
  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(root_graph, subgraphs);
  auto node = sub_graph->FindNode("Conv2D");
  EXPECT_NE(node, nullptr);
  (void)AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_STREAM_LABEL, stream_label);
  std::set<NodePtr> specific_activated_nodes = {node};
  allocator.specific_activated_labels_[stream_label] = specific_activated_nodes;
  auto end_node = sub_graph->FindNode("Node_Output");
  EXPECT_NE(end_node, nullptr);
  (void)AttrUtils::SetBool(end_node->GetOpDesc(), ATTR_NAME_SUBGRAPH_END_NODE, true);
  EXPECT_EQ(allocator.InsertSyncEvents(), SUCCESS);
}

/// Optimization scenario: one stream has multiple send events in one node,
/// and multiple nodes for recv events on another stream
/// Example:
/// Stream0            Stream1
///   N0 - - - event - > N1
///     \                |
///      \               v
///        - - event - > N2
TEST_F(UtestStreamAllocator, OptimizeBySendEvents_success) {
  std::map<int64_t, std::vector<NodePtr>> stream_nodes;
  auto node_0 = std::make_shared<Node>();
  std::vector<NodePtr> stream0_nodes;
  stream0_nodes.push_back(node_0);
  auto node_1 = std::make_shared<Node>();
  auto node_2 = std::make_shared<Node>();
  std::vector<NodePtr> stream1_nodes;
  stream1_nodes.push_back(node_1);
  stream1_nodes.push_back(node_2);
  stream_nodes[0] = stream0_nodes;
  stream_nodes[1] = stream1_nodes;

  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(nullptr, subgraphs);

  uint32_t event_id = 1U;
  allocator.node_to_recv_events_[node_1] = {event_id};
  allocator.node_to_send_events_[node_0] = {event_id++};
  allocator.node_to_recv_events_[node_2] = {event_id};
  allocator.node_to_send_events_[node_0].push_back(event_id);

  EXPECT_EQ(allocator.node_to_send_events_[node_0].size(), 2);
  EXPECT_EQ(allocator.node_to_send_events_[node_0][0], 1);
  EXPECT_EQ(allocator.node_to_send_events_[node_0][1], 2);

  EXPECT_EQ(allocator.OptimizeBySendEvents(stream_nodes), SUCCESS);
  EXPECT_EQ(allocator.node_to_send_events_[node_0].size(), 1);  // event 2 is optimized
  EXPECT_EQ(allocator.node_to_send_events_[node_0][0], 1);
}

/// Scenario: multiple send nodes on a stream sent to a single recv node on the destination stream
/// Example:
/// Stream0            Stream1
///   N0 - -
///   |    |
///   |    - - event - - -
///   |                  |
///   V                  V
///   N1 - - - event - > N2
TEST_F(UtestStreamAllocator, OptimizeByRecvEvents_success) {
  std::map<int64_t, std::vector<NodePtr>> stream_nodes;
  auto node_0 = std::make_shared<Node>();
  std::vector<NodePtr> stream0_nodes;
  stream0_nodes.push_back(node_0);
  auto node_1 = std::make_shared<Node>();
  stream0_nodes.push_back(node_1);
  auto node_2 = std::make_shared<Node>();
  std::vector<NodePtr> stream1_nodes;
  stream1_nodes.push_back(node_2);
  stream_nodes[0] = stream0_nodes;
  stream_nodes[1] = stream1_nodes;

  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(nullptr, subgraphs);

  uint32_t event_id = 1U;
  allocator.node_to_recv_events_[node_2] = {event_id};
  allocator.node_to_send_events_[node_0] = {event_id++};
  allocator.node_to_recv_events_[node_2].push_back(event_id);
  allocator.node_to_send_events_[node_1] = {event_id};

  EXPECT_EQ(allocator.node_to_recv_events_[node_2].size(), 2);
  EXPECT_EQ(allocator.node_to_recv_events_[node_2][0], 1);
  EXPECT_EQ(allocator.node_to_recv_events_[node_2][1], 2);

  EXPECT_EQ(allocator.OptimizeByRecvEvents(stream_nodes), SUCCESS);
  EXPECT_EQ(allocator.node_to_recv_events_[node_2].size(), 1);  // event 1 is optimized
  EXPECT_EQ(allocator.node_to_recv_events_[node_2][0], 2);
}

/// Example:
/// Stream0            Stream1
///   N0 - - - event - > N1
///     \                |
///      \               v
///        - - event - > N2
TEST_F(UtestStreamAllocator, OptimizeByStreamActivate_success) {
  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(nullptr, subgraphs);
  auto graph = MakeSubGraph("");
  auto node_0 = graph->FindNode("Conv2D");
  EXPECT_NE(node_0, nullptr);
  auto node_1 = graph->FindNode("Relu");
  EXPECT_NE(node_1, nullptr);
  auto node_2 = graph->FindNode("Add");
  EXPECT_NE(node_2, nullptr);
  uint32_t event_id = 1U;
  allocator.node_to_recv_events_[node_1] = {event_id};
  allocator.node_to_send_events_[node_0] = {event_id++};
  allocator.node_to_recv_events_[node_2] = {event_id};
  allocator.node_to_send_events_[node_0].push_back(event_id);

  EXPECT_EQ(allocator.node_to_send_events_[node_0].size(), 2);
  EXPECT_EQ(allocator.node_to_send_events_[node_0][0], 1);
  EXPECT_EQ(allocator.node_to_send_events_[node_0][1], 2);

  std::int64_t stream_id = 1;
  node_0->GetOpDesc()->SetStreamId(stream_id++);
  node_1->GetOpDesc()->SetStreamId(stream_id);
  AttrUtils::SetStr(node_1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, std::to_string(stream_id));
  node_2->GetOpDesc()->SetStreamId(stream_id);
  AttrUtils::SetStr(node_2->GetOpDesc(), ATTR_NAME_STREAM_LABEL, std::to_string(stream_id));
  std::set<NodePtr> specific_activated_streams_nodes;
  specific_activated_streams_nodes.insert(node_1);
  specific_activated_streams_nodes.insert(node_2);
  allocator.specific_activated_streams_nodes_map_[stream_id] = specific_activated_streams_nodes;
  EXPECT_EQ(allocator.OptimizeByStreamActivate(), SUCCESS);
}

TEST_F(UtestStreamAllocator, OptimizeByStreamActivate_fail) {
  graphStatus ret = GRAPH_SUCCESS;

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("OptimizeByStreamActivate_fail");
  MakeActiveStreamOptimizeIndirectlyGraph(graph);

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);

  ret = stream_allocator->SetActiveStreamsByLabel();
  EXPECT_EQ(ret, GRAPH_SUCCESS);

  ge::NodePtr node_a = graph->FindNode("A");
  ge::NodePtr node_c = graph->FindNode("C");
  stream_allocator->AddSendEventId(node_a, 0);
  ret = stream_allocator->OptimizeSyncEvents();
  EXPECT_EQ(ret, ge::PARAM_INVALID);
}

TEST_F(UtestStreamAllocator, SplitStreams_with_continuous_stream_label_success) {
  graphStatus ret = GRAPH_SUCCESS;
  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("SplitStreams_CallRtNodeLine_Success_Test");

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);

  // graph with one stream, 355 nodes
  MakeContinuousStreameLableGraph(graph, 345, false, "label0");
  stream_allocator->stream_num_ = 1;

  std::vector<std::set<int64_t>> split_streams(1);
  ret = stream_allocator->SplitStreams(split_streams);
  EXPECT_EQ(ret, GRAPH_SUCCESS);
  int i = 0;
  for (const auto &cur_node : graph->GetDirectNode()) {
    int64_t stream_id = cur_node->GetOpDesc()->GetStreamId();
    if (i < 336) {
      EXPECT_EQ(stream_id, 0);
    } else {
      EXPECT_EQ(stream_id, 1);
    }
    i++;
  }
}

// Iterator loop :
// StreamSwitch  ->  StreamActive
TEST_F(UtestStreamAllocator, FindSwitchNodeBeforeLoopActiveNode_iterator_loop) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("StreamSwitch", STREAMSWITCH)->Ctrl()->NODE("StreamActive", STREAMACTIVE));
  };
  auto graph = ToComputeGraph(g1);
  auto stream_active_node = graph->FindNode("StreamActive");
  (void)AttrUtils::SetBool(stream_active_node->GetOpDesc(), ATTR_NAME_IS_LOOP_ACTIVE, true);
  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);
  stream_allocator->stream_num_ = 2;
  auto ret = stream_allocator->SetActiveStreamsForLoop();
  EXPECT_EQ(ret, SUCCESS);
}
// FpBp loop:
// StreamSwitch  ->  AssignAdd  ->  StreamActive
TEST_F(UtestStreamAllocator, FindSwitchNodeBeforeLoopActiveNode_fpbp_loop) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("StreamSwitch", STREAMSWITCH)
              ->Ctrl()
              ->NODE("AssignAdd", ASSIGNADD)
              ->Ctrl()
              ->NODE("StreamActive", STREAMACTIVE));
  };
  auto graph = ToComputeGraph(g1);
  auto stream_active_node = graph->FindNode("StreamActive");
  (void)AttrUtils::SetBool(stream_active_node->GetOpDesc(), ATTR_NAME_IS_LOOP_ACTIVE, true);
  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);
  stream_allocator->stream_num_ = 2;
  auto ret = stream_allocator->SetActiveStreamsForLoop();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestStreamAllocator, UpdateActiveStreamsForSwitchNodes_success) {
  graphStatus ret = GRAPH_SUCCESS;

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("HandleSwitchNodes_success");
  MakeSwitchGraph(graph, "label1");

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);

  stream_allocator->labeled_streams_["label1"].emplace(3);
  stream_allocator->labeled_streams_["label1"].emplace(4);
  stream_allocator->stream_num_ = 5;
  int64_t stream_num;
  int64_t event_num;
  ret = stream_allocator->RefreshRealStream(stream_num, event_num);
  EXPECT_EQ(ret, GRAPH_SUCCESS);

  ge::NodePtr node_active = graph->FindNode("switch_StreamActive_0");
  auto op_desc = node_active->GetOpDesc();
  int64_t stream_id = op_desc->GetStreamId();
  EXPECT_EQ(stream_id, 5);
}

TEST_F(UtestStreamAllocator, NoNeedUpdateActiveStreamsForSwitchNodes_success) {
  graphStatus ret = GRAPH_SUCCESS;

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("HandleSwitchNodes_success");
  MakeSwitchGraph(graph, "label1");

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);

  stream_allocator->labeled_streams_["label1"].emplace(3);
  stream_allocator->stream_num_ = 4;
  int64_t stream_num;
  int64_t event_num;
  ret = stream_allocator->RefreshRealStream(stream_num, event_num);
  EXPECT_EQ(ret, GRAPH_SUCCESS);
}

TEST_F(UtestStreamAllocator, SetActivatedStreamsList_fail) {
  graphStatus ret = GRAPH_SUCCESS;

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("HandleSwitchNodes_fail");
  MakeSwitchGraph(graph, "label2");

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);
  stream_allocator->labeled_streams_["label1"].emplace(3);
  ge::NodePtr node_a = graph->FindNode("A");

  ret = stream_allocator->SetActiveStreamList(node_a, "label2");
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestStreamAllocator, AddActiveNodes_fail) {
  graphStatus ret = GRAPH_SUCCESS;

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->SetName("HandleSwitchNodes_fail");
  MakeSwitchGraph(graph, "label2");

  Graph2SubGraphInfoList subgraphs;
  std::shared_ptr<ge::StreamAllocator> stream_allocator = make_shared<ge::StreamAllocator>(graph, subgraphs);
  stream_allocator->labeled_streams_["label1"].emplace(3);
  ge::NodePtr node_a = graph->FindNode("A");
  vector<string> ori_active_label_list = {"label3"};
  vector<string> active_label_list;
  vector<NodePtr> added_active_nodes;

  ret = stream_allocator->AddActiveNodes(node_a, ori_active_label_list, active_label_list, added_active_nodes);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestStreamAllocator, test_split_streams_active) {
  const auto &graph = std::make_shared<ComputeGraph>("test_split_streams_active_graph");
  EXPECT_NE(graph, nullptr);
  make_graph_active(graph);

  StreamAllocator allocator(graph, Graph2SubGraphInfoList());
  allocator.stream_num_ = 3;
  EXPECT_EQ(allocator.SetActiveStreamsByLabel(), SUCCESS);
  std::vector<std::set<int64_t>> split_stream(3);
  EXPECT_EQ(allocator.SplitStreams(split_stream), SUCCESS);
  EXPECT_EQ(allocator.UpdateActiveStreams(split_stream), SUCCESS);
  EXPECT_EQ(allocator.SetActiveStreamsForLoop(), SUCCESS);
  EXPECT_EQ(allocator.specific_activated_streams_.count(3), 1);

  const auto &node_b = graph->FindNode("B");
  EXPECT_NE(node_b, nullptr);
  std::vector<uint32_t> active_stream_list;
  EXPECT_TRUE(AttrUtils::GetListInt(node_b->GetOpDesc(), ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list));
  EXPECT_EQ(active_stream_list.size(), 2);
  const auto &node_e = graph->FindNode("E");
  EXPECT_NE(node_e, nullptr);
  EXPECT_EQ(active_stream_list[0], node_e->GetOpDesc()->GetStreamId());
  EXPECT_EQ(active_stream_list[1], 3);
}

TEST_F(UtestStreamAllocator, AssignSingleStream_disable_single_stream_success) {
  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(nullptr, subgraphs);
  allocator.enable_single_stream_ = false;
  EXPECT_EQ(allocator.AssignSingleStream(), SUCCESS);
}

TEST_F(UtestStreamAllocator, AssignSingleStream_stream_num_not_1_fail) {
  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(nullptr, subgraphs);
  allocator.enable_single_stream_ = true;
  allocator.stream_num_ = 2;
  EXPECT_TRUE(allocator.enable_single_stream_);
  EXPECT_NE(allocator.AssignSingleStream(), SUCCESS);
}

TEST_F(UtestStreamAllocator, AssignSingleStream_normal_stream_success) {
  auto desc_temp_ptr = make_shared<ge::GeTensorDesc>();
  auto desc_temp = *desc_temp_ptr;
  ge::OpDescPtr op1 = make_shared<ge::OpDesc>("A", RELU);
  op1->AddInputDesc(desc_temp);
  op1->AddOutputDesc(desc_temp);

  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");
  graph->AddNode(op1);

  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(graph, subgraphs);
  allocator.enable_single_stream_ = true;
  allocator.stream_num_ = 1;
  EXPECT_EQ(allocator.AssignSingleStream(), SUCCESS);
  EXPECT_EQ(allocator.huge_streams_.size(), 0);
}

TEST_F(UtestStreamAllocator, AssignSingleStream_huge_stream_success) {
  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("");

  auto desc_temp_ptr = make_shared<ge::GeTensorDesc>();
  auto desc_temp = *desc_temp_ptr;

  // task count: kTaskNumPerNormalNode=3
  ge::OpDescPtr op1 = make_shared<ge::OpDesc>("A", RELU);
  op1->AddInputDesc(desc_temp);
  op1->AddOutputDesc(desc_temp);
  graph->AddNode(op1);

  // task count: kTaskNumPerHcclNode=245
  for (int i = 0; i < 5; ++i) {
    ge::OpDescPtr op2 = make_shared<ge::OpDesc>("A" + std::to_string(i), HCOMALLREDUCE);
    op2->AddInputDesc(desc_temp);
    op2->AddOutputDesc(desc_temp);
    graph->AddNode(op2);
  }

  Graph2SubGraphInfoList subgraphs;
  StreamAllocator allocator(graph, subgraphs);
  allocator.enable_single_stream_ = true;
  allocator.stream_num_ = 1;
  EXPECT_EQ(allocator.AssignSingleStream(), SUCCESS);
  EXPECT_EQ(allocator.huge_streams_.size(), 1);
  EXPECT_EQ(allocator.huge_streams_.front(), 0);
}

TEST_F(UtestStreamAllocator, test_update_active_streams_for_subgraph) {
  const auto &root_graph = std::make_shared<ComputeGraph>("test_update_active_streams_for_subgraph_root_graph");
  EXPECT_NE(root_graph, nullptr);
  root_graph->SetGraphUnknownFlag(false);
  const auto &sub_graph1 = std::make_shared<ComputeGraph>("test_update_active_streams_for_subgraph_sub_graph1");
  EXPECT_NE(sub_graph1, nullptr);
  root_graph->AddSubGraph(sub_graph1);
  const auto &sub_graph2 = std::make_shared<ComputeGraph>("test_update_active_streams_for_subgraph_sub_graph2");
  EXPECT_NE(sub_graph2, nullptr);
  root_graph->AddSubGraph(sub_graph2);
  const auto &sub_graph3 = std::make_shared<ComputeGraph>("test_update_active_streams_for_subgraph_sub_graph3");
  EXPECT_NE(sub_graph3, nullptr);
  root_graph->AddSubGraph(sub_graph3);

  const auto &case_desc = std::make_shared<OpDesc>("case", CASE);
  EXPECT_NE(case_desc, nullptr);
  EXPECT_EQ(case_desc->AddInputDesc(GeTensorDesc()), GRAPH_SUCCESS);
  EXPECT_EQ(case_desc->AddOutputDesc(GeTensorDesc()), GRAPH_SUCCESS);
  case_desc->AddSubgraphName("branch1");
  case_desc->SetSubgraphInstanceName(0, "test_update_active_streams_for_subgraph_sub_graph1");
  case_desc->AddSubgraphName("branch2");
  case_desc->SetSubgraphInstanceName(1, "test_update_active_streams_for_subgraph_sub_graph2");
  case_desc->AddSubgraphName("branch3");
  case_desc->SetSubgraphInstanceName(2, "test_update_active_streams_for_subgraph_sub_graph3");
  const auto &case_node = root_graph->AddNode(case_desc);
  EXPECT_NE(case_node, nullptr);
  sub_graph1->SetParentNode(case_node);
  sub_graph2->SetParentNode(case_node);
  sub_graph3->SetParentNode(case_node);

  const auto &active_desc1 = std::make_shared<OpDesc>("active1", STREAMACTIVE);
  EXPECT_NE(active_desc1, nullptr);
  active_desc1->SetStreamId(2);
  EXPECT_TRUE(AttrUtils::SetListInt(active_desc1, ATTR_NAME_ACTIVE_STREAM_LIST, {0}));
  const auto &active_node1 = sub_graph1->AddNode(active_desc1);
  EXPECT_NE(active_node1, nullptr);

  const auto &active_desc2 = std::make_shared<OpDesc>("active2", STREAMACTIVE);
  EXPECT_NE(active_desc2, nullptr);
  active_desc2->SetStreamId(3);
  EXPECT_TRUE(AttrUtils::SetListInt(active_desc2, ATTR_NAME_ACTIVE_STREAM_LIST, {1, 5}));
  const auto &active_node2 = sub_graph2->AddNode(active_desc2);
  EXPECT_NE(active_node2, nullptr);
  const auto &relu = std::make_shared<OpDesc>("relu", RELU);
  EXPECT_NE(relu, nullptr);
  relu->SetStreamId(5);
  const auto &relu_node = sub_graph2->AddNode(relu);

  const auto &active_desc3 = std::make_shared<OpDesc>("active3", STREAMACTIVE);
  EXPECT_NE(active_desc3, nullptr);
  active_desc3->SetStreamId(4);
  const auto &active_node3 = sub_graph3->AddNode(active_desc3);
  EXPECT_NE(active_node3, nullptr);

  StreamAllocator allocator(root_graph, Graph2SubGraphInfoList());
  allocator.node_split_stream_map_[active_node1] = 2;
  allocator.node_split_stream_map_[active_node2] = 3;
  allocator.node_split_stream_map_[active_node3] = 4;
  allocator.node_split_stream_map_[relu_node] = 5;
  allocator.split_ori_stream_map_[2] = 0;
  allocator.split_ori_stream_map_[5] = 1;
  allocator.subgraph_first_active_node_map_[sub_graph1] = active_node1;
  allocator.subgraph_first_active_node_map_[sub_graph2] = active_node2;
  allocator.subgraph_first_active_node_map_[sub_graph3] = active_node3;
  EXPECT_EQ(allocator.UpdateActiveStreamsForSubgraphs(), SUCCESS);
  std::vector<uint32_t> active_stream_list1;
  EXPECT_TRUE(AttrUtils::GetListInt(active_node1->GetOpDesc(), ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list1));
  EXPECT_EQ(active_stream_list1.size(), 1);
  EXPECT_EQ(active_stream_list1[0], 0);
  std::vector<uint32_t> active_stream_list2;
  EXPECT_TRUE(AttrUtils::GetListInt(active_node2->GetOpDesc(), ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list2));
  EXPECT_EQ(active_stream_list2.size(), 2);
  EXPECT_EQ(active_stream_list2[0], 1);
  EXPECT_EQ(active_stream_list2[1], 5);
  EXPECT_EQ(allocator.specific_activated_streams_.size(), 3);
  EXPECT_EQ(allocator.specific_activated_streams_.count(3), 1);
  EXPECT_EQ(allocator.AddEventId(case_node, active_node1, active_node2, true), 0);
  std::vector<uint32_t> active_stream_list3;
  EXPECT_EQ(AttrUtils::GetListInt(active_node3->GetOpDesc(), ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list3), false);
  EXPECT_EQ(active_stream_list3.size(), 0);
}

}  // namespace ge
