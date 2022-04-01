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
#include "graph/passes/transop_symmetry_elimination_pass.h"
#include "graph_builder_utils.h"
#include "utils/op_desc_utils.h"

namespace ge {
class UtestTransopSymmetryEliminationPass : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};
namespace {
///      data
///        |
///     transdata1(NCHW->FZ)
///        |
///     transdata2(FZ->NCHW)
///        |
///     netoutput
ComputeGraphPtr BuildGraphTransDataSymm() {
  auto builder = ut::GraphBuilder("test");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto transdata1 = builder.AddNode("transdata1", TRANSDATA, 1, 1);
  auto transdata2 = builder.AddNode("transdata2", TRANSDATA, 1, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  // set transdata1 format&shape
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

  builder.AddDataEdge(data, 0, transdata1, 0);
  builder.AddDataEdge(transdata1, 0, transdata2, 0);
  builder.AddDataEdge(transdata2, 0, netoutput, 0);
  return builder.GetGraph();
}
} // namespace

TEST_F(UtestTransopSymmetryEliminationPass, keep_not_symm_transdata) {
  auto graph = BuildGraphTransDataSymm();
  TransOpSymmetryEliminationPass transop_symmetry_elimination_pass;
  NamesToPass names_to_pass;
  names_to_pass.emplace_back("Test", &transop_symmetry_elimination_pass);
  GEPass pass(graph);
  Status status = pass.Run(names_to_pass);
  EXPECT_EQ(SUCCESS, status);
  // no elimination happend
  EXPECT_EQ(graph->GetAllNodesSize(), 4);
}

TEST_F(UtestTransopSymmetryEliminationPass, test_transposed) {
  auto builder = ut::GraphBuilder("test1");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto transpose1 = builder.AddNode("transpose1", TRANSPOSED, 1, 1);
  auto transpose2 = builder.AddNode("transpose2", TRANSPOSED, 1, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));

  transpose2->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose2->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));

  builder.AddDataEdge(data, 0, transpose1, 0);
  builder.AddDataEdge(transpose1, 0, transpose2, 0);
  builder.AddDataEdge(transpose2, 0, netoutput, 0);

  auto graph = builder.GetGraph();

  TransOpSymmetryEliminationPass transop_symmetry_elimination_pass;
  NamesToPass names_to_pass;
  names_to_pass.emplace_back("Test1", &transop_symmetry_elimination_pass);
  GEPass pass(graph);
  Status status = pass.Run(names_to_pass);
  EXPECT_EQ(SUCCESS, status);
  // no elimination happend
  EXPECT_EQ(graph->GetAllNodesSize(), 2);
}

TEST_F(UtestTransopSymmetryEliminationPass, CheckCanBeEliminated_Test) {
  auto builder = ut::GraphBuilder("test1");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto transpose1 = builder.AddNode("transpose1", RESHAPE, 1, 1);
  auto transpose2 = builder.AddNode("transpose2", RESHAPE, 1, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));

  transpose2->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose2->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));

  builder.AddDataEdge(data, 0, transpose1, 0);
  builder.AddDataEdge(transpose1, 0, transpose2, 0);
  builder.AddDataEdge(transpose2, 0, netoutput, 0);

  auto graph = builder.GetGraph();

  TransOpSymmetryEliminationPass transop_symmetry_elimination_pass;
  NamesToPass names_to_pass;
  names_to_pass.emplace_back("Test1", &transop_symmetry_elimination_pass);
  GEPass pass(graph);
  Status status = pass.Run(names_to_pass);
  EXPECT_EQ(SUCCESS, status);
  // no elimination happend
  EXPECT_EQ(graph->GetAllNodesSize(), 2);
}

TEST_F(UtestTransopSymmetryEliminationPass, DescAreSymmetry_Test) {
  auto builder = ut::GraphBuilder("test1");
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto transpose1 = builder.AddNode("transpose1", CAST, 1, 1);
  auto transpose2 = builder.AddNode("transpose2", CAST, 1, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose1->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));

  transpose2->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
  transpose2->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 3, 224, 224})));
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NHWC);
  transpose2->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({1, 224, 224, 3})));

  builder.AddDataEdge(data, 0, transpose1, 0);
  builder.AddDataEdge(transpose1, 0, transpose2, 0);
  builder.AddDataEdge(transpose2, 0, netoutput, 0);

  auto graph = builder.GetGraph();

  TransOpSymmetryEliminationPass transop_symmetry_elimination_pass;
  NamesToPass names_to_pass;
  names_to_pass.emplace_back("Test1", &transop_symmetry_elimination_pass);
  GEPass pass(graph);
  Status status = pass.Run(names_to_pass);
  EXPECT_EQ(SUCCESS, status);
  // no elimination happend
  EXPECT_EQ(graph->GetAllNodesSize(), 2);
}

}  // namespace ge
