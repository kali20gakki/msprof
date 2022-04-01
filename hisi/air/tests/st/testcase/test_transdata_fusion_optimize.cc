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
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/node_adapter.h"
#include "framework/common/types.h"
#include "graph/utils/op_desc_utils.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "graph/utils/graph_utils.h"

using namespace std;
using namespace ge;

/******************
            abs1
                \
data1-->cast1-->transdata1-->netoutput1
        \
        cast2-->transdata2-->abs----cast1
                /
            abs2
******************/
namespace {
Graph BuildTransopFusionGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast1", CAST));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast2", CAST));
    CHAIN(NODE("cast1", CAST)->EDGE(0, 0)->NODE("transdata1", TRANSDATA));
    CHAIN(NODE("cast2", CAST)->EDGE(0, 0)->NODE("transdata2", TRANSDATA));
    CHAIN(NODE("transdata1", TRANSDATA)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("transdata2", TRANSDATA)->EDGE(0, 0)->NODE("abs", ABSVAL));
    CHAIN(NODE("abs", ABSVAL)->CTRL_EDGE()->NODE("cast1", CAST));
    CHAIN(NODE("abs2", ABSVAL)->CTRL_EDGE()->NODE("transdata2", TRANSDATA));
    CHAIN(NODE("abs1", ABSVAL)->CTRL_EDGE()->NODE("transdata1", TRANSDATA));
  };
  return ToGeGraph(g1);
}

/*
*          noop
*           \\
*  data->transshape-> transdata1->transdata2-> netoutput
*                                                    |
*  data2-> transdata3-> addn1 ->transdata4 __________|
*                                                    |
*  data3->transposed->addn2->transdata5______________|
*/
Graph BuildTransopSymmetryGraph() {
  auto transdata = OP_CFG(TRANSDATA).TensorDesc(FORMAT_FRACTAL_Z, DT_FLOAT, {1,1,16,16}).InCnt(1).OutCnt(1);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)
          ->NODE("transshape", "TransShape")->EDGE(0, 0)
          ->NODE("transdata1", transdata)->EDGE(0, 0)
          ->NODE("transdata2", transdata)->EDGE(0, 0)
          ->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("data2", DATA)->EDGE(0, 0)
          ->NODE("transdata3", transdata)->EDGE(0, 0)
          ->NODE("addn1", ADDN)->EDGE(0, 0)
          ->NODE("transdata4", transdata)->EDGE(0, 1)
          ->NODE("netoutput1"));
    CHAIN(NODE("noop", NOOP)->CTRL_EDGE()
          ->NODE("transshape"));
    
    CHAIN(NODE("data3", DATA)->EDGE(0, 0)
          ->NODE("transposed", "TransposeD")->EDGE(0, 0)
          ->NODE("addn2", ADDN)->EDGE(0, 0)
          ->NODE("transdata5", transdata)->EDGE(0, 2)
          ->NODE("netoutput1", NETOUTPUT));
  };
  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);

  auto data = compute_graph->FindNode("data1");
  auto data_output_desc = data->GetOpDesc()->MutableOutputDesc(0);
  data_output_desc->SetFormat(FORMAT_FRACTAL_Z);  //src format
  data_output_desc->SetShape(GeShape({1, 1, 16, 16})); //src shape
  data_output_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  data_output_desc->SetOriginShape(GeShape({16, 1})); // src orgin shape

  

  auto transshape = compute_graph->FindNode("transshape");
  auto transshape_output_desc = transshape->GetOpDesc()->MutableOutputDesc(0);
  transshape_output_desc->SetFormat(FORMAT_FRACTAL_Z);  //src format
  transshape_output_desc->SetShape(GeShape({1, 1, 16, 16})); //src shape
  transshape_output_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  transshape_output_desc->SetOriginShape(GeShape({16, 1})); // src orgin shape

  auto transdata1 = compute_graph->FindNode("transdata1");
  auto transdata1_input_desc = transdata1->GetOpDesc()->MutableInputDesc(0);
  transdata1_input_desc->SetFormat(FORMAT_FRACTAL_Z);  //src format
  transdata1_input_desc->SetShape(GeShape({1, 1, 16, 16})); //src shape
  transdata1_input_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  transdata1_input_desc->SetOriginShape(GeShape({16, 1})); // src orgin shape
  auto transdata1_output_desc = transdata1->GetOpDesc()->MutableOutputDesc(0);
  transdata1_output_desc->SetFormat(FORMAT_NCHW);
  transdata1_output_desc->SetShape(GeShape({1, 16, 1, 1}));
  transdata1_output_desc->SetOriginFormat(FORMAT_NCHW);
  transdata1_output_desc->SetOriginShape(GeShape({16, 1}));

  auto transdata2 = compute_graph->FindNode("transdata2");
  auto transdata2_input_desc = transdata2->GetOpDesc()->MutableInputDesc(0);
  transdata2_input_desc->SetFormat(FORMAT_NCHW);
  transdata2_input_desc->SetShape(GeShape({16, 1, 1, 1}));
  transdata2_input_desc->SetOriginFormat(FORMAT_NCHW);
  transdata2_input_desc->SetOriginShape(GeShape({16, 1, 1, 1}));
  auto transdata2_output_desc = transdata2->GetOpDesc()->MutableOutputDesc(0);
  transdata2_output_desc->SetFormat(FORMAT_FRACTAL_Z); // dst format
  transdata2_output_desc->SetShape(GeShape({1, 1, 16, 16})); // dst shape
  transdata2_output_desc->SetOriginFormat(FORMAT_NCHW); // dst origin format
  transdata2_output_desc->SetOriginShape(GeShape({16, 1, 1, 1})); //dst origin shape, only orgin shape not symmetry

  auto data2 = compute_graph->FindNode("data2");
  auto data2_output_desc = data2->GetOpDesc()->MutableOutputDesc(0);
  data2_output_desc->SetFormat(FORMAT_FRACTAL_Z);  //src format
  data2_output_desc->SetShape(GeShape({1, 1, 16, 16})); //src shape
  data2_output_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  data2_output_desc->SetOriginShape(GeShape({16, 1})); // src orgin shape

  auto transdata3 = compute_graph->FindNode("transdata3");
  auto transdata3_input_desc = transdata3->GetOpDesc()->MutableInputDesc(0);
  transdata3_input_desc->SetFormat(FORMAT_FRACTAL_Z);  //src format
  transdata3_input_desc->SetShape(GeShape({1, 1, 16, 16})); //src shape
  transdata3_input_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  transdata3_input_desc->SetOriginShape(GeShape({16, 1})); // src orgin shape
  auto transdata3_output_desc = transdata3->GetOpDesc()->MutableOutputDesc(0);
  transdata3_output_desc->SetFormat(FORMAT_NCHW);
  transdata3_output_desc->SetShape(GeShape({1, 16, 1, 1}));
  transdata3_output_desc->SetOriginFormat(FORMAT_NCHW);
  transdata3_output_desc->SetOriginShape(GeShape({16, 1}));

  auto transdata4 = compute_graph->FindNode("transdata4");
  auto transdata4_input_desc = transdata4->GetOpDesc()->MutableInputDesc(0);
  transdata4_input_desc->SetFormat(FORMAT_NCHW);
  transdata4_input_desc->SetShape(GeShape({16, 1, 1, 1}));
  transdata4_input_desc->SetOriginFormat(FORMAT_NCHW);
  transdata4_input_desc->SetOriginShape(GeShape({16, 1, 1, 1}));
  auto transdata4_output_desc = transdata4->GetOpDesc()->MutableOutputDesc(0);
  transdata4_output_desc->SetFormat(FORMAT_FRACTAL_Z); // dst format
  transdata4_output_desc->SetShape(GeShape({1, 1, 16, 16})); // dst shape
  transdata4_output_desc->SetOriginFormat(FORMAT_NCHW); // dst origin format
  transdata4_output_desc->SetOriginShape(GeShape({16, 1})); //dst origin shape

  auto data3 = compute_graph->FindNode("data3");
  auto data3_output_desc = data3->GetOpDesc()->MutableOutputDesc(0);
  data3_output_desc->SetFormat(FORMAT_NCHW);  //src format
  data3_output_desc->SetShape(GeShape({1, 16, 1, 1})); //src shape
  data3_output_desc->SetOriginFormat(FORMAT_NCHW); // src origin format
  data3_output_desc->SetOriginShape(GeShape({1, 16, 1, 1})); // src orgin shape
  
  auto transposed = compute_graph->FindNode("transposed");
  auto transposed_input_desc = transposed->GetOpDesc()->MutableInputDesc(0);
  transposed_input_desc->SetFormat(FORMAT_NCHW);
  transposed_input_desc->SetShape(GeShape({1, 16, 1, 1}));
  transposed_input_desc->SetOriginFormat(FORMAT_NCHW);
  transposed_input_desc->SetOriginShape(GeShape({1, 16, 1, 1}));
  auto transposed_output_desc = transposed->GetOpDesc()->MutableOutputDesc(0);
  transposed_output_desc->SetFormat(FORMAT_NHWC); // dst format
  transposed_output_desc->SetShape(GeShape({1, 1, 1, 16})); // dst shape
  transposed_output_desc->SetOriginFormat(FORMAT_NHWC); // dst origin format
  transposed_output_desc->SetOriginShape(GeShape({1, 1, 1, 16})); //dst origin shape

  auto addn2 = compute_graph->FindNode("addn2");
  auto addn2_output_desc = addn2->GetOpDesc()->MutableOutputDesc(0);
  addn2_output_desc->SetFormat(FORMAT_NHWC);
  addn2_output_desc->SetShape(GeShape({1, 1, 1, 16}));
  addn2_output_desc->SetOriginFormat(FORMAT_NHWC);
  addn2_output_desc->SetOriginShape(GeShape({1, 1, 1, 16}));

  auto transdata5 = compute_graph->FindNode("transdata5");
  auto transdata5_input_desc = transdata5->GetOpDesc()->MutableInputDesc(0);
  transdata5_input_desc->SetFormat(FORMAT_NHWC);
  transdata5_input_desc->SetShape(GeShape({1, 1, 1, 16}));
  transdata5_input_desc->SetOriginFormat(FORMAT_NHWC);
  transdata5_input_desc->SetOriginShape(GeShape({1, 1, 1, 16}));
  auto transdata5_output_desc = transdata5->GetOpDesc()->MutableOutputDesc(0);
  transdata5_output_desc->SetFormat(FORMAT_FRACTAL_Z); // dst format
  transdata5_output_desc->SetShape(GeShape({1, 1, 16, 16})); // dst shape
  transdata5_output_desc->SetOriginFormat(FORMAT_NHWC); // dst origin format
  transdata5_output_desc->SetOriginShape(GeShape({1, 1, 16, 16})); //dst origin shape
  
  return graph;
}

/******************
      abs
        \
data1-->cast1-->netoutput1
        \         /
        cast2----
******************/
Graph BuildTransopBreadthFusionGraph() {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast1", CAST));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast2", CAST));
    CHAIN(NODE("cast1", CAST)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("cast2", CAST)->EDGE(0, 1)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("abs", ABSVAL)->CTRL_EDGE()->NODE("cast1", CAST));
  };
  return ToGeGraph(g1);
}
}  // namespace

class TransopFusionOptimizeTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}

};

TEST_F(TransopFusionOptimizeTest, test_transop_breadth_fusion_pass_normal) {
  Graph test_graph = BuildTransopBreadthFusionGraph();
    
  DUMP_GRAPH_WHEN("OptimizeStage1_1");
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, test_graph, options);
  std::vector<InputTensorInfo> inputs;
  session.BuildGraph(1, inputs);

  CHECK_GRAPH(OptimizeStage1_1) {
    auto ret = graph->TopologicalSorting();
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(graph->GetDirectNodesSize(), 4);
    auto data1 = graph->FindNode("data1");
    auto remain_cast_node = data1->GetOutNodes().at(0);
    EXPECT_EQ(remain_cast_node->GetType(), "Cast");
    EXPECT_EQ(remain_cast_node->GetInControlNodes().size(), 1);
    EXPECT_EQ(remain_cast_node->GetInControlNodes().at(0)->GetName(), "abs");
  };
}

TEST_F(TransopFusionOptimizeTest, test_same_transop_fusion_pass_not_cause_loop) {
  Graph test_graph = BuildTransopFusionGraph();
  
  DUMP_GRAPH_WHEN("OptimizeStage1_1");
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, test_graph, options);
  std::vector<InputTensorInfo> inputs;
  session.BuildGraph(1, inputs);

  CHECK_GRAPH(OptimizeStage1_1) {
    auto ret = graph->TopologicalSorting();
    EXPECT_EQ(ret, SUCCESS);
  };
}

TEST_F(TransopFusionOptimizeTest, test_transop_symmetry_elimination_pass) {
  Graph test_graph = BuildTransopSymmetryGraph();
  
  DUMP_GRAPH_WHEN("OptimizeStage1_2");
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, test_graph, options);
  std::vector<InputTensorInfo> inputs;
  session.BuildGraph(1, inputs);

  CHECK_GRAPH(OptimizeStage1_2) {
    // check transop symmtry fusion
    auto netoutput = graph->FindNode("netoutput1");
    auto transdata = netoutput->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
    auto data2 = netoutput->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
    EXPECT_EQ(transdata->GetName(), "transdata2");
    EXPECT_EQ(data2->GetName(), "data2");

    auto data3 = graph->FindNode("data3");
    EXPECT_EQ(data3->GetOutDataNodes().at(0)->GetType(), TRANSDATA);
  };
}

TEST_F(TransopFusionOptimizeTest, test_framework_transop_breath_fusion_not_fuse) {
  vector<int64_t> perm1{0, 3, 1, 2};
  GeTensorDesc tensor_desc1(GeShape(vector<int64_t>{4}), FORMAT_ND, DT_INT64);
  GeTensorPtr const_tensor1 = 
    std::make_shared<GeTensor>(tensor_desc1, reinterpret_cast<uint8_t *>(perm1.data()) , sizeof(int64_t)*perm1.size());
  auto const1 = OP_CFG(CONSTANT).Weight(const_tensor1);

  vector<int32_t> perm2{0, 2, 1, 3};
  GeTensorDesc tensor_desc2(GeShape(vector<int64_t>{4}), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor2 = 
    std::make_shared<GeTensor>(tensor_desc2, reinterpret_cast<uint8_t *>(perm2.data()), sizeof(int32_t)*perm2.size());
  auto const2 = OP_CFG(CONSTANT).Weight(const_tensor2);

  auto transpose1 = OP_CFG(TRANSPOSE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 128, 52, 52});
  auto transpose2 = OP_CFG(TRANSPOSE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 128, 52, 52});

  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("transpose1", transpose1));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("transpose2", transpose2));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("transpose1", transpose1));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("transpose2", transpose2));
    CHAIN(NODE("transpose1", transpose1)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("transpose2", transpose2)->EDGE(0, 1)->NODE("netoutput1", NETOUTPUT));
  };

  auto graph = ToGeGraph(g1);

  map<string, uint32_t> name_index;
  name_index.emplace("x", 0);
  name_index.emplace("perm", 1);
  for (auto &gn : graph.GetAllNodes()) {
    AscendString type;
    (void)gn.GetType(type);
    if (type == TRANSPOSE) {
      auto node = NodeAdapter::GNode2Node(gn);
      if (node != nullptr && node->GetOpDesc()) {
        node->GetOpDesc()->UpdateInputName(name_index);
      }
    }
  }

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(4, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(4, inputs);
  EXPECT_EQ(ret, FAILED);
  // transpose1 and transpose2 not fusion, the num of nodes not changed
  EXPECT_EQ(graph.GetDirectNode().size(), 6);
}