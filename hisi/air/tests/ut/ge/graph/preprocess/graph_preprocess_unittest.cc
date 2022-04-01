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
#include "ge_graph_dsl/graph_dsl.h"

#define private public
#define protected public
#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#include "common/util.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/manager/graph_var_manager.h"

#include "graph/node.h"
#include "graph/preprocess/graph_preprocess.h"
#include "ge/ge_api.h"
#undef private
#undef protected

using namespace std;
namespace ge {
class UtestGraphPreproces : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "") {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({100, 200});
  op_desc->SetOutputOffset({100, 200});
  return op_desc;
}

ComputeGraphPtr BuildGraph1(){
  auto builder = ut::GraphBuilder("g1");
  auto data1 = builder.AddNode("data1",DATA,1,1);
  auto data_opdesc = data1->GetOpDesc();
  AttrUtils::SetInt(data_opdesc, ATTR_NAME_INDEX, 0);
  data1->UpdateOpDesc(data_opdesc);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph2() {
  auto builder = ut::GraphBuilder("g2");
  auto data1 = builder.AddNode("data1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, std::vector<int64_t>({22, -1}));
  ge::AttrUtils::SetStr(data1->GetOpDesc(), ATTR_ATC_USER_DEFINE_DATATYPE, "DT_INT8");
  auto data_opdesc = data1->GetOpDesc();
  AttrUtils::SetInt(data_opdesc, ATTR_NAME_INDEX, 0);

  data1->UpdateOpDesc(data_opdesc);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph3() {
  auto builder = ut::GraphBuilder("g3");
  auto data1 = builder.AddNode("data1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(data1->GetOpDesc(), ATTR_ATC_USER_DEFINE_DATATYPE, "DT_INT8");
  auto data_opdesc = data1->GetOpDesc();
  AttrUtils::SetInt(data_opdesc, ATTR_NAME_INDEX, 0);

  data1->UpdateOpDesc(data_opdesc);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph3Data() {
  auto builder = ut::GraphBuilder("g3");
  auto data1 = builder.AddNode("data1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 1);
  ge::AttrUtils::SetStr(data1->GetOpDesc(), ATTR_ATC_USER_DEFINE_DATATYPE, "DT_INT8");
  auto data_opdesc = data1->GetOpDesc();
  AttrUtils::SetInt(data_opdesc, ATTR_NAME_INDEX, 0);
  AttrUtils::SetStr(data_opdesc, ATTR_ATC_USER_DEFINE_FORMAT, "NC1HWC0");
  builder.AddDataEdge(data1, 0, netoutput, 0);

  data1->UpdateOpDesc(data_opdesc);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph5() {
  auto builder = ut::GraphBuilder("g5");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto add = builder.AddNode("add", ADD, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 1);

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

/*
 *   MapIndex   Data1          subgraph1        subgraph2
 *         \    /
 *          Case      ===>       Data2            Data3
 *           |
 *       Netoutput
 */
ComputeGraphPtr BuildGraph4() {
  auto builder = ut::GraphBuilder("mbatch_Case");

  auto data1 = builder.AddNode("data1", DATA, 1, 1);
  auto data_desc = data1->GetOpDesc();
  AttrUtils::SetStr(data_desc, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  AttrUtils::SetStr(data_desc, "mbatch-switch-name", "case1");
  AttrUtils::SetInt(data_desc, ATTR_NAME_INDEX, 0);

  auto mapindex1 = builder.AddNode("mapindex1", "MapIndex", 0, 1);
  auto case1 = builder.AddNode("case1", CASE, 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", NETOUTPUT, 1, 0);

  builder.AddDataEdge(mapindex1, 0, case1, 0);
  builder.AddDataEdge(data1, 0, case1, 1);
  builder.AddDataEdge(case1, 0, netoutput1, 0);

  return builder.GetGraph();
}

/*
 *   MapIndex   Data1          subgraph1        subgraph2
 *         \    /
 *          Merge      ===>       Data2            Data3
 *           |
 *       Netoutput
 */
ComputeGraphPtr BuildGraph4Data() {
  auto builder = ut::GraphBuilder("mbatch_Case");

  auto data1 = builder.AddNode("data1", DATA, 1, 1);
  auto data_desc = data1->GetOpDesc();
  AttrUtils::SetStr(data_desc, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  AttrUtils::SetStr(data_desc, "mbatch-switch-name", "merge1");
  AttrUtils::SetInt(data_desc, ATTR_NAME_INDEX, 0);

  auto mapindex1 = builder.AddNode("mapindex1", "MapIndex", 0, 1);
  auto case1 = builder.AddNode("merge1", MERGE, 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", NETOUTPUT, 1, 0);
  std::vector<string> output_format_str;
  output_format_str.push_back("0:NC1HWC0");
  std::vector<string> output_datatype_str;
  output_datatype_str.push_back("0:DT_FLOAT16");
  (void)ge::AttrUtils::SetListStr(netoutput1->GetOpDesc(), ATTR_ATC_USER_DEFINE_DATATYPE, output_format_str);
  (void)ge::AttrUtils::SetListStr(netoutput1->GetOpDesc(), ATTR_ATC_USER_DEFINE_FORMAT, output_format_str);
  builder.AddDataEdge(mapindex1, 0, case1, 0);
  builder.AddDataEdge(data1, 0, case1, 1);
  builder.AddDataEdge(case1, 0, netoutput1, 0);

  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph4DynShape() {
  auto builder = ut::GraphBuilder("mbatch_Case");

  auto data1 = builder.AddNode("data1", DATA, 1, 1);
  auto data_desc = data1->GetOpDesc();
  AttrUtils::SetStr(data_desc, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  AttrUtils::SetStr(data_desc, "mbatch-switch-name", "case1");
  AttrUtils::SetInt(data_desc, ATTR_NAME_INDEX, 0);

  auto mapindex1 = builder.AddNode("mapindex1", "MapIndex", 0, 1);
  auto case1 = builder.AddNode("case1", CASE, 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", NETOUTPUT, 1, 0);
  std::vector<std::string> output_format_str;
  output_format_str.push_back("0:NC1HWC0");
  ge::AttrUtils::SetListStr(netoutput1->GetOpDesc(), ATTR_ATC_USER_DEFINE_FORMAT, output_format_str);

  builder.AddDataEdge(mapindex1, 0, case1, 0);
  builder.AddDataEdge(data1, 0, case1, 1);
  builder.AddDataEdge(case1, 0, netoutput1, 0);

  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph4_Subgraph(string graph_name) {
  auto builder = ut::GraphBuilder(graph_name);
  auto data1 = builder.AddNode(graph_name + "_data1", DATA, 1, 1);
  auto data_desc = data1->GetOpDesc();
  AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, 1);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph6() {
  auto builder = ut::GraphBuilder("g6");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {3, -1, -1, 5});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {});
  AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
  AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
  auto add = builder.AddNode("add", ADD, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

ComputeGraphPtr BuildGraph7() {
  auto builder = ut::GraphBuilder("g7");
  auto input1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_ND, DT_FLOAT, {2, 2});
  auto input2 = builder.AddNode("input2", CONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {2, 2});
  AttrUtils::SetInt(input1->GetOpDesc(), ATTR_NAME_INDEX, 0);
  AttrUtils::SetInt(input2->GetOpDesc(), ATTR_NAME_INDEX, 1);
  auto add = builder.AddNode("add", ADD, 2, 1, FORMAT_NCHW, DT_FLOAT, {2, 2});
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(input1, 0, add, 0);
  builder.AddDataEdge(input2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);

  GeTensorDesc tensor_desc(GeShape({1, 2}));
  GeTensor tensor(tensor_desc);
  unique_ptr<uint8_t []> data = unique_ptr<uint8_t []>(new uint8_t[8]);
  tensor.SetData(data.get(), 8);
  AttrUtils::SetTensor(input2->GetOpDesc(), ATTR_NAME_WEIGHTS, tensor);
  (void)AttrUtils::SetBool(add->GetOpDesc(), ATTR_SINGLE_OP_SCENE, true);

  ComputeGraphPtr compute_graph = builder.GetGraph();
  (void)AttrUtils::SetBool(compute_graph, ATTR_SINGLE_OP_SCENE, true);
  return compute_graph;
}

/*
 *
 *  netoutput1
 *    |
 *  merge1
 *   |
 * data1
 */
ComputeGraphPtr BuildGraph8() {
  ut::GraphBuilder builder("g1");
  auto data1 = builder.AddNode("data1", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {-1, 3, 224, 224});
  auto addn1 = builder.AddNode("merge1", MERGE, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 1, 0);

  EXPECT_TRUE(AttrUtils::SetInt(addn1->GetOpDesc(), "TEST_OP_ATTR", 512));
  auto input_desc = addn1->GetOpDesc()->GetInputDesc(0);
  EXPECT_TRUE(AttrUtils::SetInt(input_desc, "TEST_ATTR", 1024));
  addn1->GetOpDesc()->UpdateInputDesc(0, input_desc);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  auto graph = builder.GetGraph();

  graph->SetInputSize(1);
  graph->SetInputsOrder({"data1"});
  graph->AddInputNode(data1);

  return graph;
}

/*
 *
 *  netoutput1
 *    |
 *  while1
 *   |
 * data1
 */
ComputeGraphPtr BuildGraph9() {
  ut::GraphBuilder builder("g1");
  auto data1 = builder.AddNode("data1", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {-1, 3, 224, 224});
  auto addn1 = builder.AddNode("while1", WHILE, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", "NetOutput", 1, 0);

  EXPECT_TRUE(AttrUtils::SetInt(addn1->GetOpDesc(), "TEST_OP_ATTR", 512));
  auto input_desc = addn1->GetOpDesc()->GetInputDesc(0);
  EXPECT_TRUE(AttrUtils::SetInt(input_desc, "TEST_ATTR", 1024));
  addn1->GetOpDesc()->UpdateInputDesc(0, input_desc);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(addn1, 0, netoutput1, 0);
  auto graph = builder.GetGraph();

  graph->SetInputSize(1);
  graph->SetInputsOrder({"data1"});
  graph->AddInputNode(data1);

  return graph;
}

/*
 *
 *   netoutput1
 *       |
 *      add
 *    /    \
 * data1  data2
 */
Graph BuildGraph10() {
  const auto const1 = OP_CFG(CONSTANT)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {16, 16, 224, 224})
      .Build("const1");
  const auto data1 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data1");
  const auto data2 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data2");
  const auto merge = OP_CFG(MERGE)
      .InCnt(1).OutCnt(2)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("merge1");
  const auto net_output = OP_CFG(NETOUTPUT)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("net_output1");
  const auto add = OP_CFG(ADD)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("add");

  DEF_GRAPH(g1) {
                  CHAIN(NODE(data1)->EDGE(0, 0)->NODE(add));
                  CHAIN(NODE(data2)->EDGE(0, 1)->NODE(add)->EDGE(0, 0)->NODE(net_output));
                };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add");
  auto op_desc = node->GetOpDesc();
  op_desc->MutableAllOutputName().erase("__output0");
  op_desc->MutableAllOutputName()["__input0"] = 0;
  return graph;
}

/*
 *
 *   netoutput1
 *       |
 *      add
 *    /    \
 * const1  data2
 */
Graph BuildGraph11() {
  const auto const1 = OP_CFG(CONSTANT)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {16, 16, 224, 224})
      .Build("const1");
  const auto data1 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data1");
  const auto data2 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data2");
  const auto merge = OP_CFG(MERGE)
      .InCnt(1).OutCnt(2)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("merge1");
  const auto net_output = OP_CFG(NETOUTPUT)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("net_output1");
  const auto add = OP_CFG(ADD)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("add");

  DEF_GRAPH(g1) {
                  CHAIN(NODE(const1)->EDGE(0, 0)->NODE(add)->EDGE(0, 0)->NODE(net_output));
                  CHAIN(NODE(data2)->EDGE(0, 1)->NODE(add));
                };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add");
  auto op_desc = node->GetOpDesc();
  op_desc->MutableAllOutputName().erase("__output0");
  op_desc->MutableAllOutputName()["__input0"] = 0;
  return graph;
}

TEST_F(UtestGraphPreproces, test_dynamic_input_shape_parse) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph6();
  // prepare user_input & graph option
  ge::GeTensorDesc tensor1;
  tensor1.SetFormat(ge::FORMAT_NCHW);
  tensor1.SetShape(ge::GeShape({3, 12, 5, 5}));
  tensor1.SetDataType(ge::DT_FLOAT);
  GeTensor input1(tensor1);
  ge::GeTensorDesc tensor2;
  tensor2.SetFormat(ge::FORMAT_NCHW);
  tensor2.SetShape(ge::GeShape());
  tensor2.SetDataType(ge::DT_FLOAT);
  GeTensor input2(tensor2);
  std::vector<GeTensor> user_input = {input1, input2};
  std::map<string,string> graph_option = {{"ge.exec.dynamicGraphExecuteMode","dynamic_execute"},
                                          {"ge.exec.dataInputsShapeRange","[3,1~20,2~10,5],[]"}};
  auto ret = graph_prepare.UpdateInput(user_input, graph_option);
  EXPECT_EQ(ret, ge::SUCCESS);
  // check data1 node output shape_range and shape
  auto data_node = graph_prepare.compute_graph_->FindNode("input1");
  auto data_output_desc = data_node->GetOpDesc()->GetOutputDescPtr(0);
  vector<int64_t> input1_expect_shape = {3,-1,-1,5};
  vector<std::pair<int64_t, int64_t>> intpu1_expect_shape_range = {{3,3},{1,20},{2,10},{5,5}};
  auto input1_result_shape = data_output_desc->GetShape();
  vector<std::pair<int64_t, int64_t>> input1_result_shape_range;
  data_output_desc->GetShapeRange(input1_result_shape_range);
  EXPECT_EQ(input1_result_shape.GetDimNum(), input1_expect_shape.size());
  EXPECT_EQ(input1_result_shape_range.size(), input1_expect_shape.size());
  for(size_t i =0; i< input1_expect_shape.size(); ++i){
      EXPECT_EQ(input1_result_shape.GetDim(i), input1_expect_shape.at(i));
  }
  for(size_t i =0; i< intpu1_expect_shape_range.size(); ++i){
     EXPECT_EQ(input1_result_shape_range.at(i).first, intpu1_expect_shape_range.at(i).first);
     EXPECT_EQ(input1_result_shape_range.at(i).second, intpu1_expect_shape_range.at(i).second);
  }
  // check data2 node output shape_range and shape
  auto data_node_2 = graph_prepare.compute_graph_->FindNode("input2");
  auto data_output_desc_2 = data_node_2->GetOpDesc()->GetOutputDescPtr(0);
  vector<std::pair<int64_t, int64_t>> intput2_result_shape_range;
  data_output_desc_2->GetShapeRange(intput2_result_shape_range);
  EXPECT_EQ(intput2_result_shape_range.size(), 0);
}

TEST_F(UtestGraphPreproces, test_update_input_fail) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph1();

  ge::GeTensorDesc tensor1;
  tensor1.SetFormat(ge::FORMAT_NCHW);
  tensor1.SetShape(ge::GeShape({3, 12, 5, 5}));
  tensor1.SetDataType(ge::DT_UNDEFINED);
  GeTensor input1(tensor1);
  std::vector<GeTensor> user_input = {input1};
  std::map<string,string> graph_option;
  auto ret = graph_prepare.UpdateInput(user_input, graph_option);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(UtestGraphPreproces, test_check_user_input) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph1();

  vector<int64_t> dim = {2, -3};
  GeTensor tensor;
  tensor.SetTensorDesc(GeTensorDesc(GeShape(dim)));
  std::vector<GeTensor> user_input;
  user_input.emplace_back(tensor);

  Status ret = graph_prepare.CheckUserInput(user_input);
  EXPECT_EQ(ret, GE_GRAPH_INIT_FAILED);
}

TEST_F(UtestGraphPreproces, test_update_input_output1) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph3();

  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, check_ref_op_data_succ) {
  GraphPrepare graph_preparer;
  ComputeGraphPtr graph_test = BuildGraph5();
  NodePtr add_node = nullptr;
  for (auto &node : graph_test->GetAllNodes()) {
    if (node->GetName() == "add") {
      add_node = node;
    }
  }
  EXPECT_NE(add_node, nullptr);
  string input_name = "__input0";
  std::set<NodePtr> ref_nodes;
  auto ret = graph_preparer.CheckRefInputNode(add_node, input_name, ref_nodes);
  auto graph = BuildGraph10();
  ret = graph_preparer.Init(graph, 0U, nullptr, nullptr);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, check_ref_op_data_failed) {
  GraphPrepare graph_preparer;
  auto graph = BuildGraph11();
  auto ret = graph_preparer.Init(graph, 0U, nullptr, nullptr);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphPreproces, test_update_dtype_mbatch_case) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph4();
  auto parent_graph = graph_prepare.compute_graph_;
  auto subgraph1 = BuildGraph4_Subgraph("subgraph1");
  auto subgraph2 = BuildGraph4_Subgraph("subgraph2");

  auto data1 = parent_graph->FindNode("data1");
  auto data_desc = data1->GetOpDesc();

  auto case_node = parent_graph->FindNode("case1");
  EXPECT_NE(case_node, nullptr);
  case_node->GetOpDesc()->AddSubgraphName("subgraph1");
  case_node->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph1");
  subgraph1->SetParentNode(case_node);
  subgraph1->SetParentGraph(parent_graph);
  EXPECT_EQ(parent_graph->AddSubgraph("subgraph1", subgraph1), GRAPH_SUCCESS);

  case_node->GetOpDesc()->AddSubgraphName("subgraph2");
  case_node->GetOpDesc()->SetSubgraphInstanceName(1, "subgraph2");
  subgraph2->SetParentNode(case_node);
  subgraph2->SetParentGraph(parent_graph);
  EXPECT_EQ(parent_graph->AddSubgraph("subgraph2", subgraph2), GRAPH_SUCCESS);

  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);

  auto case_desc = case_node->GetOpDesc();
  auto case_input = case_desc->MutableInputDesc(1);
  EXPECT_EQ(case_input->GetDataType(), 1);

  auto sub1_data1 = subgraph1->FindNode("subgraph1_data1");
  EXPECT_NE(sub1_data1, nullptr);
  auto data1_desc = sub1_data1->GetOpDesc();
  auto data1_input = data1_desc->MutableInputDesc(0);
  EXPECT_EQ(data1_input->GetDataType(), 1);
  auto data1_output = data1_desc->MutableOutputDesc(0);
  EXPECT_EQ(data1_output->GetDataType(), 1);
}

TEST_F(UtestGraphPreproces, test_prepare_dyn_shape) {
  ComputeGraphPtr compute_graph = BuildGraph7();
  GraphPtr graph_ptr = std::make_shared<Graph>(GraphUtils::CreateGraphFromComputeGraph(compute_graph));

  GraphNodePtr graph_node = make_shared<GraphNode>(0);
  graph_node->SetComputeGraph(compute_graph);
  graph_node->SetGraph(graph_ptr);

  GeTensorDesc tensor_desc(GeShape({2, 16}));
  GeTensor tensor(tensor_desc);
  std::vector<GeTensor> user_input = {tensor};
  GraphPrepare graph_prepare;
  auto ret = graph_prepare.PrepareInitAndUpdateInput(graph_node, user_input, 0);
  EXPECT_EQ(ret, SUCCESS);
  ret = graph_prepare.PrepareDynShape();
  EXPECT_EQ(ret, SUCCESS);

  auto input2 = compute_graph->FindNode("input2");
  ASSERT_NE(input2, nullptr);
  ASSERT_NE(input2->GetOpDesc(), nullptr);
  auto out_desc = input2->GetOpDesc()->MutableOutputDesc(0);
  EXPECT_EQ(out_desc->GetShape().GetDims(), vector<int64_t>({1, 2}));
  EXPECT_EQ(out_desc->GetFormat(), FORMAT_NCHW);
}

TEST_F(UtestGraphPreproces, test_updar_variable_formats) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 0, 0), SUCCESS);
  auto builder = ut::GraphBuilder("g1");
  auto var = builder.AddNode("var", VARIABLE, 1, 1);
  auto g1 = builder.GetGraph();
  g1->SetSessionID(0);
  TransNodeInfo trans_node_info;
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(g1->GetSessionID())->SetTransRoad(var->GetName(), fusion_road);
  GraphPrepare graph_prepare;
  // trans_node_info.node_type is empty, return fail
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g1), INTERNAL_ERROR);

  auto builder1 = ut::GraphBuilder("g2");
  auto var1 = builder1.AddNode("var1", VARIABLE, 1, 1);
  auto g2 = builder1.GetGraph();
  g2->SetSessionID(0);
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  TransNodeInfo trans_node_info1 = {.node_type = RESHAPE, .input = input_desc, .output = output_desc};
  VarTransRoad fusion_road1;
  fusion_road1.emplace_back(trans_node_info1);
  VarManager::Instance(g2->GetSessionID())->SetTransRoad(var1->GetName(), fusion_road1);
  AttrUtils::SetStr(var1->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "var1");
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g2), SUCCESS);
}

TEST_F(UtestGraphPreproces, RecoverTransRoadForVarRef_TRANSDATA) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 0, 0), SUCCESS);
  GraphPrepare graph_prepare;
  VarTransRoad fusion_road;
  TransNodeInfo trans_node_info;

  auto builder1 = ut::GraphBuilder("g2");
  auto var1 = builder1.AddNode("var1", VARIABLE, 1, 1);
  auto g2 = builder1.GetGraph();
  g2->SetSessionID(0);
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  trans_node_info = {.node_type = TRANSDATA, .input = input_desc, .output = output_desc};
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(g2->GetSessionID())->SetTransRoad(var1->GetName(), fusion_road);
  (void)AttrUtils::SetStr(var1->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "var1");
  (void)AttrUtils::SetStr(var1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "stream1");
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g2), SUCCESS);
}

TEST_F(UtestGraphPreproces, test_updar_variable_formats_insert_transpose) {
  auto builder = ut::GraphBuilder("g1");
  auto var = builder.AddNode("var_transpose", VARIABLE, 1, 1);
  auto g1 = builder.GetGraph();
  g1->SetSessionID(1); // diffenent from pre test case. cause varmanager is instance
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  TransNodeInfo trans_node_info = {.node_type = TRANSDATA, .input = input_desc, .output = output_desc};
  TransNodeInfo transpose_node_info = {.node_type = TRANSPOSED, .input = input_desc, .output = output_desc};
  TransNodeInfo cast_node_info = {.node_type = CAST, .input = input_desc, .output = output_desc};
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  fusion_road.emplace_back(transpose_node_info);
  fusion_road.emplace_back(cast_node_info);
  VarManager::Instance(g1->GetSessionID())->Init(0, 1, 0, 1);
  VarManager::Instance(g1->GetSessionID())->SetTransRoad(var->GetName(), fusion_road);
  GraphPrepare graph_prepare;
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g1), SUCCESS);

  // check new transdata dst_format
}

TEST_F(UtestGraphPreproces, test_update_data_netoutput_by_storageFormat) {
  ComputeGraphPtr compute_graph = BuildGraph5();
  auto set_tensor_desc_nd_to_nz = [](GeTensorDescPtr &desc) {
    desc->SetFormat(FORMAT_ND);
    desc->SetOriginFormat(FORMAT_ND);
    desc->SetShape(GeShape({-1, -1}));
    desc->SetOriginShape(GeShape({-1, -1}));
    AttrUtils::SetInt(desc, ATTR_NAME_STORAGE_FORMAT, static_cast<int64_t>(FORMAT_FRACTAL_NZ));
    vector<int64_t> storage_shape = {1, 1, 1, 16, 16};
    AttrUtils::SetListInt(desc, ATTR_NAME_STORAGE_SHAPE, storage_shape);
  };
  auto set_origin_shape_range = [](GeTensorDescPtr &desc) {
    desc->SetOriginShapeRange({{1, -1}, {1, -1}});
    desc->SetShapeRange({{1, -1}, {1, -1}});
  };

  auto input1 = compute_graph->FindNode("input1");
  ASSERT_NE(input1, nullptr);
  ASSERT_NE(input1->GetOpDesc(), nullptr);
  auto in_desc = input1->GetOpDesc()->MutableInputDesc(0);
  auto out_desc = input1->GetOpDesc()->MutableOutputDesc(0);
  set_tensor_desc_nd_to_nz(in_desc);
  set_tensor_desc_nd_to_nz(out_desc);

  auto net_output = compute_graph->FindNode("netoutput");
  ASSERT_NE(net_output, nullptr);
  ASSERT_NE(net_output->GetOpDesc(), nullptr);
  auto net_output_in_desc = net_output->GetOpDesc()->MutableInputDesc(0);
  auto net_output_out_desc = net_output->GetOpDesc()->MutableOutputDesc(0);
  set_tensor_desc_nd_to_nz(net_output_in_desc);
  set_tensor_desc_nd_to_nz(net_output_out_desc);
  set_origin_shape_range(net_output_in_desc);
  set_origin_shape_range(net_output_out_desc);

  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = compute_graph;
  vector<int64_t> storage_shape = {1, 1, 1, 16, 16};

  // 1. not shape generalized mode
  auto ret = graph_prepare.UpdateDataNetOutputByStorageFormat();
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(in_desc->GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(in_desc->GetShape().GetDims(), storage_shape);

  // 2. format is same to storageFormat
  AttrUtils::SetInt(in_desc, ATTR_NAME_STORAGE_FORMAT, static_cast<int64_t>(FORMAT_ND));
  set_origin_shape_range(in_desc);
  set_origin_shape_range(out_desc);
  ret = graph_prepare.UpdateDataNetOutputByStorageFormat();
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(in_desc->GetFormat(), FORMAT_ND);
  EXPECT_EQ(in_desc->GetShape().GetDims(), storage_shape);

  // 3. transformer
  set_tensor_desc_nd_to_nz(in_desc);
  set_tensor_desc_nd_to_nz(out_desc);
  ret = graph_prepare.UpdateDataNetOutputByStorageFormat();
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(in_desc->GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(in_desc->GetShape().GetDims(), vector<int64_t>({ -1, -1, 16, 16 }));
}

TEST_F(UtestGraphPreproces, test_updar_variable_formats_insert_transdata) {
  auto builder = ut::GraphBuilder("g1");
  auto var = builder.AddNode("var_transdata", VARIABLE, 1, 1);
  auto g1 = builder.GetGraph();
  g1->SetSessionID(1); // diffenent from pre test case. cause varmanager is instance
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  TransNodeInfo trans_node_info = {.node_type = TRANSDATA, .input = input_desc, .output = output_desc};
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(g1->GetSessionID())->Init(0, 1, 0, 1);
  VarManager::Instance(g1->GetSessionID())->SetTransRoad(var->GetName(), fusion_road);
  GraphPrepare graph_prepare;
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g1), SUCCESS);

  // check new transdata dst_format
  for (const auto &node : g1->GetDirectNode()) {
    if (node->GetType() == TRANSDATA) {
      string dst_format;
      AttrUtils::GetStr(node->GetOpDesc(), FORMAT_TRANSFER_DST_FORMAT, dst_format);
      int sub_format = 0;
      AttrUtils::GetInt(node->GetOpDesc(), "groups", sub_format);
      EXPECT_EQ(dst_format, "FRACTAL_Z");
      EXPECT_EQ(sub_format, 240);
    }
  }
}

TEST_F(UtestGraphPreproces, test_UpdateDataInputOutputDesc_skip_refresh) {
  auto builder = ut::GraphBuilder("g1");
  auto data = builder.AddNode("data", DATA, 1, 1, ge::FORMAT_NCHW);
  AttrUtils::SetBool(data->GetOpDesc(), "_skip_refresh_data_format", true);
  auto g1 = builder.GetGraph();
  GeTensorDesc tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NHWC, DT_INT32);

  // pre check
  bool skip_refresh_data = false;
  AttrUtils::GetBool(data->GetOpDesc(), "_skip_refresh_data_format", skip_refresh_data);
  ASSERT_TRUE(skip_refresh_data);
  // test interface
  GraphPrepare graph_prepare;
  auto op_desc = data->GetOpDesc();
  EXPECT_EQ(graph_prepare.UpdateDataInputOutputDesc(0, op_desc, tensor_desc), SUCCESS);

  // after check
  skip_refresh_data = false;
  ASSERT_FALSE(AttrUtils::HasAttr(data->GetOpDesc(), "_skip_refresh_data_format"));
  ASSERT_EQ(data->GetOpDesc()->GetOutputDescPtr(0)->GetFormat(), ge::FORMAT_NCHW);

  // test normal
  EXPECT_EQ(graph_prepare.UpdateDataInputOutputDesc(0, op_desc, tensor_desc), SUCCESS);
  ASSERT_EQ(data->GetOpDesc()->GetOutputDescPtr(0)->GetFormat(), ge::FORMAT_NHWC);

  // test update_op_desc failed, data without input_desc
  auto builder2 = ut::GraphBuilder("g2");
  auto data2 = builder2.AddNode("data", DATA, 0, 1, ge::FORMAT_NCHW);
  auto g2 = builder2.GetGraph();
  auto op_desc2 = data2->GetOpDesc();
  EXPECT_EQ(graph_prepare.UpdateDataInputOutputDesc(0, op_desc2, tensor_desc), GRAPH_FAILED);
}

TEST_F(UtestGraphPreproces, UpdateUninitializedOriginShape) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "file"));
  EXPECT_TRUE(AttrUtils::SetListInt(op_desc, "shape", shape));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  graph->AddNode(op_desc);

  graph_prepare.compute_graph_ = graph;

  for (const NodePtr &node : graph_prepare.compute_graph_->GetAllNodes()) {
    Status ret = graph_prepare.UpdateUninitializedOriginShape(node);
    EXPECT_EQ(ret, SUCCESS);
  }
}

TEST_F(UtestGraphPreproces, SwitchOpOptimize) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "file"));
  EXPECT_TRUE(AttrUtils::SetListInt(op_desc, "shape", shape));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  graph->AddNode(op_desc);
  graph_prepare.compute_graph_ = graph;

  Status ret = graph_prepare.SwitchOpOptimize(graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, GenerateInfershapeGraph) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "file"));
  EXPECT_TRUE(AttrUtils::SetListInt(op_desc, "shape", shape));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  graph->AddNode(op_desc);

  graph_prepare.compute_graph_ = graph;

  const ge::Graph graph_node = ge::GraphUtils::CreateGraphFromComputeGraph(graph);
  ConstGraphPtr const_graph = MakeShared<const ge::Graph>(graph_node);
  Status ret = graph_prepare.GenerateInfershapeGraph(const_graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, NeedUpdateFormatByOutputTypeParm) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph4DynShape();
  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, InitDummyShapeOnControlFlow) {
  ge::GraphPrepare graph_prepare;
  auto compute_graph = BuildGraph8();
  auto ret = graph_prepare.InferShapeForPreprocess(compute_graph, nullptr, nullptr);
  EXPECT_EQ(ret, FAILED);
  compute_graph = BuildGraph9();
  ret = graph_prepare.InferShapeForPreprocess(compute_graph, nullptr, nullptr);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, CheckConstOpFramwordOpCheckInvalid) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("framework_op", FRAMEWORKOP);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, CONSTANT));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  auto node_ptr = graph->AddNode(op_desc);

  graph_prepare.compute_graph_ = graph;
  const auto ret = graph_prepare.CheckConstOp();
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphPreproces, IsDynamicDimsNoPositive) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("framework_op", FRAMEWORKOP);
  std::vector<int64_t> shape = {-1,2,2,2};

  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, CONSTANT));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  auto node_ptr = graph->AddNode(op_desc);
  graph_prepare.options_.input_shape = "2,2,2,2";
  graph_prepare.options_.dynamic_dims = "-1,2,2,2";
  graph_prepare.options_.dynamic_node_type = 1;
  const auto ret = graph_prepare.IsDynamicDims(node_ptr);
  EXPECT_EQ(ret, true);
}

TEST_F(UtestGraphPreproces, CheckUserInputInitFailed) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("data1", DATA);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, -1));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  auto node_ptr = graph->AddNode(op_desc);

  graph_prepare.compute_graph_ = graph;
  const std::vector<GeTensor> user_input;
  const auto ret = graph_prepare.CheckUserInput(user_input);
  EXPECT_EQ(ret, GE_GRAPH_INIT_FAILED);
}

TEST_F(UtestGraphPreproces, TypeConversionOfConstant) {
  ge::GraphPrepare graph_prepare;

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("const1", CONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_SINGLE_OP_SCENE, false));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  auto node_ptr = graph->AddNode(op_desc);

  graph_prepare.compute_graph_ = graph;
  graph_prepare.options_.train_graph_flag = true;
  graph_prepare.TypeConversionOfConstant();
  EXPECT_EQ(node_ptr->GetOpDesc()->GetType(), CONSTANTOP);
  graph_prepare.options_.train_graph_flag = false;
  graph_prepare.TypeConversionOfConstant();
  EXPECT_EQ(node_ptr->GetOpDesc()->GetType(), CONSTANT);
}


TEST_F(UtestGraphPreproces, test_updar_variable_formats_insert_reshape) {
  auto builder = ut::GraphBuilder("g1");
  auto var = builder.AddNode("var_reshape", VARIABLE, 1, 1);
  (void)AttrUtils::SetStr(var->GetOpDesc(), REF_VAR_SRC_VAR_NAME, "var1");
  auto g1 = builder.GetGraph();
  g1->SetSessionID(1); // diffenent from pre test case. cause varmanager is instance
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  TransNodeInfo trans_node_info = {.node_type = RESHAPE, .input = input_desc, .output = output_desc};
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(g1->GetSessionID())->Init(0, 1, 0, 1);
  VarManager::Instance(g1->GetSessionID())->SetTransRoad(var->GetName(), fusion_road);
  GraphPrepare graph_prepare;
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g1), SUCCESS);

  // check new reshape dst_format
  for (const auto &node : g1->GetDirectNode()) {
    if (node->GetType() == RESHAPE) {
      string dst_format;
      AttrUtils::GetStr(node->GetOpDesc(), FORMAT_TRANSFER_DST_FORMAT, dst_format);
      int sub_format = 0;
      AttrUtils::GetInt(node->GetOpDesc(), "groups", sub_format);
      EXPECT_EQ(dst_format, "");
      EXPECT_EQ(sub_format, 0);
    }
  }
  std::vector<int> empty_shape;
  GeTensorDesc output_desc_empty(GeShape(), ge::FORMAT_NCHW, DT_INT32);
  trans_node_info.output = output_desc_empty;
  EXPECT_EQ(graph_prepare.UpdateVariableFormats(g1), SUCCESS);
}

TEST_F(UtestGraphPreproces, TryDoAipp) {
  auto builder = ut::GraphBuilder("g1");
  auto var = builder.AddNode("var", VARIABLE, 1, 1);
  auto g1 = builder.GetGraph();
  g1->SetSessionID(1); // diffenent from pre test case. cause varmanager is instance
  GeTensorDesc input_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(GetFormatFromSub(ge::FORMAT_FRACTAL_Z, 240)), DT_INT32);
  GeTensorDesc output_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_INT32);
  TransNodeInfo trans_node_info = {.node_type = RESHAPE, .input = input_desc, .output = output_desc};
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(g1->GetSessionID())->Init(0, 1, 0, 1);
  VarManager::Instance(g1->GetSessionID())->SetTransRoad(var->GetName(), fusion_road);
  GraphPrepare graph_prepare;
  graph_prepare.options_.train_graph_flag = false;
  graph_prepare.options_.insert_op_file = "op_file";
  graph_prepare.compute_graph_ = g1;
  EXPECT_EQ(graph_prepare.TryDoAipp(), GE_GRAPH_OPTIMIZE_INSERT_OP_PARSE_FAILED);
  graph_prepare.options_.insert_op_file = "\0";
  EXPECT_EQ(graph_prepare.TryDoAipp(), SUCCESS);
}

TEST_F(UtestGraphPreproces, ProcessInputNC1HWC0DynShape) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph3Data();

  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphPreproces, ProcessInputNC1HWC0DynShapeDynBatchFailed) {
  const char *const kMbatchSwitchnName = "mbatch-switch-name";
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph3Data();
  auto data1 = graph_prepare.compute_graph_->FindNode("data1");
  AttrUtils::SetStr(data1->GetOpDesc(), kMbatchSwitchnName, "");
  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestGraphPreproces, ProcessInputNC1HWC0DynShapeDynBatch) {
  const char *const kMbatchSwitchnName = "mbatch-switch-name";
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph3Data();
  auto data1 = graph_prepare.compute_graph_->FindNode("data1");
  AttrUtils::SetStr(data1->GetOpDesc(), kMbatchSwitchnName, "netoutput");
  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);
}


TEST_F(UtestGraphPreproces, test_update_dtype_mbatch_NeedUpdateDtByOutputTypeParm) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = BuildGraph4Data();
  auto parent_graph = graph_prepare.compute_graph_;
  auto subgraph1 = BuildGraph4_Subgraph("subgraph1");
  auto subgraph2 = BuildGraph4_Subgraph("subgraph2");

  auto data1 = parent_graph->FindNode("data1");
  auto data_desc = data1->GetOpDesc();

  auto merge_node = parent_graph->FindNode("merge1");
  EXPECT_NE(merge_node, nullptr);
  merge_node->GetOpDesc()->AddSubgraphName("subgraph1");
  merge_node->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph1");
  subgraph1->SetParentNode(merge_node);
  subgraph1->SetParentGraph(parent_graph);
  EXPECT_EQ(parent_graph->AddSubgraph("subgraph1", subgraph1), GRAPH_SUCCESS);

  merge_node->GetOpDesc()->AddSubgraphName("subgraph2");
  merge_node->GetOpDesc()->SetSubgraphInstanceName(1, "subgraph2");
  subgraph2->SetParentNode(merge_node);
  subgraph2->SetParentGraph(parent_graph);
  EXPECT_EQ(parent_graph->AddSubgraph("subgraph2", subgraph2), GRAPH_SUCCESS);

  Status ret = graph_prepare.UpdateInputOutputByOptions();
  EXPECT_EQ(ret, SUCCESS);
  // need check results
}

TEST_F(UtestGraphPreproces, SaveOriginalGraphToOmModel) {
  ge::GraphPrepare graph_prepare;
  graph_prepare.compute_graph_ = nullptr;
  graph_prepare.options_.save_original_model = "true";
  Status ret = graph_prepare.SaveOriginalGraphToOmModel();
  EXPECT_EQ(ret, SUCCESS);
}

}

