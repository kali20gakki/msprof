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
#ifndef RUNTIME_COMMUNICATION_RANK_PARSER_H_
#define RUNTIME_COMMUNICATION_RANK_PARSER_H_
#include <vector>
#include <map>
#include <cstdlib>
#include <string>

#include "mmpa/mmpa_api.h"
#include "framework/common/debug/ge_log.h"

namespace ge {
class AutoArrangeRankParser {
 public:
  AutoArrangeRankParser() {
    special_rank_status_ = 0;
    rank_index_ = 0;
  }
  ~AutoArrangeRankParser() = default;

  static AutoArrangeRankParser &GetInstance() {
    static thread_local AutoArrangeRankParser instance;
    return instance;
  }

  void Reset();

  Status GetAutoArrangeRankType();

  Status GetAutoArrangeRankId(uint32_t rank_type);

 private:
  uint32_t rank_index_;
  uint32_t special_rank_status_;
};

class RankParser {
 public:
  Status Init(const std::string &hccl_cluster_info, const std::string &model_deploy_info) {
    GELOGI("[RankParser][Rank] Cluster info is %s, deploy info is %s", hccl_cluster_info.c_str(),
           model_deploy_info.c_str());
    model_to_rank_.clear();
    device_to_ranks_.clear();
    rank_to_device_.clear();
    sub_groups_.clear();
    return ParseClusterInfo(hccl_cluster_info, model_deploy_info);
  }

  const std::vector<uint32_t> &GetRanks(uint32_t device_id) {
    return device_to_ranks_.find(device_id)->second;
  }

  std::map<std::string, std::vector<uint32_t>> &GetSubGroups() {
    return sub_groups_;
  }

  Status GetDeviceFromeRank(uint32_t rank_id, uint32_t *device) {
    const auto &find = rank_to_device_.find(rank_id);
    if (find == rank_to_device_.end()) {
      return FAILED;
    }
    *device = find->second;
    return SUCCESS;
  }

  Status GetRankFromSubModel(uint32_t model_instance_id, uint32_t *rank_id) {
    const auto &find = model_to_rank_.find(model_instance_id);
    if (find == model_to_rank_.end()) {
      return FAILED;
    }
    *rank_id = find->second;
    return SUCCESS;
  }

  static RankParser &GetInstance() {
    static RankParser instance;
    return instance;
  }

 private:
  Status ParseClusterInfo(const std::string &hccl_cluster_info, const std::string &model_deploy_info);

  std::map<uint32_t, uint32_t> model_to_rank_;
  std::map<uint32_t, std::vector<uint32_t>> device_to_ranks_;
  std::map<uint32_t, uint32_t> rank_to_device_;
  std::map<std::string, std::vector<uint32_t>> sub_groups_;  // group_to_ranK_
};
}  // namespace ge

#endif  // RUNTIME_COMMUNICATION_RANK_PARSER_H_
