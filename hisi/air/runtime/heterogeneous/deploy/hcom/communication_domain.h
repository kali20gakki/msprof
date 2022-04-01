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
#ifndef RUNTIME_COMMUNICATION_COMMUNICATION_DOMAIN_H_
#define RUNTIME_COMMUNICATION_COMMUNICATION_DOMAIN_H_
#include <string>
#include <map>
#include <vector>

#include "ge/ge_api_error_codes.h"

#include "deploy/hcom/cluster/cluster_data.h"
#include "deploy/hcom/cluster/cluster_manager.h"
namespace ge {
class CommDomainManager {
 public:
  static CommDomainManager &GetInstance() {
    static CommDomainManager instance;
    return instance;
  }
  void DestroyRankTable() {
    rank_table_.clear();
    return;
  }
  Status Init(const std::string &local_addr, const std::string &local_ports, const std::vector<DeviceInfo> &devices);
  const std::string &GetRankTable() {
    return rank_table_;
  }

 private:
  Status GenerateRankTable(const std::string &master_ip, const std::string &master_ports,
		           const std::vector<ClusterNodeInfo> &nodes);
  std::string rank_table_;
};
}  // namespace ge
#endif  // RUNTIME_COMMUNICATION_COMMUNICATION_DOMAIN_H_
