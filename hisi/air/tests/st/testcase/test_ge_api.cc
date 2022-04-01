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
#define protected public
#define private public
#include "common/plugin/ge_util.h"
#include "proto/ge_ir.pb.h"
#include "external/ge/ge_api.h"
#include "session/session_manager.h"
#include "init_ge.h"
#undef private
#undef protected

namespace ge {
class GeApiTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(GeApiTest, ge_session_test) {
  std::map<std::string, std::string> options;
  EXPECT_EQ(GEInitialize(options), SUCCESS);

  std::map<AscendString, AscendString> ascend_options;
  Session session(options);

  GraphId graph_id = 1;
  const auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);
  EXPECT_EQ(session.AddGraph(graph_id, graph, ascend_options), SUCCESS);

  EXPECT_EQ(session.AddGraphWithCopy(graph_id, graph), FAILED);
  EXPECT_EQ(session.AddGraphWithCopy(graph_id, graph, ascend_options), FAILED);

  vector<Tensor> inputs;
  vector<InputTensorInfo> tensors;
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), FAILED);
  EXPECT_EQ(session.BuildGraph(graph_id, tensors), FAILED);

  vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), FAILED);
  EXPECT_EQ(session.RunGraphWithStreamAsync(graph_id, nullptr, inputs, outputs), FAILED);
  EXPECT_EQ(session.RunGraphAsync(graph_id, inputs, nullptr), SUCCESS); // Push to queue.

  vector<string> var_inputs;
  EXPECT_EQ(session.GetVariables(var_inputs, outputs), FAILED);

  vector<AscendString> var_names;
  EXPECT_EQ(session.GetVariables(var_names, outputs), FAILED);

  std::string key;
  pCallBackFunc ge_callback;
  EXPECT_EQ(session.RegisterCallBackFunc(key, ge_callback), SUCCESS);

  session::pCallBackFunc session_callback;
  EXPECT_EQ(session.RegisterCallBackFunc(key.c_str(), session_callback), SUCCESS);

  EXPECT_TRUE(session.IsGraphNeedRebuild(graph_id));
  EXPECT_EQ(session.RemoveGraph(graph_id), SUCCESS);
  EXPECT_EQ(GEFinalize(), SUCCESS);
  ReInitGe();
}
}  // namespace ge
