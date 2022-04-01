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

#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#define protected public
#include "deploy/hcom/cluster/cluster_data.h"
#include "deploy/hcom/cluster/cluster_manager.h"
#undef protected
#undef private

#include "deploy/hcom/cluster/cluster_client.h"

namespace ge {
class ClusterClientTest : public testing::Test {
 protected:
  void SetUp() override {
    setenv("RANK_ID", "0", 1);
  }
  void TearDown() override {
    unsetenv("RANK_ID");
  }
};

namespace {
const std::string kClusterJsonTestEnv =
    "{\"chief\":\"0.0.0.0:50051\"}";

}  // namespace

TEST_F(ClusterClientTest, TestRegisterMemberFailed) {
  auto &chief_data = ClusterChiefData::GetInstance();
  chief_data.member_info_.cluster_member_num = 1;

  ClusterClient client;
  EXPECT_EQ(client.CreateClient("127.0.0.1:6666"), SUCCESS);
  EXPECT_EQ(client.RegisterMemberToChief(), FAILED);
}

TEST_F(ClusterClientTest, TestQueryAllNodesFailed) {
  auto &chief_data = ClusterChiefData::GetInstance();
  chief_data.member_info_.cluster_member_num = 1;

  ClusterClient client;
  EXPECT_EQ(client.CreateClient("127.0.0.1:6666"), SUCCESS);
  EXPECT_EQ(client.QueryAllNodes(), FAILED);
}

TEST_F(ClusterClientTest, TestWaitWorkerTimeout) {
  // construct chief
  std::string help_clu = R"({"chief":"0.0.0.0:50051","worker":["0.0.0.0:1"]})";
  setenv("HELP_CLUSTER", help_clu.c_str(), 1);
  std::vector<DeviceInfo> devices;
  DeviceInfo device_info(1, 0);
  device_info.SetHostIp("0.0.0.1");
  devices.emplace_back(device_info);
  ClusterManagerFactory factory;
  auto chief_manager = factory.Create("0.0.0.0", devices);
  ASSERT_NE(chief_manager, nullptr);
  chief_manager->SetTimeout(1);
  chief_manager->SetWaitInterval(1);
  // Wait worker Timeout
  ASSERT_EQ(chief_manager->Init(), FAILED);
}

TEST_F(ClusterClientTest, TestInitSuccess) {
  // construct chief
  setenv("HELP_CLUSTER", kClusterJsonTestEnv.c_str(), 1);
  std::vector<DeviceInfo> devices;
  DeviceInfo device_info(1, 0);
  device_info.SetHostIp("0.0.0.1");
  devices.emplace_back(device_info);
  ClusterManagerFactory factory;
  auto chief_manager = factory.Create("0.0.0.0", devices);
  ASSERT_NE(chief_manager, nullptr);
  chief_manager->SetTimeout(10);
  chief_manager->SetWaitInterval(1);

  ASSERT_EQ(chief_manager->Init(), SUCCESS);
}
}  // namespace ge
