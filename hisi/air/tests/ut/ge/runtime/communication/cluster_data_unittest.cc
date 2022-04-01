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
#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "deploy/hcom/cluster/cluster_data.h"

namespace ge {
class UtClusterData : public testing::Test {
 public:
  UtClusterData() {}

 protected:
  void SetUp() override {}
  void TearDown() override {}
};

struct ClusterDataTestConfig {
  const std::string server_address;
  const uint32_t client_num;
  const std::string meta_client_address;
  const std::string meta_device_address;
  const uint32_t device_num;
};

static ClusterDataTestConfig kClusterDataTestCase = {
  server_address : "0.0.0.0:50051",
  client_num : 10,
  meta_client_address : "0.0.0.1",
  meta_device_address : "0.0.0.2",
  device_num : 10,
};

static void ConstructMemberInfo(ClusterMemberType type, uint32_t member_id, ClusterMemberInfo &member_info) {
  member_info.chief_addr = kClusterDataTestCase.server_address;
  member_info.cluster_member_num = kClusterDataTestCase.client_num + 1;
  if (type == ClusterMemberType::kChief) {
    member_info.local_addr = kClusterDataTestCase.server_address;
  } else {
    member_info.local_addr = kClusterDataTestCase.meta_client_address;
    member_info.local_addr += ":";
    member_info.local_addr += std::to_string(member_id);
  }
  member_info.member_type = type;
  for (uint32_t i = 0; i < member_info.cluster_member_num; i++) {
    std::string m = kClusterDataTestCase.meta_client_address;
    m += ":" + std::to_string(i);
    member_info.members.emplace(make_pair(m, ClusterMemberStatus::kInit));
  }
}

static void ConstructDevices(std::vector<DeviceInfo> *devices) {
  for (uint32_t i = 0; i < kClusterDataTestCase.device_num; i++) {
    DeviceInfo d(1, i);
    d.SetHostIp(kClusterDataTestCase.meta_device_address);
    d.SetDgwPort(i);
    devices->emplace_back(d);
  }
}

static void ConstructChief(ClusterChiefData *chief) {
  ClusterMemberInfo m;
  std::vector<DeviceInfo> devices;

  ConstructMemberInfo(ClusterMemberType::kChief, 0, m);
  ConstructDevices(&devices);

  chief->Init(m, devices);

  const std::vector<ClusterNodeInfo> &result = chief->GetNodeList();

  ASSERT_EQ(result.size(), kClusterDataTestCase.device_num + 1);
  ASSERT_EQ(result[0].node_type, ClusterNodeType::kHost);
  ASSERT_STREQ(result[0].ipaddr.c_str(), kClusterDataTestCase.server_address.c_str());

  for (uint32_t i = 1; i < result.size(); i++) {
    ASSERT_EQ(result[i].node_type, ClusterNodeType::kDevice);
    std::string addr = kClusterDataTestCase.meta_device_address;
    ASSERT_STREQ(result[i].ipaddr.c_str(), addr.c_str());
  }
}

static void ConstructMember(ClusterMemberData *member, uint32_t member_id) {
  ClusterMemberInfo m;
  std::vector<DeviceInfo> devices;

  ConstructMemberInfo(ClusterMemberType::kMember, member_id, m);
  ConstructDevices(&devices);

  member->Init(m, devices);

  const std::vector<ClusterNodeInfo> &result = member->GetNodeList();

  ASSERT_EQ(result.size(), kClusterDataTestCase.device_num + 1);
  ASSERT_EQ(result[0].node_type, ClusterNodeType::kHost);
  std::string local_addr = kClusterDataTestCase.meta_client_address;
  local_addr += ":";
  local_addr += std::to_string(member_id);

  ASSERT_STREQ(result[0].ipaddr.c_str(), local_addr.c_str());

  for (uint32_t i = 1; i < result.size(); i++) {
    ASSERT_EQ(result[i].node_type, ClusterNodeType::kDevice);
    std::string addr = kClusterDataTestCase.meta_device_address;
    ASSERT_STREQ(result[i].ipaddr.c_str(), addr.c_str());
  }
}

TEST_F(UtClusterData, run_cluster_data_construct) {
  ClusterChiefData chief;
  ClusterMemberData member;
  ConstructChief(&chief);
  ConstructMember(&member, 0);
}
}  // namespace ge
