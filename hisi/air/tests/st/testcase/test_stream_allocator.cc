/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "graph/utils/op_desc_utils.h"
#include "init/gelib.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"

using namespace std;
using namespace ge;
namespace {
///   Const   Const     Data      Const   Const
///      \     /       /    \        \     /
///      GenMask     Relu   Relu     GenMask
///           \      /         \      /
///            DoMask           DoMask
///                  \         /
///                   NetOutput
Graph BuildGenmaskGraph() {
  int32_t data_value_vec1[1] = {1};
  GeTensorDesc data_tensor_desc(GeShape({1}), FORMAT_ND, DT_INT32);
  GeTensorPtr data_tensor1 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec1, sizeof(int32_t));
  auto const1 = OP_CFG(CONSTANT).Weight(data_tensor1);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->NODE("relu1", RELU)->EDGE(0, 0)->NODE("domask1", DROPOUTDOMASK)->NODE("output", NETOUTPUT));
    CHAIN(NODE("const1", const1)->NODE("DropOutGenMask1", DROPOUTGENMASK)->EDGE(0, 1)->NODE("domask1"));
    CHAIN(NODE("const2", const1)->NODE("DropOutGenMask1"));
    CHAIN(NODE("data1")->NODE("relu2", RELU)->EDGE(0, 0)->NODE("domask2", DROPOUTDOMASK)->NODE("output"));
    CHAIN(NODE("const3", const1)->NODE("DropOutGenMask2", DROPOUTGENMASK)->EDGE(0, 1)->NODE("domask2"));
    CHAIN(NODE("const4", const1)->NODE("DropOutGenMask2"));
  };
  return ToGeGraph(g1);
}

///    Data1    Data2
///       \      /
///    HcomAllReduce1
///       /      \
///    Relu1    Relu2
///       \      /
///      NetOutput
Graph BuildHCCLGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->NODE("hcom1", HCOMALLREDUCE)->NODE("relu1", RELU)->NODE("output", NETOUTPUT));
    CHAIN(NODE("data2", DATA)->NODE("hcom1")->NODE("relu2", RELU)->NODE("output"));
  };
  return ToGeGraph(g1);
}

///   Data1    Data2
///      \      /
///       Switch
///      /      \
///    700      700
///   Relus    Relus
///      \      /
///       Merge
///      /     \
///    Relu   Relu
///      \     /
///     NetOutput
Graph BuildSwitchMergeBigGraph() {
  auto pred_node = OP_CFG(DATA).TensorDesc(FORMAT_ND, DT_BOOL, {}).InCnt(1).OutCnt(1);

  DEF_GRAPH(g1) {
    for (size_t i = 1; i < 700; ++i) {
      std::string name_true_src = "true_relu" + std::to_string(i);
      std::string name_false_src = "false_relu" + std::to_string(i);
      std::string name_true_dst = "true_relu" + std::to_string(i + 1);
      std::string name_false_dst = "false_relu" + std::to_string(i + 1);
      auto relu_true = OP_CFG(RELU).Attr(ATTR_NAME_PARALLEL_GROUP, "true");
      auto relu_false = OP_CFG(RELU).Attr(ATTR_NAME_PARALLEL_GROUP, "false");
      CHAIN(NODE(name_true_src, relu_true)->NODE(name_true_dst, relu_true));
      CHAIN(NODE(name_false_src, relu_false)->NODE(name_false_dst, relu_false));
    }
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("switch1", SWITCH)->EDGE(0, 0)->NODE("true_relu1"));
    CHAIN(NODE("data2", pred_node)->EDGE(0, 1)->NODE("switch1")->EDGE(1, 0)->NODE("false_relu1"));
    CHAIN(NODE("true_relu700", RELU)->EDGE(0, 0)->NODE("merge1", MERGE)->EDGE(0, 0)->NODE("relu1", RELU)
          ->NODE("output", NETOUTPUT));
    CHAIN(NODE("false_relu700", RELU)->EDGE(0, 1)->NODE("merge1")->EDGE(1, 0)->NODE("relu2", RELU)->NODE("output"));
  };
  return ToGeGraph(g1);
}

///   sub_data1            Data1
///       |                  |
///    700Relu   ==>>  PartitionedCall
///       |                  |
///  sub_output1         NetOutput
Graph BuildPartitionedCallGraph() {
  DEF_GRAPH(sub) {
    for (size_t i = 1; i < 700; ++i) {
      std::string sub_relu_src = "sub_relu" + std::to_string(i);
      std::string sub_relu_dst = "sub_relu" + std::to_string(i + 1);
      CHAIN(NODE(sub_relu_src, RELU)->NODE(sub_relu_dst, RELU));
    }
    CHAIN(NODE("sub_relu700")->NODE("hcom1", HCOMALLREDUCE)->NODE("sub_output1", NETOUTPUT));
  };

  DEF_GRAPH(root_graph) {
    CHAIN(NODE("data1", DATA)->NODE("partitionedcall", PARTITIONEDCALL, sub)->NODE("output1", NETOUTPUT));
  };
  sub.Layout();

  return ToGeGraph(root_graph);
}

///  Data1 -- 350Relu -- NetOutput
Graph BuildContinueStreamGraph() {
  DEF_GRAPH(g1) {
    for (size_t i = 1; i < 350; ++i) {
      std::string relu_src = "relu" + std::to_string(i);
      std::string relu_dst = "relu" + std::to_string(i + 1);
      if (i + 20 > 350) {
        auto relu_continue = OP_CFG(RELU).Attr(ATTR_NAME_CONTINUOUS_STREAM_LABEL, "continue");
        CHAIN(NODE(relu_src, relu_continue)->NODE(relu_dst, relu_continue));
      } else {
        CHAIN(NODE(relu_src, RELU)->NODE(relu_dst, RELU));
      }
    }
    CHAIN(NODE("data1", DATA)->NODE("relu1"));
    CHAIN(NODE("relu350")->NODE("output1", NETOUTPUT));
  };
  return ToGeGraph(g1);
}
}

namespace ge {
class STEST_stream_allocator : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(STEST_stream_allocator, link_genmask_nodes) {
  auto graph = BuildGenmaskGraph();

  // new session & add graph
  map<string, string> options;
  options["ge.streamMaxParallelNum"] = "DNN_HCCL:1";
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);

  Session session(options);
  ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 15);
    auto genmask1 = graph->FindNode("DropOutGenMask1");
    EXPECT_NE(genmask1, nullptr);
    auto stream_id = genmask1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
    auto genmask2 = graph->FindNode("DropOutGenMask2");
    EXPECT_NE(genmask2, nullptr);
    stream_id = genmask2->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
  };
}

TEST_F(STEST_stream_allocator, single_stream) {
  auto graph = BuildGenmaskGraph();
  map<string, string> options;
  options["ge.streamMaxParallelNum"] = "DNN_HCCL:10";
  options["ge.enableSingleStream"] = "true";
  GetThreadLocalContext().SetGlobalOption(options);

  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);

  Session session(options);
  ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 9);
    auto genmask1 = graph->FindNode("DropOutGenMask1");
    EXPECT_NE(genmask1, nullptr);
    auto stream_id = genmask1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
    auto genmask2 = graph->FindNode("DropOutGenMask2");
    EXPECT_NE(genmask2, nullptr);
    stream_id = genmask2->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
  };

  options["ge.enableSingleStream"] = "false";
  GetThreadLocalContext().SetGlobalOption(options);
  ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(STEST_stream_allocator, hcom_nodes_independent_stream) {
  auto graph = BuildHCCLGraph();
  map<string, string> options;
  options["ge.hcomParallel"] = "1";

  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 8);
    auto hcom1 = graph->FindNode("hcom1");
    EXPECT_NE(hcom1, nullptr);
    auto stream_id = hcom1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
    auto relu1 = graph->FindNode("relu1");
    EXPECT_NE(relu1, nullptr);
    stream_id = relu1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 1);
  };
}

TEST_F(STEST_stream_allocator, switch_merge_big_graph_split_stream) {
  auto graph =  BuildSwitchMergeBigGraph();
  // new session & add graph
  map<string, string> options;
  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    auto switch1 = graph->FindFirstNodeMatchType(STREAMSWITCH);
    EXPECT_NE(switch1, nullptr);
    auto stream_id = switch1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
    auto merge1 = graph->FindFirstNodeMatchType(STREAMMERGE);
    EXPECT_NE(merge1, nullptr);
    stream_id = merge1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 3);
    auto active1 = graph->FindFirstNodeMatchType(STREAMACTIVE);
    EXPECT_NE(active1, nullptr);
    stream_id = active1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 4);
    auto true_relu700 = graph->FindNode("true_relu700");
    EXPECT_NE(true_relu700, nullptr);
    stream_id = true_relu700->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 8);
    auto false_relu700 = graph->FindNode("false_relu700");
    EXPECT_NE(false_relu700, nullptr);
    stream_id = false_relu700->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 10);
  };
}

TEST_F(STEST_stream_allocator, partitionedcall_with_subgraph) {
  auto graph =  BuildPartitionedCallGraph();
  // new session & add graph
  map<string, string> options;
  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    EXPECT_EQ(graph->GetAllSubgraphs().size(), 1);
    auto subgraph = graph->GetAllSubgraphs().at(0);
    EXPECT_NE(subgraph, nullptr);
    auto active1 = subgraph->FindFirstNodeMatchType(STREAMACTIVE);
    EXPECT_NE(active1, nullptr);
    auto stream_id = active1->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 0);
    std::vector<int64_t> active_stream_list;
    AttrUtils::GetListInt(active1->GetOpDesc(), "active_stream_list", active_stream_list);
    EXPECT_EQ(active_stream_list.size(), 3);
    EXPECT_EQ(active_stream_list[0], 1);
    EXPECT_EQ(active_stream_list[1], 2);
  };
}

TEST_F(STEST_stream_allocator, continue_stream_graph) {
  auto graph = BuildContinueStreamGraph();
  // new session & add graph
  map<string, string> options;
  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    auto relu335 = graph->FindNode("relu335");
    EXPECT_NE(relu335, nullptr);
    auto stream_id = relu335->GetOpDesc()->GetStreamId();
    EXPECT_EQ(stream_id, 1);
  };
}
}  // namespace ge