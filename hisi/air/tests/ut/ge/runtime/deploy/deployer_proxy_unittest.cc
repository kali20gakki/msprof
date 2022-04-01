/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#include "framework/common/debug/ge_log.h"

#define protected public
#define private public
#include "deploy/deployer/deployer_proxy.h"
#undef private
#undef protected

using namespace std;
namespace ge {
class DeployerProxyTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

namespace {
class MockDeployer : public Deployer {
 public:
  MockDeployer() = default;
  const NodeInfo &GetNodeInfo() const override {
    return node_info_;
  }
  Status Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) override {
    return SUCCESS;
  }
  Status Initialize() override {
    return SUCCESS;
  }
  Status Finalize() override {
    return SUCCESS;
  }
 private:
  NodeInfo node_info_;
};
}

TEST_F(DeployerProxyTest, TestInitializeAndAutoFinalize) {
  NodeConfig local_node_config;
  local_node_config.device_id = -1;
  std::vector<NodeConfig> node_config_list{local_node_config};
  DeployerProxy deployer_proxy;
  ASSERT_EQ(deployer_proxy.Initialize(node_config_list), SUCCESS);
  ASSERT_EQ(deployer_proxy.NumNodes(), node_config_list.size());
}

TEST_F(DeployerProxyTest, TestCreateDeployer) {
  NodeConfig local_node_config;
  local_node_config.device_id = -1;
  ASSERT_TRUE(DeployerProxy::CreateDeployer(local_node_config) != nullptr);
  NodeConfig remote_node_config;
  local_node_config.device_id = 0;
  ASSERT_TRUE(DeployerProxy::CreateDeployer(remote_node_config) != nullptr);
}

TEST_F(DeployerProxyTest, TestInitializeAndFinalize_empty) {
  std::vector<NodeConfig> node_config_list{};
  DeployerProxy deployer_proxy;
  auto ret = deployer_proxy.Initialize(node_config_list);
  ASSERT_EQ(ret, ge::SUCCESS);
  deployer_proxy.Finalize();
}

TEST_F(DeployerProxyTest, TestSendRequest) {
  NodeConfig local_node_config;
  local_node_config.device_id = -1;
  NodeConfig remote_node_config;
  remote_node_config.device_id = 0;
  std::vector<NodeConfig> node_config_list{local_node_config, remote_node_config};
  DeployerProxy deployer_proxy;
  deployer_proxy.deployers_.emplace_back(MakeUnique<MockDeployer>());
  deployer_proxy.deployers_.emplace_back(MakeUnique<MockDeployer>());
  ASSERT_TRUE(deployer_proxy.GetNodeInfo(0) != nullptr);
  ASSERT_TRUE(deployer_proxy.GetNodeInfo(1) != nullptr);
  ASSERT_TRUE(deployer_proxy.GetNodeInfo(2) == nullptr);
  ASSERT_TRUE(deployer_proxy.GetNodeInfo(-1) == nullptr);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  ASSERT_EQ(deployer_proxy.SendRequest(0, request, response), SUCCESS);
  ASSERT_EQ(deployer_proxy.SendRequest(1, request, response), SUCCESS);
  ASSERT_EQ(deployer_proxy.SendRequest(2, request, response), PARAM_INVALID);
  ASSERT_EQ(deployer_proxy.SendRequest(-1, request, response), PARAM_INVALID);
}

TEST_F(DeployerProxyTest, TestLocalDeployer) {
  LocalDeployer deployer;
  ASSERT_EQ(deployer.Initialize(), SUCCESS);
  deployer::DeployerRequest request;
  request.set_type(deployer::kHeartbeat);
  deployer::DeployerResponse response;
  ASSERT_EQ(deployer.Process(request, response), SUCCESS);
  deployer.Finalize();
}

TEST_F(DeployerProxyTest, TestRemoteDeployer) {
  Configurations::GetInstance().host_information_ = DeployerConfig{};
  NodeConfig remote_node_config;
  remote_node_config.device_id = 0;
  RemoteDeployer deployer(remote_node_config);
  ASSERT_EQ(deployer.Initialize(), SUCCESS);
  deployer.SendHeartbeat();
  deployer.Finalize();
}
}  // namespace ge
