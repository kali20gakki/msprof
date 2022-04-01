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

#include "deploy/hcom/cluster/cluster_manager.h"
#include <string>
#include <memory>
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/cluster/cluster_server.h"
#include "deploy/hcom/cluster/cluster_client.h"


namespace ge {
constexpr int32_t kChiefTimeout = 600;
constexpr int32_t kChiefWaitInterval = 1;
constexpr int32_t kMemberTimeout = 300;
constexpr int32_t kMemberWaitInterval = 1;

class ClusterChiefManager : public ClusterManager {
 public:
  ClusterChiefManager(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices) {
    timeout_ = kChiefTimeout;
    wait_interval_ = kChiefWaitInterval;
    ClusterChiefData::GetInstance().Init(member_info, devices);
  }
  ~ClusterChiefManager() override = default;

  Status Init();
  const std::vector<ClusterNodeInfo> &GetNodeList() {
    return ClusterChiefData::GetInstance().GetNodeList();
  }

 protected:
  void SetTimeout(uint32_t value) {
    timeout_ = value;
  }

  void SetWaitInterval(uint32_t interval) {
    wait_interval_ = interval;
  }

 private:
  uint32_t timeout_;
  uint32_t wait_interval_;
};

class ClusterMemberManager : public ClusterManager {
 public:
  ClusterMemberManager(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices) {
    timeout_ = kMemberTimeout;
    wait_interval_ = kMemberWaitInterval;
    ClusterMemberData::GetInstance().Init(member_info, devices);
  }
  ~ClusterMemberManager() override = default;

  Status Init();
  const std::vector<ClusterNodeInfo> &GetNodeList() override {
    return ClusterMemberData::GetInstance().GetNodeList();
  }

 protected:
  void SetTimeout(uint32_t timeout) {
    timeout_ = timeout;
  }

  void SetWaitInterval(uint32_t wait_interval) {
    wait_interval_ = wait_interval;
  }

 private:
  uint32_t timeout_;
  uint32_t wait_interval_;
};
//  非集群场景todo
Status ClusterChiefManager::Init() {
  Status ret;

  ClusterChiefData::GetInstance().RegisterChiefSelf();
  ret = TryUntil(wait_interval_, timeout_, [&]() { return ClusterChiefData::GetInstance().IsFinished(); });
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Wait worker Timeout.");
    GELOGE(ge::FAILED, "[Cluster][Chief]Wait worker Timeout.");
    ClusterServer::GetInstance().StopClusterServer();
    return FAILED;
  }
  GELOGI("[Cluster][Chief] Register Success.");

  if (ClusterServer::GetInstance().StopClusterServer() != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status ClusterMemberManager::Init() {
  ClusterClient client;
  const std::string &chief_addr = ClusterMemberData::GetInstance().GetChiefAddr();
  Status ret;
  client.CreateClient(chief_addr);

  ret = TryUntil(wait_interval_, timeout_, [&]() { return client.RegisterMemberToChief() == SUCCESS; });
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Register To Chief(%s) Timeout.", chief_addr.c_str());
    GELOGE(ge::FAILED, "[Cluster][Member]Register To Chief(%s) Timeout.", chief_addr.c_str());
    return FAILED;
  }
  ret = TryUntil(wait_interval_, timeout_, [&]() { return client.QueryAllNodes() == SUCCESS; });
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Querry Nodes Timeout.");
    GELOGE(ge::FAILED, "[Cluster][Member]Querry Nodes Timeout.");
    return FAILED;
  }

  GELOGI("[Cluster][Member] Querry Success.");
  if (client.RegisterFinished() != SUCCESS) {
    return FAILED;
  }
  GELOGI("[Cluster][Member] Register Success.");

  return SUCCESS;
}

Status ClusterManager::TryUntil(uint32_t wait_interval, uint32_t timeout, std::function<bool()> try_function) {
  uint32_t time = 0;
  while (!try_function() && time < timeout) {
    std::this_thread::sleep_for(std::chrono::seconds(wait_interval));
    time += wait_interval;
  }
  if (time >= timeout) {
    return FAILED;
  } else {
    return SUCCESS;
  }
}

std::unique_ptr<ClusterManager> ClusterManagerFactory::Create(std::string local_addr,
                                                              const std::vector<DeviceInfo> &devices) {
  ClusterMemberInfo member_info;
  ClusterParser parser;
  GELOGI("[Comm][Manager] Local addr is %s, devices num is %zu", local_addr.c_str(), devices.size());

  member_info.local_addr = local_addr;

  if (parser.MemberParser(member_info) != SUCCESS) {
    return nullptr;
  }

  std::unique_ptr<ClusterManager> member;
  if (member_info.member_type == ClusterMemberType::kChief) {
    member = std::unique_ptr<ClusterManager>(new (std::nothrow) ClusterChiefManager(member_info, devices));
  } else if (member_info.member_type == ClusterMemberType::kMember) {
    member = std::unique_ptr<ClusterManager>(new (std::nothrow) ClusterMemberManager(member_info, devices));
  }

  if (member == nullptr) {
    REPORT_INNER_ERROR("E19999", "Alloc cluster manager failed");
    GELOGE(ge::FAILED, "[Check][new]Alloc cluster manager failed");
  }

  return member;
}
}  // namespace ge
