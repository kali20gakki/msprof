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
#include <chrono>

#include <gtest/gtest.h>

#include "deploy/hcom/cluster/cluster_server.h"
#include "deploy/hcom/cluster/cluster_client.h"
#define private public
#define protected public
#include "deploy/hcom/rank_parser.h"
#include "deploy/hcom/communication_domain.h"
#include "deploy/hcom/cluster/cluster_manager.h"
#undef protected
#undef private

namespace ge {

class STESTClusterServer : public testing::Test {
 public:
  STESTClusterServer() {}

 protected:
  void SetUp() override {}
  void TearDown() override {}
};

enum ClusterSeverTestCase {
  kServerCreate = 0,
  kClientCreate,
  kClientConstruct,
  kClientMsg,
  kCommDomain,
  kSeverTestCaseMax = kCommDomain + 1,
};

struct ClusterSeverTestConfig {
  std::string server_address;
  const int client_num;
  std::string meta_client_address;
  std::string meta_device_address;
  const int device_num;
  const int node_device_num;
};

static ClusterSeverTestConfig kClusterServeTestCase[kSeverTestCaseMax] = {
  [kServerCreate] = {
    server_address : "0.0.0.0:50051",
    client_num : 0,
  },
  [kClientCreate] = {
    server_address : "0.0.0.0:50051",
    client_num : 10,
  },
  [kClientConstruct] = {
    server_address : "0.0.0.0:50051",
    client_num : 10,
    meta_client_address : "0.0.0.1",
    meta_device_address : "0.0.0.2",
    device_num : 10,
  },
  [kClientMsg] = {
    server_address : "0.0.0.0:50051",
    client_num : 10,
    meta_client_address : "0.0.0.1",
    meta_device_address : "0.0.0.2",
    device_num : 10,
  },
  [kCommDomain] = {
    server_address : "0.0.0.0:50051",
    client_num : 1,
    meta_client_address : "0.0.0.1",
    meta_device_address : "0.0.0.2",
    device_num : 1,
    node_device_num : 2,
  }
};

static const std::string kHcclJsonTestInfo = {
    "{"
    "	\"rankSize\": 4,"
    "	\"rankTable\": [{"
    "			\"rank_id\": 0,"
    "			\"model_instance_id\": 0"
    "		},"
    "		{"
    "			\"rank_id\": 1,"
    "			\"model_instance_id\": 1"
    "		},"
    "		{"
    "			\"rank_id\": 2,"
    "			\"model_instance_id\": 2"
    "		},"
    "		{"
    "			\"rank_id\": 3,"
    "			\"model_instance_id\": 3"
    "		},"
    "		{"
    "			\"rank_id\": 4,"
    "			\"model_instance_id\": 4"
    "		}"
    "	],"
    "	\"subGroups\": [{"
    "			\"group_id\": \"group1\","
    "			\"group_rank_list\": [0,2]"
    "		},"
    "		{"
    "			\"group_id\": \"group2\","
    "			\"group_rank_list\": [1,3]"
    "		}"
    "	]"
    "}",
};

static const std::string kDepolyJsonTestInfo = {
    "[{"
    "		\"model_instance_id\": 0,"
    "		\"device_id\": 0,"
    "		\"model_id\": \"model1\""
    "	},"
    " {"
    "		\"model_instance_id\": 1,"
    "		\"device_id\": 0,"
    "		\"model_id\": \"model1\""
    "	},"
    "	{"
    "		\"model_instance_id\": 2,"
    "		\"device_id\": 0,"
    "		\"model_id\": \"model1\""
    "	},"
    "	{"
    "		\"model_instance_id\": 3,"
    "		\"device_id\": 1,"
    "		\"model_id\": \"model2\""
    "	},"
    "	{"
    "		\"model_instance_id\": 4,"
    "		\"device_id\": 2,"
    "		\"model_id\": \"model3\""
    "	}"
    "]",
};

static const std::string kClusterJsonTestEnv =
    "{\"chief\":\"0.0.0.0:50051\",\"worker\":[\"0.0.0.0:1\"]}";

TEST_F(STESTClusterServer, run_cluster_server_create) {
  // setup
  ClusterServer server;
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv.c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "0.0.0.0:50051";

  auto ret = parser.MemberParser(member_info);
  ASSERT_EQ(ret, SUCCESS);

  ClusterServer::GetInstance().StopClusterServer();
  ret = server.StartClusterServer(member_info.chief_addr);
  ASSERT_EQ(ret, SUCCESS);

  ASSERT_TRUE(server.GetThread().joinable() == true);

  server.StopClusterServer();
  ASSERT_TRUE(server.GetThread().joinable() == false);
  unsetenv("HELP_CLUSTER");
}

#define _ kClusterServeTestCase

static void ConstructMemberInfo(ClusterMemberType type, int member_id, ClusterMemberInfo &member_info) {
  member_info.chief_addr = _[kClientConstruct].server_address;
  member_info.cluster_member_num = _[kClientConstruct].client_num + 1;
  if (type == ClusterMemberType::kChief) {
    member_info.local_addr = _[kClientConstruct].server_address;
  } else {
    member_info.local_addr = _[kClientConstruct].meta_client_address;
    member_info.local_addr += ":";
    member_info.local_addr += std::to_string(member_id);
  }
  member_info.member_type = type;
  for (uint32_t i = 0; i < member_info.cluster_member_num; i++) {
    std::string m = _[kClientConstruct].meta_client_address;
    m += ":" + std::to_string(i);
    member_info.members.insert(make_pair(m, ClusterMemberStatus::kInit));
  }
}

static void ConstructDevices(std::vector<DeviceInfo> &devices, ClusterSeverTestCase type, int32_t client_id) {
  std::string ipaddr = _[type].meta_device_address;
  ipaddr += ":";
  ipaddr += std::to_string(client_id);
  for (int i = 0; i < _[type].device_num; i++) {
    DeviceInfo d(1, i);
    d.SetHostIp(ipaddr + ":" + std::to_string(i/_[type].node_device_num));
    devices.emplace_back(d);
  }
}

static void ConstructDevices(std::vector<DeviceInfo> &devices, ClusterSeverTestCase type) {
  for (int i = 1; i < _[type].device_num + 1; i++) {
    DeviceInfo d(1, i);
    d.SetHostIp(_[type].meta_device_address);
    devices.emplace_back(d);
  }
}

static void ConstructChief(ClusterChiefData &chief) {
  ClusterMemberInfo m;
  std::vector<DeviceInfo> devices;

  ConstructMemberInfo(ClusterMemberType::kChief, 0, m);
  ConstructDevices(devices, kClientConstruct);

  chief.Init(m, devices);

  const std::vector<ClusterNodeInfo> &result = chief.GetNodeList();

  ASSERT_EQ(result.size(), _[kClientConstruct].device_num + 1);
  ASSERT_EQ(result[0].node_type, ClusterNodeType::kHost);
  ASSERT_STREQ(result[0].ipaddr.c_str(), _[kClientConstruct].server_address.c_str());

  for (uint32_t i = 1; i < result.size(); i++) {
    ASSERT_EQ(result[i].node_type, ClusterNodeType::kDevice);
    std::string addr = _[kClientConstruct].meta_device_address;
    ASSERT_STREQ(result[i].ipaddr.c_str(), addr.c_str());
  }
}

static void ConstructMember(int member_id, ClusterMemberData &member) {
  ClusterMemberInfo m;
  std::vector<DeviceInfo> devices;

  ConstructMemberInfo(ClusterMemberType::kMember, member_id, m);
  ConstructDevices(devices, kClientConstruct);

  member.Init(m, devices);

  const std::vector<ClusterNodeInfo> &result = member.GetNodeList();

  ASSERT_EQ(result.size(), _[kClientConstruct].device_num + 1);
  ASSERT_EQ(result[0].node_type, ClusterNodeType::kHost);
  std::string local_addr = _[kClientConstruct].meta_client_address;
  local_addr += ":";
  local_addr += std::to_string(member_id);

  ASSERT_STREQ(result[0].ipaddr.c_str(), local_addr.c_str());

  for (uint32_t i = 1; i < result.size(); i++) {
    ASSERT_EQ(result[i].node_type, ClusterNodeType::kDevice);
    std::string addr = _[kClientConstruct].meta_device_address;
    ASSERT_STREQ(result[i].ipaddr.c_str(), addr.c_str());
  }
}

TEST_F(STESTClusterServer, run_cluster_construct) {
  ClusterChiefData chief;
  ClusterMemberData member;
  ConstructChief(chief);
  ConstructMember(0, member);
}

static void ClusterStartClient() {
  ClusterClient client;
  auto ret = client.CreateClient(kClusterServeTestCase[kClientCreate].server_address);
  ASSERT_TRUE(ret == SUCCESS);
}

TEST_F(STESTClusterServer, run_cluster_client_create) {
  // setup
  ClusterServer server;

  const std::string server_address(kClusterServeTestCase[kClientCreate].server_address);

  auto ret = server.StartClusterServer(server_address);
  ASSERT_EQ(ret, SUCCESS);

  std::unique_ptr<std::thread[]> threads(new std::thread[kClusterServeTestCase[kClientCreate].client_num]);

  for (int i = 0; i < kClusterServeTestCase[kClientCreate].client_num; i++) {
    threads[i] = std::move(std::thread(ClusterStartClient));
  }

  for (int i = 0; i < kClusterServeTestCase[kClientCreate].client_num; i++) {
    threads[i].join();
  }
  server.StopClusterServer();
  ASSERT_TRUE(server.GetThread().joinable() == false);
}

static void ClusterStartClientAndSendMsg(int client_id) {
  // setup
  ClusterClient client;
  ClusterMemberData &member = ClusterMemberData::GetInstance();

  auto ret = client.CreateClient(_[kClientMsg].server_address);
  ASSERT_TRUE(ret == SUCCESS);
  ConstructMember(client_id, member);

  // test
  ASSERT_TRUE(client.RegisterMemberToChief() == SUCCESS);
  int cnt = 20;
  while (cnt--) {
    if (client.QueryAllNodes() == SUCCESS) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  ASSERT_EQ(member.GetNodeList().size(), (_[kClientMsg].client_num + 1) * (_[kClientMsg].device_num + 1));
  ASSERT_TRUE(client.RegisterFinished() == SUCCESS);
}

TEST_F(STESTClusterServer, run_cluster_client_msg) {
  // setup
  ClusterServer server;
  ClusterChiefData &chief = ClusterChiefData::GetInstance();

  setenv("RANK_ID", "1", 1);
  const std::string server_address(kClusterServeTestCase[kClientMsg].server_address);
  auto ret = server.StartClusterServer(server_address);
  ASSERT_EQ(ret, SUCCESS);

  auto &rank_parser = RankParser::GetInstance();
  ret = rank_parser.Init(kHcclJsonTestInfo, kDepolyJsonTestInfo);
  ASSERT_TRUE(ret == SUCCESS);

  ConstructChief(chief);

  // test
  std::unique_ptr<std::thread[]> threads(new std::thread[kClusterServeTestCase[kClientMsg].client_num]);
  for (int i = 0; i < kClusterServeTestCase[kClientMsg].client_num; i++) {
    threads[i] = std::move(std::thread(ClusterStartClientAndSendMsg, i));
  }

  for (int i = 0; i < kClusterServeTestCase[kClientMsg].client_num; i++) {
    threads[i].join();
  }

  server.StopClusterServer();
  unsetenv("RANK_ID");
  ASSERT_TRUE(chief.IsFinished());
  ASSERT_TRUE(server.GetThread().joinable() == false);
}

#undef _

TEST_F(STESTClusterServer, run_cluster_create_chief_manger) {
  ClusterManagerFactory factory;
  setenv("HELP_CLUSTER", kClusterJsonTestEnv.c_str(), 1);
  const std::string &chief_local_addr = "0.0.0.0:50051";
  std::vector<DeviceInfo> devices;
  ConstructDevices(devices, kClientConstruct);
  setenv("RANK_ID", "1", 1);
  auto ret = factory.Create(chief_local_addr, devices);
  ASSERT_NE(ret, nullptr);
  ClusterServer::GetInstance().StopClusterServer();

  const std::string &member_local_addr = "0.0.0.1:0";
  ret = factory.Create(member_local_addr, devices);
  ASSERT_NE(ret, nullptr);
  ClusterServer::GetInstance().StopClusterServer();
  unsetenv("RANK_ID");
  unsetenv("HELP_CLUSTER");
}

TEST_F(STESTClusterServer, run_domain_Fail) {
  CommDomainManager domain;
  const std::string &chief_local_addr = "0.0.0.0";
  std::vector<DeviceInfo> devices;
  ConstructDevices(devices, kClientConstruct);

  auto ret = domain.Init(chief_local_addr, "50051", devices);
  ASSERT_NE(ret,SUCCESS);
}

TEST_F(STESTClusterServer, run_parser_fail) {
  RankParser parser;
  std::string hccl_cluster_info = {};
  std::string model_deploy_info = {};
  Status ret = parser.ParseClusterInfo(hccl_cluster_info, model_deploy_info);
  ASSERT_NE(ret,SUCCESS);
  ret = parser.ParseClusterInfo(kHcclJsonTestInfo, model_deploy_info);
  ASSERT_NE(ret,SUCCESS);
}

static void CreateClient(int client_id) {
  CommDomainManager domain;
  std::vector<DeviceInfo> devices;
  ConstructDevices(devices, kCommDomain, client_id);
  std::string client_base_addr = kClusterServeTestCase[kCommDomain].meta_client_address;
  std::string client_local_addr = client_base_addr + ":" + std::to_string(client_id - 1);

  ClusterManagerFactory factory;
  auto worker = factory.Create(client_local_addr, devices);
  ASSERT_NE(worker, nullptr);

  worker->SetTimeout(5);
  worker->SetWaitInterval(1);

  Status ret = worker->Init();
  //ASSERT_EQ(ret, SUCCESS);

  ret = domain.GenerateRankTable(client_base_addr, "0", worker->GetNodeList());
  ASSERT_EQ(ret, SUCCESS);
}

static void CreateServer() {
  CommDomainManager domain;
  const std::string &chief_local_addr = kClusterServeTestCase[kCommDomain].server_address;
  std::vector<DeviceInfo> devices;
  ConstructDevices(devices, kCommDomain, 0);

  ClusterManagerFactory factory;
  auto worker = factory.Create(chief_local_addr, devices);
  ASSERT_NE(worker, nullptr);

  worker->SetTimeout(5);
  worker->SetWaitInterval(1);

  Status ret = worker->Init();
  ASSERT_EQ(ret, SUCCESS);

  ret = domain.GenerateRankTable("0.0.0.0", "0", worker->GetNodeList());
  ASSERT_EQ(ret, SUCCESS);
}

TEST_F(STESTClusterServer, run_comm_domain) {
  dlog_setlevel(0,0,0);
  setenv("HELP_CLUSTER", kClusterJsonTestEnv.c_str(), 1);
  auto &rank_parser = RankParser::GetInstance();
  auto ret = rank_parser.Init(kHcclJsonTestInfo, kDepolyJsonTestInfo);
  ASSERT_TRUE(ret == SUCCESS);

  auto pid = fork();
  if (pid == 0) {
  setenv("RANK_ID", "0", 1);
  std::thread server_threads(CreateServer);
  server_threads.join();
  unsetenv("RANK_ID");
  }

  setenv("RANK_ID", "1", 1);
  std::unique_ptr<std::thread[]> client_threads(new std::thread[kClusterServeTestCase[kCommDomain].client_num]);
  for (int i = 0; i < kClusterServeTestCase[kCommDomain].client_num; i++) {
    client_threads[i] = std::move(std::thread(CreateClient, i + 1));
  }

  for (int i = 0; i < kClusterServeTestCase[kCommDomain].client_num; i++) {
    client_threads[i].join();
  }
  //server_threads.join();
  unsetenv("RANK_ID");
  unsetenv("HELP_CLUSTER");
  dlog_setlevel(0,3,0);
}
}  // namespace ge
