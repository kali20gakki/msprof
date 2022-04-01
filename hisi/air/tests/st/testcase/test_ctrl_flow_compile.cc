/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "gtest/gtest.h"
#include "ge_graph_dsl/graph_dsl.h"

#include "test_tools_task_info.h"
#include "framework/executor/ge_executor.h"
#include "graph/execute/model_executor.h"
#include "ge/ge_ir_build.h"

using namespace std;
using namespace testing;

namespace ge {
class CtrlFlowCompileTest : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

///
///    net_output
///         |
///       merge
///      /   \
///     /    NEG
///    /      |
/// square   shape(will be fold)
///   F|     T/
///   switch1
///  /       \
/// data     data
///
TEST_F(CtrlFlowCompileTest,  TestSwitchAndMerge) {
  auto pred_node = OP_CFG(DATA).TensorDesc(FORMAT_ND, DT_BOOL, {}).InCnt(1).OutCnt(1);
  auto shape_node = OP_CFG(SHAPE).TensorDesc(FORMAT_ND, DT_INT32, {}).InCnt(1).OutCnt(1);
  DEF_GRAPH(g0) {
    CHAIN(NODE("_arg_0", DATA)->NODE("switch", SWITCH)->EDGE(0, 0)->NODE("shape", shape_node)->NODE("neg0", NEG)->NODE("merge", MERGE)->NODE(
        NODE_NAME_NET_OUTPUT,
        NETOUTPUT));
    CHAIN(NODE("_arg_1", pred_node)->NODE("switch", SWITCH)->EDGE(1, 0)->NODE("neg1", NEG)->NODE("merge", MERGE)->NODE(
        NODE_NAME_NET_OUTPUT,
        NETOUTPUT));
  };

  auto graph = ToGeGraph(g0);
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);
  ModelBufferData model_buffer_data{};

  std::map<string, string> build_options;
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
  aclgrphBuildFinalize();
}
}  // namespace ge