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

#include "deploy/hcom/cluster/cluster_data.h"
#include "deploy/hcom/rank_parser.h"
#include "framework/common/debug/ge_log.h"

namespace ge {
static void ClusterDataAddLocalNodes(const std::string &local_addr, const std::vector<DeviceInfo> &devices,
                                     std::vector<ClusterNodeInfo> &nodes) {
  ClusterNodeInfo host_node;

  host_node.node_type = ClusterNodeType::kHost;
  host_node.ipaddr = local_addr;
  host_node.ranks = RankParser::GetInstance().GetRanks(0);
  host_node.device_id = 0;
  nodes.emplace_back(host_node);

  for (const auto &device : devices) {
    ClusterNodeInfo device_node;
    device_node.node_type = ClusterNodeType::kDevice;
    device_node.device_id = device.GetDeviceId();
    device_node.ipaddr = device.GetHostIp();
    uint32_t type = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankType();
    device_node.ranks.emplace_back(type);
    nodes.emplace_back(device_node);
  }
}

void ClusterChiefData::Init(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices) {
  member_info_ = member_info;
  std::atomic_init(&current_member_num_, 1U);
  std::atomic_init(&finished_member_num_, 1U);
  node_.clear();
  ClusterDataAddLocalNodes(member_info_.local_addr, devices, node_);
}

bool ClusterChiefData::CheckAndSetVaildHost(const std::string &addr) {
  auto find = member_info_.members.find(addr);
  if (find != member_info_.members.end()) {
    if (find->second == ClusterMemberStatus::kInit) {
      find->second = ClusterMemberStatus::kRegisted;
    } else {
      REPORT_INNER_ERROR("E19999", "Repeated register addr:[%s].", addr.c_str());
      GELOGE(FAILED, "[Cluster][Api] Repeated register addr:[%s].", addr.c_str());
      return false;
    }
  } else {
    REPORT_INNER_ERROR("E19999", "Unknown register addr:[%s].", addr.c_str());
    GELOGE(FAILED, "[Cluster][Api] Unknown register addr:[%s].", addr.c_str());
    return false;
  }
  return true;
}

Status ClusterChiefData::AddNode(ClusterNodeInfo node) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (node.node_type == ClusterNodeType::kHost) {
    if (!CheckAndSetVaildHost(node.ipaddr)) {
      return FALSE;
    }
  }
  node_.emplace_back(node);
  return SUCCESS;
}

void ClusterMemberData::Init(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices) {
  member_info_ = member_info;
  current_member_num_ = 1;
  node_.clear();
  ClusterDataAddLocalNodes(member_info_.local_addr, devices, node_);
}
}  // namespace ge
