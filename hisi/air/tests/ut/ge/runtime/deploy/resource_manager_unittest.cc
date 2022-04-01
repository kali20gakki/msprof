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
#include "deploy/resource/resource_manager.h"
#undef private
#undef protected

using namespace std;
namespace ge {
class ResourceManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    DeployerProxy::GetInstance().Finalize();
  }
  void TearDown() override {
    DeployerProxy::GetInstance().Finalize();
  }
};

TEST_F(ResourceManagerTest, TestResourceManager) {
  DeployerProxy deployer_proxy;
  {
    auto deployer_1 = MakeUnique<LocalDeployer>();
    deployer_1->node_info_.device_id_ = -1;
    DeployerProxy::GetInstance().deployers_.emplace_back(std::move(deployer_1));
  }
  {
    auto deployer_2 = MakeUnique<LocalDeployer>();
    deployer_2->node_info_.device_id_ = 0;
    DeployerProxy::GetInstance().deployers_.emplace_back(std::move(deployer_2));
  }
  ResourceManager resource_manager;
  ASSERT_EQ(resource_manager.Initialize(), SUCCESS);
  ASSERT_EQ(resource_manager.GetNpuDeviceInfoList().size(), 1);
  ASSERT_EQ(resource_manager.GetDeviceInfoList().size(), 2);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(0, -1) != nullptr);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(0, 0) == nullptr);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, 0) != nullptr);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, -1) == nullptr);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, 1) == nullptr);
}

TEST_F(ResourceManagerTest, TestResourceManager_2PG) {
  DeployerProxy deployer_proxy;
  {
    auto deployer_1 = MakeUnique<LocalDeployer>();
    deployer_1->node_info_.device_id_ = -1;
    deployer_1->node_info_.dgw_ports_ = {6666};
    DeployerProxy::GetInstance().deployers_.emplace_back(std::move(deployer_1));
  }
  {
    auto deployer_2 = MakeUnique<LocalDeployer>();
    deployer_2->node_info_.device_id_ = 0;
    deployer_2->node_info_.device_count_ = 2;
    deployer_2->node_info_.dgw_ports_ = {16666, 16667};
    DeployerProxy::GetInstance().deployers_.emplace_back(std::move(deployer_2));
  }
  ResourceManager resource_manager;
  ASSERT_EQ(resource_manager.Initialize(), SUCCESS);
  ASSERT_EQ(resource_manager.GetNpuDeviceInfoList().size(), 2);
  ASSERT_EQ(resource_manager.GetDeviceInfoList().size(), 3);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(0, -1) != nullptr);
  ASSERT_EQ(resource_manager.GetDeviceInfo(0, -1)->GetDgwPort(), 6666);
  ASSERT_EQ(resource_manager.GetDeviceInfo(0, -1)->GetFlowRankId(), 0);

  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, 0) != nullptr);
  ASSERT_EQ(resource_manager.GetDeviceInfo(1, 0)->GetDgwPort(), 16666);
  ASSERT_EQ(resource_manager.GetDeviceInfo(1, 0)->GetFlowRankId(), 1);

  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, 1) != nullptr);
  ASSERT_EQ(resource_manager.GetDeviceInfo(1, 1)->GetDgwPort(), 16667);
  ASSERT_EQ(resource_manager.GetDeviceInfo(1, 1)->GetFlowRankId(), 2);

  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, -1) == nullptr);
  ASSERT_TRUE(resource_manager.GetDeviceInfo(1, 2) == nullptr);
}

TEST_F(ResourceManagerTest, TestNodeInfo) {
  NodeInfo node_info;
  node_info.SetDgwPorts({1, 2});
  node_info.SetDeviceCount(2);
  ASSERT_EQ(node_info.GetDeviceCount(), 2);
  int32_t port = -1;
  ASSERT_EQ(node_info.GetDgwPort(-1, port), SUCCESS);
  ASSERT_EQ(port, 1);
  ASSERT_EQ(node_info.GetDgwPort(0, port), SUCCESS);
  ASSERT_EQ(port, 1);
  ASSERT_EQ(node_info.GetDgwPort(1, port), SUCCESS);
  ASSERT_EQ(port, 2);
  ASSERT_EQ(node_info.GetDgwPort(2, port), PARAM_INVALID);
}
}  // namespace ge
