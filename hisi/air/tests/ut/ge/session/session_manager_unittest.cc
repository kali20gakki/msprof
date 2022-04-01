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
#include "session/session_manager.h"
#undef private
#undef protected


using namespace std;

namespace ge {
class Utest_SessionManager : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

Status Callback3(uint32_t, const std::map<std::string, ge::Tensor> &){
  return SUCCESS;
}

Status Callback4(uint32_t, const std::map<AscendString, ge::Tensor> &){
  return SUCCESS;
}

TEST_F(Utest_SessionManager, build_graph_failed) {
  map<string, string> session_manager_option;
  map<string, string> session_option;
  SessionManager *session_manager = new SessionManager();
  uint64_t session_id = 0;
  uint32_t graph_id = 0;
  std::vector<ge::Tensor> inputs;

  Status ret = session_manager->BuildGraph(session_id, graph_id, inputs);
  EXPECT_EQ(ret, ge::GE_SESSION_MANAGER_NOT_INIT);

  session_manager->Initialize(session_manager_option);
  ret = session_manager->BuildGraph(session_id, graph_id, inputs);
  EXPECT_NE(ret, ge::SUCCESS);
  delete session_manager;
}

TEST_F(Utest_SessionManager, RungraphAsync_before_init) {
  SessionManager *session_manager = new SessionManager();
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  std::vector<ge::Tensor> inputs;
  RunAsyncCallback callback;
  Status ret = session_manager->RunGraphAsync(session_id, graph_id, inputs, callback);
  EXPECT_EQ(ret, ge::GE_SESSION_MANAGER_NOT_INIT);
  delete session_manager;
}

TEST_F(Utest_SessionManager, RungraphAsync_failed) {
  map<string, string> session_manager_option;
  SessionManager *session_manager = new SessionManager();
  session_manager->Initialize(session_manager_option);

  SessionId session_id = 0;
  uint32_t graph_id = 0;
  std::vector<ge::Tensor> inputs;
  RunAsyncCallback callback;
  Status ret = session_manager->RunGraphAsync(session_id, graph_id, inputs, callback);
  EXPECT_EQ(ret, ge::GE_SESSION_NOT_EXIST);
  delete session_manager;
}

TEST_F(Utest_SessionManager, Initialize) {
  map<string, string> session_manager_option;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = true;
  EXPECT_EQ(sm->Initialize(session_manager_option), SUCCESS);
}

TEST_F(Utest_SessionManager, Finalize) {
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->Finalize(), SUCCESS);
}

TEST_F(Utest_SessionManager, CreateSession) {
  std::map<std::string, std::string> options;
  SessionId session_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->CreateSession(options, session_id), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, DestroySession) {
  SessionId session_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->DestroySession(session_id), GE_SESSION_MANAGER_NOT_INIT);
  sm->init_flag_ = true;
  EXPECT_EQ(sm->DestroySession(session_id), GE_SESSION_NOT_EXIST);
  std::map<std::string, std::string> options;
  sm->session_manager_map_[0] = std::make_shared<InnerSession>(session_id, options);
  EXPECT_EQ(sm->DestroySession(session_id), SUCCESS);
}

TEST_F(Utest_SessionManager, GetVariable) {
  SessionId session_id = 0;
  std::string name = "name";
  auto val = Tensor();
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->GetVariable(session_id, name, val), GE_SESSION_MANAGER_NOT_INIT);
  sm->init_flag_ = true;
  EXPECT_EQ(sm->GetVariable(session_id, name, val), GE_SESSION_NOT_EXIST);
  std::map<std::string, std::string> options;
  sm->session_manager_map_[0] = std::make_shared<InnerSession>(session_id, options);
  EXPECT_NE(sm->GetVariable(session_id, name, val), SUCCESS);
}

TEST_F(Utest_SessionManager, AddGraph) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  Graph graph("graph");
  auto sm = std::make_shared<SessionManager>();
  EXPECT_NE(sm->AddGraph(session_id, graph_id, graph), SUCCESS);
  std::map<std::string, std::string> options;
  sm->init_flag_ = false;
  EXPECT_EQ(sm->AddGraph(session_id, graph_id, graph, options), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, RunGraph) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  std::vector<Tensor> inputs;
  std::vector<Tensor> outputs;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->RunGraph(session_id, graph_id, inputs, outputs), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, RunGraphWithStreamAsync) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  std::vector<Tensor> inputs;
  std::vector<Tensor> outputs;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->RunGraphWithStreamAsync(session_id, graph_id, nullptr, inputs, outputs), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, RemoveGraph) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->RemoveGraph(session_id, graph_id), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, HasSession) {
  SessionId session_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->HasSession(session_id), false);
  sm->init_flag_ = true;
  EXPECT_EQ(sm->HasSession(session_id), false);
}

TEST_F(Utest_SessionManager, GetNextSessionId) {
  SessionId session_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->GetNextSessionId(session_id), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, RegisterCallBackFunc) {
  SessionId session_id = 0;
  std::string key = "key";
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->RegisterCallBackFunc(session_id, key, Callback3), GE_SESSION_MANAGER_NOT_INIT);
  EXPECT_EQ(sm->RegisterCallBackFunc(session_id, key, Callback4), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, BuildGraph) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  std::vector<InputTensorInfo> inputs;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->BuildGraph(session_id, graph_id, inputs), GE_SESSION_MANAGER_NOT_INIT);
}

TEST_F(Utest_SessionManager, GetVariables) {
  SessionId session_id = 0;
  std::vector<std::string> var_names;
  std::vector<Tensor> var_values;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->GetVariables(session_id, var_names, var_values), GE_SESSION_MANAGER_NOT_INIT);
  sm->init_flag_ = true;
  EXPECT_EQ(sm->GetVariables(session_id, var_names, var_values), GE_SESSION_NOT_EXIST);
}

TEST_F(Utest_SessionManager, IsGraphNeedRebuild) {
  SessionId session_id = 0;
  uint32_t graph_id = 0;
  auto sm = std::make_shared<SessionManager>();
  sm->init_flag_ = false;
  EXPECT_EQ(sm->IsGraphNeedRebuild(session_id, graph_id), true);
}


}  // namespace ge
