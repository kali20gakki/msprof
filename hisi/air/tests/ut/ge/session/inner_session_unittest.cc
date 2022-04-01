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

#define private public
#define protected public
#include "session/inner_session.h"
#include "framework/omg/omg_inner_types.h"
#include "graph/ge_local_context.h"
using namespace std;

namespace ge {
class UtestInnerSession : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

Status Callback1(uint32_t, const std::map<std::string, ge::Tensor> &){
  return SUCCESS;
}

Status Callback2(uint32_t, const std::map<AscendString, ge::Tensor> &){
  return SUCCESS;
}


TEST_F(UtestInnerSession, build_graph_success) {
  std::map <string, string> options;
  uint64_t session_id = 1;
  InnerSession inner_seesion(session_id, options);
  std::vector<ge::Tensor> inputs;
  ge::Tensor tensor;
  inputs.emplace_back(tensor);
  Status ret = inner_seesion.BuildGraph(1, inputs);
  EXPECT_NE(ret, ge::SUCCESS);
}

TEST_F(UtestInnerSession, initialize) {
  std::map<std::string, std::string> options = {};
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  EXPECT_EQ(inner_session.Initialize(), SUCCESS);
  EXPECT_EQ(inner_session.Finalize(), SUCCESS);
}

TEST_F(UtestInnerSession, check_op_precision_mode) {
  std::map<std::string, std::string> options = {
    {ge::OP_PRECISION_MODE, "./op_precision_mode.ini"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  auto ret = inner_session.Initialize();
  EXPECT_NE(ret, ge::SUCCESS);
}

TEST_F(UtestInnerSession, set_train_flag) {
  std::map<std::string, std::string> options = {
    {ge::OPTION_GRAPH_RUN_MODE, "1"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  EXPECT_EQ(inner_session.Initialize(), SUCCESS);
  inner_session.SetTrainFlagOption();
  EXPECT_EQ(domi::GetContext().train_flag, true);
  EXPECT_EQ(inner_session.Finalize(), SUCCESS);
}

TEST_F(UtestInnerSession, Initialize) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  inner_session.init_flag_ = true;
  EXPECT_EQ(inner_session.Initialize(), SUCCESS);
  inner_session.init_flag_ = false;
  EXPECT_EQ(inner_session.Initialize(), FAILED);
  options["ge.exec.disableReuseMemory"] = "1";
  EXPECT_EQ(inner_session.Initialize(), FAILED);
  options["ge.exec.modify_mixlist"] = "";
  EXPECT_EQ(inner_session.Initialize(), FAILED);
  GEThreadLocalContext &context = GetThreadLocalContext();
  std::map<std::string, std::string> options_map;
  options_map["ge.session_device_id"] = "session_device_id";
  EXPECT_EQ(inner_session.Initialize(), FAILED);
  options_map["ge.session_device_id"] = "9999999999";
  EXPECT_EQ(inner_session.Initialize(), FAILED);
  options_map["ge.session_device_id"] = "1";
  EXPECT_EQ(inner_session.Initialize(), FAILED);
}

TEST_F(UtestInnerSession, GetVariable) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  Tensor ts;
  EXPECT_NE(inner_session.GetVariable("name", ts), SUCCESS);
}

TEST_F(UtestInnerSession, AddGraph) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  Graph g("graph");
  EXPECT_NE(inner_session.AddGraph(1, g), SUCCESS);
  std::map<std::string, std::string> mapoptions;
  EXPECT_EQ(inner_session.AddGraph(1, g, mapoptions), GE_SESS_INIT_FAILED);
}

TEST_F(UtestInnerSession, RunGraph) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  uint32_t graph_id = 0;
  std::vector<Tensor> inputs;
  std::vector<Tensor> outputs;
  EXPECT_EQ(inner_session.RunGraph(graph_id, inputs, outputs), GE_SESS_INIT_FAILED);
}

TEST_F(UtestInnerSession, RunGraphWithStreamAsync) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  uint32_t graph_id = 0;
  std::vector<Tensor> inputs;
  std::vector<Tensor> outputs;
  EXPECT_EQ(inner_session.RunGraphWithStreamAsync(graph_id, nullptr, inputs, outputs), GE_SESS_INIT_FAILED);
}

TEST_F(UtestInnerSession, RemoveGraph) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  uint32_t graph_id = 0;
  EXPECT_EQ(inner_session.RemoveGraph(graph_id), GE_SESS_INIT_FAILED);
}

TEST_F(UtestInnerSession, RegisterCallBackFunc1) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  const std::string key = "key";
  EXPECT_EQ(inner_session.RegisterCallBackFunc(key, Callback1), GE_SESS_INIT_FAILED);
  EXPECT_EQ(inner_session.RegisterCallBackFunc(key, Callback2), GE_SESS_INIT_FAILED);
}

TEST_F(UtestInnerSession, BuildGraph) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  uint32_t graph_id = 0;
  std::vector<InputTensorInfo> inputs;
  inputs.push_back(InputTensorInfo());
  EXPECT_NE(inner_session.BuildGraph(graph_id, inputs), SUCCESS);
}

TEST_F(UtestInnerSession, GenCheckPointGraph) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  std::map<std::string, GeTensorDesc> all_variables;
  Graph g("graph");
  all_variables["name"] = GeTensorDesc();
  EXPECT_EQ(inner_session.GenCheckPointGraph(all_variables, g), SUCCESS);
}

TEST_F(UtestInnerSession, Finalize) {
  std::map<std::string, std::string> options = {
    {"ge.exec.disableReuseMemory", "100"}
  };
  uint64_t session_id = 1;
  InnerSession inner_session(session_id, options);
  inner_session.init_flag_ = true;
  EXPECT_EQ(inner_session.Finalize(), SUCCESS);
}

}  // namespace ge
