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

#include "graph/passes/start_of_sequence_pass.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "common/ge_inner_error_codes.h"
#include "common/profiling/profiling_properties.h"
#include "graph_builder_utils.h"

namespace ge {
class UtestGraphPassesStartOfSequencePass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};
namespace {
/*
 *         var                         StartOfSequence
 *         |  \                             ||
 *         |   assign                       var
 *         |   //         =======>          |  \
 *      allreduce                           |   assign
 *         |                                |   //
 *      netoutput                        allreduce
 *                                          |
 *                                       netoutput
 */
ComputeGraphPtr BuildGraph() {
  auto builder = ut::GraphBuilder("test");
  const auto var = builder.AddNode("var", VARIABLE, 0, 1);
  const auto assign = builder.AddNode("assign", ASSIGN, 1, 1);
  const auto allreduce = builder.AddNode("allreduce", HCOMALLREDUCE, 1, 1);
  const auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(var, 0, assign, 0);
  builder.AddDataEdge(var, 0, allreduce, 0);
  builder.AddControlEdge(assign, allreduce);
  builder.AddDataEdge(allreduce, 0, netoutput, 0);
  return builder.GetGraph();
}
}  // namespace

TEST(UtestGraphPassesStartOfSequencePass, testInsertStartBeforeVar) {
  ComputeGraphPtr graph = BuildGraph();
  ProfilingProperties::Instance().SetTrainingTrace(true);
  StartOfSequencePass start_of_sequence_pass;
  EXPECT_EQ(start_of_sequence_pass.Run(graph), SUCCESS);
  ProfilingProperties::Instance().SetTrainingTrace(false);

  // check
  const auto dst_node = graph->FindNode("var");
  const auto in_node_before_dst_node = dst_node->GetInControlAnchor()->GetPeerOutControlAnchors().at(0)->GetOwnerNode();
  EXPECT_NE(in_node_before_dst_node, nullptr);
  EXPECT_EQ(in_node_before_dst_node->GetType(), STARTOFSEQUENCE);
}
}  // namespace ge
