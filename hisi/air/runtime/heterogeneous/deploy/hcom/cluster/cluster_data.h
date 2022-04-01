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
#ifndef RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_DATA_H_
#define RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_DATA_H_
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "deploy/hcom/cluster/cluster_parser.h"
#include "deploy/resource/device_info.h"

namespace ge {

enum class ClusterNodeType { kHost = 0, kDevice = 1 };

struct ClusterNodeInfo {
  ClusterNodeType node_type;
  std::string ipaddr;
  std::vector<uint32_t> ranks;
  uint32_t device_id;
};

class ClusterChiefData {
 public:
  void Init(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices);

  static ClusterChiefData &GetInstance() {
    static ClusterChiefData instance;
    return instance;
  }

  const std::vector<ClusterNodeInfo> &GetNodeList() {
    return node_;
  }

  Status RegisterChiefSelf() {
    return SUCCESS;
  }

  bool IsRegisterOk() {
    return (member_info_.cluster_member_num == current_member_num_.load());
  }

  bool IsFinished() {
    return (member_info_.cluster_member_num == finished_member_num_.load());
  }

  uint64_t GetCurrentNum() {
    return current_member_num_;
  }
  uint64_t GetAllNum() {
    return member_info_.cluster_member_num;
  }

  Status AddNode(ClusterNodeInfo node);

  void AddMember() {
    current_member_num_++;
  }

  void SetFinished(__attribute__((unused)) const std::string &ipaddr) {
    finished_member_num_++;
  }

  const std::string &GetChiefAddr() {
    return member_info_.chief_addr;
  }

 private:
  bool CheckAndSetVaildHost(const std::string &addr);
  std::vector<ClusterNodeInfo> node_;
  ClusterMemberInfo member_info_;
  std::mutex mutex_;
  std::atomic<uint32_t> current_member_num_;
  std::atomic<uint32_t> finished_member_num_;
};

class ClusterMemberData {
 public:
  void Init(const ClusterMemberInfo &member_info, const std::vector<DeviceInfo> &devices);

  static ClusterMemberData &GetInstance() {
    static thread_local ClusterMemberData instance;
    return instance;
  }

  const std::vector<ClusterNodeInfo> &GetNodeList() {
    return node_;
  }

  void AddNode(const ClusterNodeInfo &node) {
    node_.emplace_back(node);
    if (node.node_type == ClusterNodeType::kHost) {
      current_member_num_++;
    }
  }

  void ClearNodeList() {
    node_.clear();
  }

  bool IsRegisterOk() {
    return (member_info_.cluster_member_num == current_member_num_);
  }

  const std::string &GetChiefAddr() {
   return member_info_.chief_addr;
  }

 private:
  std::vector<ClusterNodeInfo> node_;
  ClusterMemberInfo member_info_;
  uint32_t current_member_num_;
};
}  // namespace ge
#endif  // RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_DATA_H_
