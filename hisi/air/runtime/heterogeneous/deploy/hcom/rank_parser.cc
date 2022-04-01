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
#include "deploy/hcom/rank_parser.h"
#include <utility>
#include "nlohmann/json.hpp"

namespace ge {
constexpr int32_t kMaxRankIdStrLen = 128;
constexpr int32_t kDecimalBase = 10;
constexpr uint32_t kSpecialRank = 0xaa;
constexpr uint32_t kCommonRank = 0;

struct ModelRank {
  uint32_t model;
  uint32_t rank;
};

struct ModeDevice {
  uint32_t model;
  uint32_t device;
};

struct GroupRanks {
  std::string group;
  std::vector<uint32_t> ranks;
};

void from_json(const nlohmann::json &j, struct ModelRank &d) {
  j.at("model_instance_id").get_to(d.model);
  j.at("rank_id").get_to(d.rank);
}

void from_json(const nlohmann::json &j, struct ModeDevice &d) {
  j.at("model_instance_id").get_to(d.model);
  j.at("device_id").get_to(d.device);
}

void from_json(const nlohmann::json &j, struct GroupRanks &d) {
  j.at("group_id").get_to(d.group);
  j.at("group_rank_list").get_to(d.ranks);
}

Status RankParser::ParseClusterInfo(const std::string &hccl_cluster_info, const std::string &model_deploy_info) {
  nlohmann::json j_hccl;
  nlohmann::json j_deploy;

  try {
    j_hccl = nlohmann::json::parse(hccl_cluster_info);
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Check][Json]Invalid json file, hccl info:%s,exception:%s.", hccl_cluster_info.c_str(), e.what());
    REPORT_CALL_ERROR("E19999", "Invalid json file, hccl info:%s,exception:%s.", hccl_cluster_info.c_str(), e.what());
    return FAILED;
  }

  try {
    j_deploy = nlohmann::json::parse(model_deploy_info);
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Check][Json]Invalid json file, deploy info:%s,exception:%s.", model_deploy_info.c_str(), e.what());
    REPORT_CALL_ERROR("E19999", "Invalid json file, deploy info:%s,exception:%s.", model_deploy_info.c_str(), e.what());
    return FAILED;
  }

  auto j_rank_table = j_hccl.at("rankTable");
  auto model_rank = j_rank_table.get<std::vector<struct ModelRank>>();
  for (const auto &t : model_rank) {
    model_to_rank_.emplace(std::make_pair(t.model, t.rank));
  }

  auto j_sub_groups = j_hccl.at("subGroups");
  auto group_rank = j_sub_groups.get<std::vector<struct GroupRanks>>();
  for (const auto &t : group_rank) {
    sub_groups_.emplace(std::make_pair(t.group, t.ranks));
  }

  auto model_device = j_deploy.get<std::vector<struct ModeDevice>>();
  for (const auto &t : model_device) {
    const auto &find = device_to_ranks_.find(t.device);
    if (find != device_to_ranks_.end()) {
      std::vector<uint32_t> &rs = find->second;
      const auto &f = model_to_rank_.find(t.model);
      if (f == model_to_rank_.end()) {
        GELOGE(FAILED, "[Check][Rank]Cannot find model:%d.", t.model);
        REPORT_CALL_ERROR("E19999", "Cannot find model:%d.", t.model);
        return FAILED;
      }
      rs.emplace_back(f->second);
      rank_to_device_.emplace(std::make_pair(f->second, t.device));
    } else {
      std::vector<uint32_t> rs;
      const auto &f = model_to_rank_.find(t.model);
      if (f == model_to_rank_.end()) {
        GELOGE(FAILED, "[Check][Rank]Cannot find model:%d.", t.model);
        REPORT_CALL_ERROR("E19999", "Cannot find model:%d.", t.model);
        return FAILED;
      }
      rs.emplace_back(f->second);
      device_to_ranks_.emplace(std::make_pair(t.device, rs));
      rank_to_device_.emplace(std::make_pair(f->second, t.device));
    }
  }

  return SUCCESS;
}

Status AutoArrangeRankParser::GetAutoArrangeRankType() {
  if (special_rank_status_ != 0U) {
    return kCommonRank;
  }

  char_t env[kMaxRankIdStrLen];
  int32_t ret = mmGetEnv("RANK_ID", env, kMaxRankIdStrLen);
  if (ret != 0) {
    GELOGE(FAILED, "[Check][Env]Check RankId env failed.");
    REPORT_INNER_ERROR("E19999", "Check RankId env failed.");
    return kCommonRank;
  }
  GELOGI("[RankParser][Env] RANK_ID is %s", env);
  errno = 0;
  uint32_t rank_base = std::strtol(env, nullptr, kDecimalBase);
  if (errno != 0) {
    GELOGE(FAILED, "[Check][Env]Check RankId env failed.");
    REPORT_INNER_ERROR("E19999", "Check RankId env failed.");
    return kCommonRank;
  }
  special_rank_status_ = 1;
  if (rank_base == 0) {
    return kSpecialRank;
  } else {
    return kCommonRank;
  }
}

Status AutoArrangeRankParser::GetAutoArrangeRankId(uint32_t rank_type) {
  if (rank_type == kSpecialRank) {
    return 0;
  } else {
    rank_index_++;
    return rank_index_;
  }
}

void AutoArrangeRankParser::Reset() {
  special_rank_status_ = 0;
  rank_index_ = 0;
}
}  // namespace ge
