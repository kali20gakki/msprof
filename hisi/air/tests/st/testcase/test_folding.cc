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
#include <map>
#include <vector>
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/passes/constant_folding_pass.h"
#include "graph/passes/dimension_compute_pass.h"
#include "graph/passes/dimension_adjust_pass.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"

using namespace std;
using namespace ge;

class ConstantFoldingTest : public testing::Test {
 protected:
  void SetUp() {
    std::cout << "Enter constant folding st" << std::endl;
  }
  void TearDown() {
    std::cout << "End constant folding st" << std::endl;
  }
};

static void BuildGatherShapesGraph(std::vector<std::vector<int64_t>> &axes, Graph &graph) {
  auto gathershapes = OP_CFG(GATHERSHAPES)
                          .TensorDesc(FORMAT_NCHW, DT_INT32, {-1})
                          .Attr<std::vector<std::vector<int64_t>>>("axes", axes);
  auto netouput = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_INT32, {-1});
  auto constant_1 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3});
  auto constant_2 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4});

  DEF_GRAPH(g1) {
    CHAIN(NODE("constant_1", constant_1)->EDGE(0, 0)->NODE("gathershapes", gathershapes));
    CHAIN(NODE("constant_2", constant_2)->EDGE(0, 1)->NODE("gathershapes", gathershapes));
    CHAIN(NODE("gathershapes", gathershapes)->EDGE(0, 0)->NODE("netoutput", netouput));
  };

  graph = ToGeGraph(g1);
}

///   constant,constant
///       \     /
///    gathershapes
///          |
///      netoutput
TEST_F(ConstantFoldingTest, test_gathershapes_folding) {
  std::vector<std::vector<int64_t>> axes = {{1, 0}, {0, 1}};
  Graph graph;
  BuildGatherShapesGraph(axes, graph);
  auto all_gnodes = graph.GetDirectNode();
  for (GNode &gnode : all_gnodes) {
    AscendString node_name;
    (void)gnode.GetName(node_name);
    if (node_name == "gathershapes") {
      TensorDesc out_desc;
      out_desc.SetDataType((DataType)DT_INT32);
      gnode.UpdateOutputDesc(0, out_desc);
    }
  }

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    ASSERT_EQ(graph->GetDirectNode().size(), 2);
  };
}

///   constant,constant
///       \     /
///    gathershapes
///          |
///      netoutput
TEST_F(ConstantFoldingTest, test_gathershapes_folding_fail) {
  std::vector<std::vector<int64_t>> axes = {{1, 0}, {0, 8}};
  Graph graph;
  BuildGatherShapesGraph(axes, graph);
  auto all_gnodes = graph.GetDirectNode();
  for (GNode &gnode : all_gnodes) {
    AscendString node_name;
    (void)gnode.GetName(node_name);
    if (node_name == "gathershapes") {
      TensorDesc out_desc;
      out_desc.SetDataType((DataType)DT_INT32);
      gnode.UpdateOutputDesc(0, out_desc);
    }
  }

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, FAILED);
}

///      constant            constant
///       |                    |
///    flattenv2     -->     netoutput
///        |
///      netoutput
TEST_F(ConstantFoldingTest, test_flattenv2_folding) {
  int64_t axis = 1;
  int64_t end_axis = -1;
  GeTensor weight;
  std::vector<uint8_t> data = {1, 2};
  weight.SetData(data);
  GeTensorDesc weight_desc;
  weight_desc.SetShape(GeShape({2, 3, 4}));
  weight_desc.SetOriginShape(GeShape({2, 3, 4}));
  weight.SetTensorDesc(weight_desc);
  auto constant =
      OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4}).Attr<GeTensor>(ATTR_NAME_WEIGHTS, weight);
  auto flattenv2 =
      OP_CFG(FLATTENV2).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3}).Attr("axis", axis).Attr("end_axis", end_axis);
  auto netouput = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_INT32, {-1});
  DEF_GRAPH(g1) {
    CHAIN(NODE("constant", constant)->EDGE(0, 0)->NODE("flattenv2", flattenv2));
    CHAIN(NODE("flattenv2", flattenv2)->EDGE(0, 0)->NODE("netoutput", netouput));
  };

  auto graph = ToGeGraph(g1);

  auto compute_graph = GraphUtils::GetComputeGraph(graph);

  auto node_ptr = compute_graph->FindNode("flattenv2");
  auto op_desc_ptr = node_ptr->GetOpDesc();
  op_desc_ptr->SetOpKernelLibName("aicpu_ascend_kernel");
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(compute_graph, "test_flattenv2");
  std::map<string, uint32_t> name_idx_map = {{"x", 0}};
  op_desc_ptr->UpdateInputName(name_idx_map);
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(2, graph_2, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(2, inputs);
  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_2");
    ASSERT_EQ(graph->GetDirectNode().size(), 2);
  };
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
}

//  data   constant
//   \         /
//    \       /
//     squeezev3
//       |
//      netoutput
TEST_F(ConstantFoldingTest, test_squeezev3_folding) {
  int64_t axis = 1;
  int64_t end_axis = -1;
  GeTensor weight;
  std::vector<uint8_t> data = {1, 2};
  weight.SetData(data);
  GeTensorDesc weight_desc;
  weight_desc.SetShape(GeShape({2, 3, 4}));
  weight_desc.SetOriginShape(GeShape({2, 3, 4}));
  weight.SetTensorDesc(weight_desc);

  auto data_x = OP_CFG(DATA)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 2, 3, 4})
        .InCnt(1)
        .OutCnt(1);

  auto squeezev3 =
      OP_CFG(SQUEEZEV3).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3});
  std::vector<uint8_t> data_2 = {0};    
  weight.SetData(data_2);
  auto constant_axes =
      OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {1}).Attr<GeTensor>(ATTR_NAME_WEIGHTS, weight);

  auto netouput = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_INT32, {-1});
  DEF_GRAPH(graph_squeezev3) {
    CHAIN(NODE("data_x", data_x)->EDGE(0, 0)->NODE("squeezev3", squeezev3));
    CHAIN(NODE("constant_axes",constant_axes)->EDGE(0,1)->NODE("squeezev3", squeezev3));
    CHAIN(NODE("squeezev3", squeezev3)->EDGE(0, 0)->NODE("netoutput", netouput));
  };

  auto graph = ToGeGraph(graph_squeezev3);

  auto compute_graph = GraphUtils::GetComputeGraph(graph);

  auto node_ptr = compute_graph->FindNode("squeezev3");
  auto op_desc_ptr = node_ptr->GetOpDesc();
  std::map<string, uint32_t> name_idx_map = {{"x", 0}};
  op_desc_ptr->UpdateInputName(name_idx_map);
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph_2, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "graph_squeezev3_" + pne_name + "_1");
    EXPECT_EQ(graph->GetDirectNode().size(), 2);
  };
}

//  data   constant
//   \         /
//    \       /
//     unsqueezev3
//       |
//     netoutput
TEST_F(ConstantFoldingTest, test_unsqueezev3_folding) {
  int64_t axis = 1;
  int64_t end_axis = -1;
  GeTensor weight;
  std::vector<uint8_t> data = {1, 2};
  weight.SetData(data);
  GeTensorDesc weight_desc;
  weight_desc.SetShape(GeShape({2, 3, 4}));
  weight_desc.SetOriginShape(GeShape({2, 3, 4}));
  weight.SetTensorDesc(weight_desc);

  auto data_x = OP_CFG(DATA)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 2, 3, 4})
        .InCnt(1)
        .OutCnt(1);

  auto unsqueezev3 =
      OP_CFG(UNSQUEEZEV3).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3});
  std::vector<uint8_t> data_2 = {0};    
  weight.SetData(data_2);
  auto constant_axes =
      OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {1}).Attr<GeTensor>(ATTR_NAME_WEIGHTS, weight);

  auto netouput = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_INT32, {-1});
  DEF_GRAPH(graph_unsqueezev3) {
    CHAIN(NODE("data_x", data_x)->EDGE(0, 0)->NODE("unsqueezev3", unsqueezev3));
    CHAIN(NODE("constant_axes",constant_axes)->EDGE(0,1)->NODE("unsqueezev3", unsqueezev3));
    CHAIN(NODE("unsqueezev3", unsqueezev3)->EDGE(0, 0)->NODE("netoutput", netouput));
  };

  auto graph = ToGeGraph(graph_unsqueezev3);

  auto compute_graph = GraphUtils::GetComputeGraph(graph);

  auto node_ptr = compute_graph->FindNode("unsqueezev3");
  auto op_desc_ptr = node_ptr->GetOpDesc();
  std::map<string, uint32_t> name_idx_map = {{"x", 0}};
  op_desc_ptr->UpdateInputName(name_idx_map);
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph_2, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "graph_unsqueezev3_" + pne_name + "_1");
    EXPECT_EQ(graph->GetDirectNode().size(), 2);
  };
}