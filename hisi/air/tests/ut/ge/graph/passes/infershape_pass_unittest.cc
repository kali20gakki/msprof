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
#include "graph/passes/infershape_pass.h"
#undef private public
#undef protected public
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/operator_factory.h"
#include "graph/operator_reg.h"
#include "graph_builder_utils.h"
#include "graph/resource_context_mgr.h"
#include "graph/manager/util/graph_rebuild_state_ctrl.h"

using namespace std;
using namespace testing;
namespace ge {
namespace {
///     data1  const_op
///        \ /              -----------------
///       while   -------- | data_1   data_2 |
///     /     \.           |       \ /       |
///    |      |            |     write2      |
///   read   write1        -----------------
ComputeGraphPtr BuildWhileGraph1() {
  const auto write_infer_func = [](Operator &op) {
    auto inference_context = op.GetInferenceContext();
    inference_context->AddChangedResourceKey("123");
    return GRAPH_SUCCESS;
  };
  const auto read_infer_func = [](Operator &op) {
    auto inference_context = op.GetInferenceContext();
    inference_context->RegisterReliedOnResourceKey("123");
    return GRAPH_SUCCESS;
  };
  // build sub graph
  auto builder_sub = ut::GraphBuilder("sub");
  auto data_1 = builder_sub.AddNode("data_1", DATA, 0, 1);
  auto data_2 = builder_sub.AddNode("data_2", DATA, 0, 1);
  auto write2 =  builder_sub.AddNode("write2", "TensorArrayWrite", 2, 1);
  write2->GetOpDesc()->AddInferFunc(write_infer_func);

  builder_sub.AddDataEdge(data_1, 0, write2, 0);
  builder_sub.AddDataEdge(data_2, 0, write2, 1);
  auto sub_graph = builder_sub.GetGraph();
  sub_graph->SetName("while_sub");
  // build root graph
  auto builder = ut::GraphBuilder("g1");
  auto data = builder.AddNode("data1", DATA, 0, 1);
  auto const_op = builder.AddNode("const_op", CONSTANT, 0, 1);
  auto read = builder.AddNode("read", "TensorArrayRead", 1, 1);
  read->GetOpDesc()->AddInferFunc(read_infer_func);
  auto write1 = builder.AddNode("write1", "TensorArrayWrite", 1, 1);
  write1->GetOpDesc()->AddInferFunc(write_infer_func);
  // add while op
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape({1,1,1,1}));
  tensor_desc->SetFormat(FORMAT_ND);
  tensor_desc->SetDataType(DT_INT32);

  auto op_desc = std::make_shared<OpDesc>("while", WHILE);
  for (int i = 0; i < 2; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < 2; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }
  AttrUtils::SetBool(op_desc,"_need_infer_again", true);
  op_desc->AddSubgraphName(sub_graph->GetName());
  op_desc->SetSubgraphInstanceName(0,sub_graph->GetName());
  auto root_graph = builder.GetGraph();
  auto while_op = root_graph->AddNode(op_desc);

  builder.AddDataEdge(data, 0, while_op, 0);
  builder.AddDataEdge(const_op, 0, while_op, 1);
  builder.AddDataEdge(while_op, 0, read, 0);
  builder.AddDataEdge(while_op, 1, write1, 0);
  sub_graph->SetParentGraph(root_graph);
  sub_graph->SetParentNode(while_op);
  root_graph->AddSubgraph(sub_graph);
  return root_graph;
}
}
class UtestGraphInfershapePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static NodePtr CreateNode(ComputeGraph &graph, const string &name, const string &type, int in_num, int out_num) {
  OpDescPtr op_desc = std::make_shared<OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);
  vector<int64_t> input_offset;
  for (int i = 0; i < in_num; i++) {
    op_desc->AddInputDesc(tensor);
    input_offset.emplace_back(1024);
  }
  op_desc->SetInputOffset(input_offset);

  vector<int64_t> output_offset;
  for (int i = 0; i < out_num; i++) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(1024);
  }
  op_desc->SetOutputOffset(output_offset);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetOpKernelLibName("DNN_VM_RTS_OP_STORE");

  const auto stub_func = [](Operator &op) { return GRAPH_SUCCESS; };
  op_desc->AddInferFunc(stub_func);
  op_desc->AddInferFormatFunc(stub_func);
  op_desc->AddVerifierFunc(stub_func);

  return graph.AddNode(op_desc);
}

TEST_F(UtestGraphInfershapePass, infershape_pass_failed) {
  GeTensorDesc ge_tensor_desc(GeShape({-2, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  string type = "AddN";
  auto addn_op_desc = std::make_shared<OpDesc>("AddN", type);
  addn_op_desc->AddInputDesc(ge_tensor_desc);
  addn_op_desc->AddOutputDesc(ge_tensor_desc);
  auto graph = std::make_shared<ComputeGraph>("test");
  auto addn_node = std::make_shared<Node>(addn_op_desc, graph);
  addn_node->Init();

  InferShapePass infershape_pass;
  EXPECT_EQ(infershape_pass.Run(addn_node), GRAPH_FAILED);
}

TEST_F(UtestGraphInfershapePass, stop_node_for_while_loop) {
/*******************************************************************************
 *      Exit         Identify
 *        \         /       \.
 *         \       /         \.
 *          Switch           Add
 *         /     |            |
 *        /      |            |
 *       /       |            |
 *  LoopCond     |            |
 *      \        |            |
 *       \       |            |
 *        \      |            |
 *       Less    |            |
 *          \    |       NextIteration
 *           \   |            |
 *            \  |            |
 *            Merge <---------|
 *              |
 *              |
 *            Enter
 ******************************************************************************/
  auto graph = std::make_shared<ComputeGraph>("test_infer_shape");
  auto data1 = CreateNode(*graph, "data", DATA, 1, 1);
  auto enter1 = CreateNode(*graph, "enter", ENTER, 1, 1);
  auto merge1 = CreateNode(*graph, "merge", MERGE, 2, 2);
  auto less1 = CreateNode(*graph, "less", LESS, 2, 1);
  auto loop1 = CreateNode(*graph, "loopcond", LOOPCOND, 1, 1);
  auto switch1 = CreateNode(*graph, "switch", SWITCH, 2, 2);
  auto ident1 = CreateNode(*graph, "identity", IDENTITY, 1, 1);
  auto add1 = CreateNode(*graph, "add", ADD, 2, 1);
  auto next1 = CreateNode(*graph, "next", NEXTITERATION, 1, 1);
  auto exit1 = CreateNode(*graph, "exit", EXIT, 1, 1);
  auto value0 = CreateNode(*graph, "const", CONSTANT, 0, 1);
  auto value1 = CreateNode(*graph, "const", CONSTANT, 0, 1);
  auto output1 = CreateNode(*graph, "net_output", NETOUTPUT, 1, 1);

  GraphUtils::AddEdge(data1->GetOutDataAnchor(0), enter1->GetInDataAnchor(0));
  GraphUtils::AddEdge(enter1->GetOutDataAnchor(0), merge1->GetInDataAnchor(0));
  GraphUtils::AddEdge(merge1->GetOutDataAnchor(0), less1->GetInDataAnchor(0));
  GraphUtils::AddEdge(value1->GetOutDataAnchor(0), less1->GetInDataAnchor(1));
  GraphUtils::AddEdge(less1->GetOutDataAnchor(0), loop1->GetInDataAnchor(0));

  GraphUtils::AddEdge(loop1->GetOutDataAnchor(0), switch1->GetInDataAnchor(1));
  GraphUtils::AddEdge(merge1->GetOutDataAnchor(0), switch1->GetInDataAnchor(0));

  GraphUtils::AddEdge(switch1->GetOutDataAnchor(0), exit1->GetInDataAnchor(0));
  GraphUtils::AddEdge(switch1->GetOutDataAnchor(1), ident1->GetInDataAnchor(0));

  GraphUtils::AddEdge(ident1->GetOutDataAnchor(0), add1->GetInDataAnchor(0));
  GraphUtils::AddEdge(value1->GetOutDataAnchor(0), add1->GetInDataAnchor(1));
  GraphUtils::AddEdge(add1->GetOutDataAnchor(0), next1->GetInDataAnchor(0));

  GraphUtils::AddEdge(next1->GetOutDataAnchor(0), merge1->GetInDataAnchor(1));
  GraphUtils::AddEdge(exit1->GetOutDataAnchor(0), output1->GetInDataAnchor(0));

  GEPass ge_passes(graph);
  NamesToPass names_to_passes;
  InferShapePass infer_shape_pass;
  names_to_passes.emplace_back("InferShapePass", &infer_shape_pass);

  EXPECT_EQ(infer_shape_pass.Run(switch1), SUCCESS);
  auto suspend_nodes = infer_shape_pass.GetNodesSuspend();
  auto exit_node = graph->FindNode("exit");
  EXPECT_EQ(suspend_nodes.count(exit_node), 1);
  infer_shape_pass.OnSuspendNodesLeaked();
  auto resume_nodes = infer_shape_pass.GetNodesResume();
  EXPECT_EQ(resume_nodes.count(exit_node), 1);
}
TEST_F(UtestGraphInfershapePass, update_tensordesc_when_changed) {
  GeTensorDesc src_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc dst_ge_tensor_desc(GeShape({2, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDescPtr src_tensor_desc_ptr = std::make_shared<GeTensorDesc>(src_ge_tensor_desc);
  GeTensorDescPtr dst_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  bool changed = false;
  infershape_pass.UpdateTensorDesc(src_tensor_desc_ptr, dst_tensor_desc_ptr, changed);
  EXPECT_EQ(changed, true);
  EXPECT_EQ(dst_tensor_desc_ptr->GetShape().GetDims(), std::vector<int64_t>({1, 2, 3, 4}));
}

TEST_F(UtestGraphInfershapePass, update_tensordesc_when_not_changed) {
  GeTensorDesc src_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDescPtr src_tensor_desc_ptr = std::make_shared<GeTensorDesc>(src_ge_tensor_desc);
  GeTensorDescPtr dst_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  bool changed = false;
  infershape_pass.UpdateTensorDesc(src_tensor_desc_ptr, dst_tensor_desc_ptr, changed);
  EXPECT_EQ(changed, false);
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_failed) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc ge_tensor_desc2(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphs({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr}, dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_get_unknown_rank) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc ge_tensor_desc2(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphs({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr}, dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(dst_ge_tensor_desc_ptr->GetShape().GetDims(), UNKNOWN_RANK);

  std::vector<std::pair<int64_t, int64_t>> ge_tensor_desc1_ptr_shape_range;
  EXPECT_EQ(ge_tensor_desc1_ptr->GetShapeRange(ge_tensor_desc1_ptr_shape_range), GRAPH_SUCCESS);
  EXPECT_EQ(ge_tensor_desc1_ptr_shape_range.empty(), true);
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_get_unknown_shape) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc ge_tensor_desc2(GeShape({2, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphs({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr}, dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(dst_ge_tensor_desc_ptr->GetShape().GetDims(), std::vector<int64_t>({-1,2,3,4}));
  // todo shape range?
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_for_multiDims_failed) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc ge_tensor_desc2(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphsForMultiDims({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr},
                                                                   dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_for_multiDims_failed_shape_size_overflow) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc ge_tensor_desc2(GeShape({INT64_MAX, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphsForMultiDims({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr},
                                                                   dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_for_multiDims_success) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc ge_tensor_desc2(GeShape({2, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = infershape_pass.UpdateOutputFromSubgraphsForMultiDims({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr},
                                                                   dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(dst_ge_tensor_desc_ptr->GetShape().GetDims(), std::vector<int64_t>({2,2,3,4}));
}

TEST_F(UtestGraphInfershapePass, repass_relied_node_if_resource_changed) {
  auto graph = BuildWhileGraph1();
  ResourceContextMgr resource_context_mgr;
  GraphRebuildStateCtrl resource_op_access_ctrl;
  InferShapePass infershape_pass(&resource_op_access_ctrl, &resource_context_mgr);
  auto read_node = graph->FindNode("read");
  EXPECT_EQ(infershape_pass.Run(read_node), SUCCESS);
  auto relied_nodes = resource_context_mgr.MutableNodesReliedOnResource("123");
  EXPECT_EQ(relied_nodes.size(), 1);
  EXPECT_EQ((*relied_nodes.begin())->GetName(), "read");

  // test write1 change resource, read get repassed. both two node in curr graph
  auto write1_node = graph->FindNode("write1");
  EXPECT_EQ(infershape_pass.Run(write1_node), SUCCESS);
  auto need_repass_immediate_set = infershape_pass.GetNodesNeedRePassImmediately();
  EXPECT_EQ(need_repass_immediate_set.size(), 1);
  EXPECT_EQ((*need_repass_immediate_set.begin())->GetName(), "read");

  // test write2 change resource, read get repassed. they are in different graph
  NodePtr write2_node = nullptr;
  for (const auto &node : graph->GetAllNodes()) {
    if (node->GetName() == "write2") {
      write2_node = node;
    }
  }
  EXPECT_NE(write2_node, nullptr);
  EXPECT_EQ(infershape_pass.Run(write2_node), SUCCESS);
  auto global_need_repass_immediate_set = infershape_pass.GetGlobalNodesNeedRePassImmediately();
  EXPECT_EQ(global_need_repass_immediate_set.size(), 1);
  EXPECT_EQ((*global_need_repass_immediate_set.begin())->GetName(), "read");
}

TEST_F(UtestGraphInfershapePass, update_output_from_subgraphs_for_subgraph_multiDims_success) {
  // ref output has different dtype
  GeTensorDesc ge_tensor_desc1(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc ge_tensor_desc2(GeShape({2, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc dst_ge_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, DT_FLOAT);
  GeTensorDescPtr ge_tensor_desc1_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc1);
  GeTensorDescPtr ge_tensor_desc2_ptr = std::make_shared<GeTensorDesc>(ge_tensor_desc2);
  GeTensorDescPtr dst_ge_tensor_desc_ptr = std::make_shared<GeTensorDesc>(dst_ge_tensor_desc);
  InferShapePass infershape_pass;
  auto ret = 
    infershape_pass.UpdateOutputFromSubgraphsForSubgraphMultiDims({ge_tensor_desc1_ptr, ge_tensor_desc2_ptr},
                                                                  dst_ge_tensor_desc_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(dst_ge_tensor_desc_ptr->GetShape().GetDims(), std::vector<int64_t>({-1,2,3,4}));
  std::vector<std::pair<int64_t, int64_t>> range;
  EXPECT_EQ(dst_ge_tensor_desc_ptr->GetShapeRange(range), GRAPH_SUCCESS);
  EXPECT_EQ(range[0].first, 1);
  EXPECT_EQ(range[0].second, 2);
}
}  // namespace ge
