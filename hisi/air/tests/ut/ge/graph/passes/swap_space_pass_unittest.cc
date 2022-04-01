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
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/ge_local_context.h"
#include "graph/passes/swap_space_pass.h"
#define private public
#define protected public
#include "graph/build/memory/mem_assigner.h"
#include "graph/build/model_builder.h"
#include "graph/manager/graph_manager.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;
class UtestSwapSpacePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {
    ge::GetThreadLocalContext().SetGraphOption({});
  }
};
namespace {
const std::string OPTION_SWAP_SPACE_NODES = "ge.swapSpaceNodes";
ComputeGraphPtr MakeGraph() {
  DEF_GRAPH(g1) {
    ge::OpDescPtr A;
    A = OP_CFG(DATA)
            .Attr(ATTR_NAME_INDEX, 0)
            .InCnt(1)
            .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
            .OutCnt(1)
            .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
            .Build("A");

    auto B = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("B");
    auto C = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("C");
    auto D = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("D");
    auto E = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("E");
    auto netoutput1 = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput1");

    CHAIN(NODE(A)->NODE(B)->NODE(C)->NODE(D)->NODE(E))->NODE(netoutput1);
    CHAIN(NODE(B)->EDGE(0, 1)->NODE(E));
  };
  return ToComputeGraph(g1);
}
}  // namespace
TEST_F(UtestSwapSpacePass, swap_success_options_test) {
  const auto &graph1 = MakeGraph();
  SwapSpacePass swap_space_pass;
  std::string swap_space_nodes("B");
  std::map<std::string, std::string> options;
  options.emplace(OPTION_SWAP_SPACE_NODES, swap_space_nodes);
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_NE(swap_space_pass.Run(graph1), SUCCESS);
  options[OPTION_SWAP_SPACE_NODES] = "E:0";
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(swap_space_pass.Run(graph1), SUCCESS);
}

TEST_F(UtestSwapSpacePass, swap_success_success) {
  const auto &graph1 = MakeGraph();
  SwapSpacePass swap_space_pass;
  std::string swap_space_nodes("E:0");
  std::map<std::string, std::string> options;
  options.emplace(OPTION_SWAP_SPACE_NODES, swap_space_nodes);
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(swap_space_pass.Run(graph1), SUCCESS);
}