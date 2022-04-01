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
#include <vector>
#include <string>
#define protected public
#define private public
#include "daemon/daemon_service.h"
#include "common/config/configurations.h"

using namespace std;

namespace ge {
class DaemonServiceUnittest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DaemonServiceUnittest, TestInitializeAndFinalize) {
  ge::DaemonService daemon_service;
  EXPECT_EQ(daemon_service.Initialize(), SUCCESS);
  daemon_service.Finalize();
}

TEST_F(DaemonServiceUnittest, TestProcessInitRequest) {
  ge::DaemonService daemon_service;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  deployer::InitRequest init_request_;
  std::string token = "xxxxxxx";
  init_request_.set_token(token);
  *request.mutable_init_request() = init_request_;
  daemon_service.ProcessInitRequest("xxx:127.0.0.1:8080", request, response);
  ASSERT_EQ(response.error_code(), SUCCESS);
}

TEST_F(DaemonServiceUnittest, TestProcessInitRequest_wrong_token) {
  DaemonService daemon_service;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  deployer::InitRequest init_request_;
  std::string token = "xxxxxxxx";
  init_request_.set_token(token);
  *request.mutable_init_request() = init_request_;
  daemon_service.ProcessInitRequest("xxx:127.0.0.1:8080", request, response);
  ASSERT_NE(response.error_code(), SUCCESS);
}

TEST_F(DaemonServiceUnittest, TestHeartbeatRequest) {
  DaemonService daemon_service;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  deployer::InitRequest init_request_;
  std::string token = "xxxxxxx";
  init_request_.set_token(token);
  request.set_client_id(0);
  *request.mutable_init_request() = init_request_;
  daemon_service.ProcessInitRequest("xxx:127.0.0.1:8080", request, response);
  request.set_type(deployer::kHeartbeat);
  daemon_service.Process("xxx:127.0.0.1:8080", request, response);
  ASSERT_EQ(response.error_code(), SUCCESS);
}

TEST_F(DaemonServiceUnittest, TestDisconnectRequest) {
  DaemonService daemon_service;
  deployer::DeployerRequest request;
  request.set_type(deployer::kDisconnect);
  deployer::DeployerResponse response;
  daemon_service.Process("xxx:127.0.0.1:8080", request, response);
  daemon_service.Finalize();
}

TEST_F(DaemonServiceUnittest, TestInitAndDisconnectRequest) {
  DaemonService daemon_service;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  deployer::InitRequest init_request_;
  std::string token = "xxxxxxx";
  init_request_.set_token(token);
  *request.mutable_init_request() = init_request_;
  daemon_service.ProcessInitRequest("xxx:127.0.0.1:8080", request, response);
  request.set_client_id(0);
  daemon_service.ProcessDisconnectRequest("xxx:127.0.0.1:8080", request, response);
}

TEST_F(DaemonServiceUnittest, TestDeployRequest) {
  DaemonService daemon_service;
  daemon_service.client_manager_.clients_[1] = std::make_unique<DeployerDaemonClient>(1);
  deployer::DeployerRequest request;
  request.set_client_id(1);
  request.set_type(deployer::kLoadModel);
  deployer::DeployerResponse response;
  daemon_service.Process("xxx:127.0.0.1:8080", request, response);
  ASSERT_NE(response.error_code(), SUCCESS);
  daemon_service.Finalize();
}

} // namespace ge





