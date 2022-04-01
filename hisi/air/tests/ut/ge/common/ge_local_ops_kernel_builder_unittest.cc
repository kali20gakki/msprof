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
#include "local_engine/ops_kernel_store/ge_local_ops_kernel_builder.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/passes/graph_builder_utils.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {
namespace ge_local{

class UtestGeLocalOpsKernelBuilder : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestGeLocalOpsKernelBuilder, Normal) {
  auto p = std::make_shared<GeLocalOpsKernelBuilder>();
  const std::map<std::string, std::string> options = {};
  EXPECT_EQ(p->Initialize(options), SUCCESS);
  OpDescPtr test_opdesc = std::make_shared<OpDesc>("test", "test");
  OpDescPtr test_opdesc1 = std::make_shared<OpDesc>("test1", "test1");
  OpDescPtr const_opdesc = std::make_shared<OpDesc>("Const", "Constant");
  ComputeGraphPtr test_graph;
  GeTensorDesc te_desc1(GeShape({4, 5, 6, 7}), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc te_desc2(GeShape({4, 5, 6, 7}), FORMAT_NCHW, DT_STRING);
  GeTensorDesc te_desc3(GeShape({-1, 5, 6, 7}), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc te_desc4(GeShape({4, 5, 6, 7}), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc te_desc5(GeShape({-4, 5, 6, 7}), FORMAT_NCHW, DT_FLOAT);
  test_opdesc->AddInputDesc("x", te_desc3);
  NodePtr test_node = std::make_shared<Node>(test_opdesc, test_graph);
  test_opdesc1->AddInputDesc("x", te_desc1);
  test_opdesc1->AddOutputDesc("y1", te_desc1);
  TensorUtils::SetSize(te_desc4, 3360);
  test_opdesc1->AddOutputDesc("y2", te_desc4);
  test_opdesc1->AddOutputDesc("y3", te_desc5);
  NodePtr test_node1 = std::make_shared<Node>(test_opdesc1, test_graph);
  NodePtr test_node2 = std::make_shared<Node>(const_opdesc, test_graph);
  const_opdesc->AddOutputDesc("y", te_desc2);
  Node node1;
  EXPECT_EQ(p->CalcOpRunningParam(node1), FAILED);
  EXPECT_EQ(p->CalcOpRunningParam(*test_node), SUCCESS);
  EXPECT_EQ(p->CalcOpRunningParam(*test_node1), FAILED);
  EXPECT_EQ(p->CalcOpRunningParam(*test_node2), FAILED);
  ConstGeTensorPtr value = make_shared<const GeTensor>();
  int32_t const_value = 0;
  GeTensorPtr weight_value = make_shared<GeTensor>(te_desc1, reinterpret_cast<uint8_t *>(&const_value), sizeof(int32_t));
  AttrUtils::SetTensor(const_opdesc, "value", weight_value);
  OpDescPtr date_opdesc = std::make_shared<OpDesc>("Data", "Data");
  OpDescPtr empty_opdesc = nullptr;
  ComputeGraphPtr const_graph;
  NodePtr const_node = std::make_shared<Node>(const_opdesc, const_graph);
  int64_t outputsize;
  EXPECT_EQ(p->CalcMemSizeByNodeType(empty_opdesc, te_desc1, outputsize, "Constant"), SUCCESS);
  EXPECT_EQ(p->CalcMemSizeByNodeType(empty_opdesc, te_desc2, outputsize, "Constant"), FAILED);
  EXPECT_EQ(p->CalcMemSizeByNodeType(const_opdesc, te_desc2, outputsize, "Constant"), SUCCESS);
  EXPECT_EQ(p->CalcMemSizeByNodeType(const_opdesc, te_desc1, outputsize, "Constant"), SUCCESS);
  EXPECT_EQ(p->CalcMemSizeByNodeType(date_opdesc, te_desc1, outputsize, "Data"), SUCCESS);

  ge::AttrUtils::SetBool(te_desc1, "_tensor_no_tiling_mem_type", true);
  EXPECT_EQ(p->CalcMemSizeByNodeType(date_opdesc, te_desc1, outputsize, "Constant"), SUCCESS);
}

TEST_F(UtestGeLocalOpsKernelBuilder, GenerateTask) {
  auto p = std::make_shared<GeLocalOpsKernelBuilder>();
  RunContext runContext;
  std::vector<domi::TaskDef> tasks;
  OpDescPtr test_opdesc = std::make_shared<OpDesc>("test", "test");
  ComputeGraphPtr test_graph;
  NodePtr test_node = std::make_shared<Node>(test_opdesc, test_graph);
  EXPECT_EQ(p->GenerateTask(*test_node, runContext, tasks), FAILED);
  OpDescPtr test_opdesc1 = std::make_shared<OpDesc>("test", "StackPop");
  NodePtr test_node1 = std::make_shared<Node>(test_opdesc1, test_graph);
  EXPECT_EQ(p->GenerateTask(*test_node1, runContext, tasks), SUCCESS);

  GeTensorDesc te_desc(GeShape({-1, 5, 6, 7}), FORMAT_NCHW, DT_FLOAT);
  test_opdesc->AddInputDesc(te_desc);
  EXPECT_EQ(p->GenerateTask(*test_node, runContext, tasks), SUCCESS);

  OpDescPtr test_opdesc2 = std::make_shared<OpDesc>("test", "NoOp");
  NodePtr test_node2 = std::make_shared<Node>(test_opdesc2, test_graph);
  EXPECT_EQ(p->GenerateTask(*test_node2, runContext, tasks), SUCCESS);

  OpDescPtr test_opdesc3 = std::make_shared<OpDesc>("test", "Reshape");
  NodePtr test_node3 = std::make_shared<Node>(test_opdesc3, test_graph);
  EXPECT_EQ(p->GenerateTask(*test_node3, runContext, tasks), FAILED);
}

TEST_F(UtestGeLocalOpsKernelBuilder, SetSizeForStringInput) {
  auto p = std::make_shared<GeLocalOpsKernelBuilder>();
  DEF_GRAPH(test1) {
    auto add1 = OP_CFG("Add").TensorDesc(FORMAT_ND, DT_STRING, {200, 200, 3})
                           .Attr("_op_max_shape", "200,200,3");
    auto data1 = OP_CFG("Data").TensorDesc(FORMAT_ND, DT_STRING, {0});
    auto data2 = OP_CFG("Data").TensorDesc(FORMAT_ND, DT_STRING, {0});
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
  };

  auto graph = ToGeGraph(test1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  EXPECT_EQ(ge::AttrUtils::SetListInt(node->GetOpDesc(), "_op_max_size", {1000}), true);

  auto node_data = compute_graph->FindNode("data_2");
  GeTensorDesc output_tensor = node_data->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(p->SetSizeForStringInput(*node_data, output_tensor, 0), FAILED);

  EXPECT_EQ(ge::AttrUtils::SetListInt(node->GetOpDesc(), "_op_max_size", {1000, 1000}), true);
  EXPECT_EQ(p->SetSizeForStringInput(*node_data, output_tensor, 0), SUCCESS);
}

}
} // namespace ge
