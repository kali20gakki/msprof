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
#include "nlohmann/json.hpp"
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/rank_parser.h"
#include "deploy/hcom/communication_domain.h"

namespace ge {
constexpr uint32_t kCommInitPortForCheck = 18001;

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

void to_json(nlohmann::json &j, const CommDomainRank &p) {
  j = nlohmann::json{{"rank_id", p.rank_id}, {"device_id", p.device_id}};
}

void to_json(nlohmann::json &j, const CommDomainNode &p) {
  j = nlohmann::json{{"node_addr", p.node_addr}, {"ranks", p.ranks}};
}

void to_json(nlohmann::json &j, const CommDomainRankTable &p) {
  j = nlohmann::json{{"collective_id", p.collective_id}, {"master_ip", p.master_ip}, {"master_port", p.master_port},
                     {"node_list", p.node_list},         {"status", p.status},       {"version", p.version}};
}

void ConstructComDomainNode(const std::vector<ClusterNodeInfo> &nodes, std::vector<CommDomainNode> &node_list) {
  std::map<std::string, std::vector<CommDomainRank>> ipaddr_to_domain_ranks;

  for (const auto &node : nodes) {
    if (node.ranks.size() == 0) {
      continue;
    }
    GELOGI("[Comm][Manager] com domain node device id = %d", node.device_id);
    const auto &find = ipaddr_to_domain_ranks.find(node.ipaddr);
    uint32_t rank_id = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankId(node.ranks[0]);
    CommDomainRank domain_rank = {
        .rank_id = std::to_string(rank_id),
        .device_id = std::to_string(node.device_id)
    };

    if (find != ipaddr_to_domain_ranks.end()) {
      auto &find_domain_ranks = find->second;
      find_domain_ranks.emplace_back(domain_rank);
    } else {
      std::vector<CommDomainRank> domain_ranks;
      domain_ranks.emplace_back(domain_rank);
      ipaddr_to_domain_ranks.emplace(std::make_pair(node.ipaddr, domain_ranks));
    }
  }

  for (const auto &iter : ipaddr_to_domain_ranks) {
    CommDomainNode n;
    n.node_addr = iter.first;
    for (const auto &rank : iter.second) {
      n.ranks.emplace_back(rank);
    }
    node_list.emplace_back(n);
  }
}

Status CommDomainManager::GenerateRankTable(const std::string &master_ip, const std::string &master_ports,
                                            const std::vector<ClusterNodeInfo> &nodes) {
  CommDomainRankTable rank_table;
  rank_table.collective_id = master_ip + "-worldgroup";
  rank_table.master_ip = master_ip;
  rank_table.master_port = master_ports; // hccl do not use, but check is exist

  ConstructComDomainNode(nodes, rank_table.node_list);
  rank_table.status = "completed";
  rank_table.version = "1.1";

  try {
    nlohmann::json j = rank_table;
    rank_table_ = j.dump();
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Comm][Json]Generate Rank Table Failed,exception:%s.", e.what());
    return FAILED;
  }
  GELOGI("[Comm][Manager] Rank table is %s.", rank_table_.c_str());
  return SUCCESS;
}

Status CommDomainManager::Init(const std::string &local_addr, const std::string &local_ports,
                               const std::vector<DeviceInfo> &devices) {
  ClusterManagerFactory factory;
  GELOGI("[Comm][Manager] Commnuication domain init start.");
  auto worker = factory.Create(local_addr, devices);
  if (worker == nullptr) {
    return FAILED;
  }
  if (worker->Init() != SUCCESS) {
    return FAILED;
  }
  if (GenerateRankTable(local_addr, local_ports, worker->GetNodeList()) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge
