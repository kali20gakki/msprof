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
#define private public
#define protected public
#include "deploy/hcom/communication_domain.h"
#include "deploy/hcom/cluster/cluster_data.h"
#undef protected
#undef private

namespace ge {
class UtCommDomain : public testing::Test {
 public:
  UtCommDomain() {}

 protected:
  void SetUp() override {}
  void TearDown() override {}
};

struct CommDomainTestCfg {
  std::string master_ip;
  std::string master_port;
  std::string meta_ipaddr;
  int32_t node_cnt;
  int32_t meta_rank_id;
  int32_t rank_num;
  int32_t device_num;
};

CommDomainTestCfg kCommTestRankTable = {
    .master_ip = "1.1.1.1",
    .master_port = "18001",
    .meta_ipaddr = "1.1.1.2",
    .node_cnt = 2,
    .meta_rank_id = 0,
    .rank_num = 2,
    .device_num = 2,
};

struct CommDomainRank {
  std::string rank_id;
  std::string device_id;
};

struct CommDomainNode {
  std::string node_addr;
  std::vector<CommDomainRank> ranks;
};

struct CommDomainRankTable {
  std::string collective_id;
  std::string master_ip;
  std::string master_port;
  std::vector<CommDomainNode> node_list;
  std::string status;
  std::string version;
};

void from_json(const nlohmann::json &j, CommDomainRank &p) {
  j.at("rank_id").get_to(p.rank_id);
  j.at("device_id").get_to(p.device_id);
}

void from_json(const nlohmann::json &j, CommDomainNode &p) {
  j.at("node_addr").get_to(p.node_addr);
  j.at("ranks").get_to(p.ranks);
}

void from_json(const nlohmann::json &j, CommDomainRankTable &p) {
  j.at("collective_id").get_to(p.collective_id);
  j.at("master_ip").get_to(p.master_ip);
  j.at("master_port").get_to(p.master_port);
  j.at("node_list").get_to(p.node_list);
  j.at("status").get_to(p.status);
  j.at("version").get_to(p.version);
}

TEST_F(UtCommDomain, run_cluster_parser_right_format) {
  CommDomainManager domain;
  std::vector<ClusterNodeInfo> nodes;
  for (int32_t i = 0; i < kCommTestRankTable.node_cnt; i++) {
    ClusterNodeInfo n;
    n.ipaddr = "1.1.1.2";
    for (int32_t j = 0; j < kCommTestRankTable.rank_num; j++) {
      n.ranks.emplace_back(i * kCommTestRankTable.rank_num + j);
    }
    nodes.emplace_back(n);
  }

  domain.GenerateRankTable(kCommTestRankTable.master_ip, kCommTestRankTable.master_port, nodes);

  std::string rank_table = domain.GetRankTable();
  auto j = nlohmann::json::parse(rank_table.c_str());
  auto ret_rank_table = j.get<CommDomainRankTable>();
  ASSERT_STREQ(ret_rank_table.master_ip.c_str(), kCommTestRankTable.master_ip.c_str());
  ASSERT_STREQ(ret_rank_table.master_port.c_str(), kCommTestRankTable.master_port.c_str());
  ASSERT_EQ(ret_rank_table.node_list.size(), 1);
  domain.DestroyRankTable();
}

TEST_F(UtCommDomain, run_domain_creater_worker_fail) {
  CommDomainManager domain;
  const std::string &chief_local_addr = "0.0.0.0:50051";
  std::vector<DeviceInfo> devices;

  auto ret = domain.Init(chief_local_addr, "0", devices);
  ASSERT_NE(ret,SUCCESS);
}

}  // namespace ge
