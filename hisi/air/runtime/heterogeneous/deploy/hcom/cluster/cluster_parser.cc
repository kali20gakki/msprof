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
#include <vector>
#include <map>
#include <cstdlib>
#include <utility>

#include "nlohmann/json.hpp"
#include "mmpa/mmpa_api.h"
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/cluster/cluster_server.h"

#include "deploy/hcom/cluster/cluster_parser.h"

namespace ge {
namespace {
constexpr uint32_t kNumberForChief = 1;
}

Status ClusterParser::CheckIpAddr(const std::string &ipaddr) {
  if (ipaddr.empty()) {
    GELOGE(FAILED, "[Check][Env]IP address is invaild.");
    REPORT_INNER_ERROR("E19999", "IP address is invaild.");
    return FAILED;
  }

  return SUCCESS;
}

Status ClusterParser::ParseMembers(const nlohmann::json &j, ClusterMemberInfo &member_info) {
  std::vector<std::string> member;
  if (!j.contains("worker")) {
    // only set chief, for 2PG or 1P
    member_info.cluster_member_num = kNumberForChief;
    GELOGI( "[Check][Env] The format of cluster env does not have worker");
    return SUCCESS;
  }

  try {
    member = j.at("worker").get<std::vector<std::string>>();
  } catch (std::exception &e) {
    GELOGE(FAILED, "[Check][Env]The format of cluster env is wrong,%s", e.what());
    REPORT_CALL_ERROR("E19999", "The format of cluster env is wrong,%s", e.what());
    return FAILED;
  }
  //  转换成memberInfo
  member_info.cluster_member_num = member.size() + kNumberForChief;
  for (const auto &m : member) {
    const auto &find = member_info.members.find(m);
    if (find != member_info.members.end()) {
      GELOGE(FAILED, "[Check][Env]Repeated ip address.");
      REPORT_CALL_ERROR("E19999", "Repeated ip address.");
      return FAILED;
    }
    if (CheckIpAddr(m) != SUCCESS) {
      return FAILED;
    }
    member_info.members.emplace(std::make_pair(m, ClusterMemberStatus::kInit));
  }
  return SUCCESS;
}

Status ClusterParser::ParseChief(const nlohmann::json &j, ClusterMemberInfo &member_info) {
  if (j.contains("chief")) {
    member_info.chief_addr = j.at("chief").get<std::string>();
    if (CheckIpAddr(member_info.chief_addr) != SUCCESS) {
      return FAILED;
    }
    return SUCCESS;
  }

  GELOGE(FAILED, "[Check][Env]The cluster env dont have chief info.");
  REPORT_INNER_ERROR("E19999", "The cluster env dont have chief info.");
  return FAILED;
}

Status ClusterParser::ParseMemberType(ClusterMemberInfo &member_info) {
  if (ClusterServer::GetInstance().StartClusterServer(member_info.chief_addr) == SUCCESS) {
    member_info.local_addr = member_info.chief_addr;
    member_info.member_type = ClusterMemberType::kChief;
  } else {
    member_info.local_addr = member_info.members.begin()->first;
    member_info.member_type = ClusterMemberType::kMember;
  }

  GELOGI("[ClusterParser][Type] Local IP[%s] member type is %s", member_info.local_addr.c_str(),
         ((member_info.member_type == ClusterMemberType::kChief) ? "Chief" : "Member"));
  return SUCCESS;
}

const int32_t kMaxClusterEnvStrLen = 1024;
Status ClusterParser::MemberParser(ClusterMemberInfo &member_info) {
  //  读环境变量
  char_t help_cluster[kMaxClusterEnvStrLen];
  int32_t ret = mmGetEnv("HELP_CLUSTER", help_cluster, kMaxClusterEnvStrLen);
  if (ret != 0) {
    GELOGE(FAILED, "[Check][Env]Check cluster env failed.");
    REPORT_INNER_ERROR("E19999", "Check cluster env failed.");
    return FAILED;
  }
  GELOGI("[ClusterParser][Env] Help cluster is %s", help_cluster);
  //  转换成json
  nlohmann::json j;

  try {
    j = nlohmann::json::parse(help_cluster);
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Check][Env]Invalid json file,env:%s,exception:%s", help_cluster, e.what());
    REPORT_CALL_ERROR("E19999", "Invalid cluster env,env:%s,exception:%s", help_cluster, e.what());
    return FAILED;
  }

  if (ParseMembers(j, member_info) != SUCCESS) {
    return FAILED;
  }

  if (ParseChief(j, member_info) != SUCCESS) {
    return FAILED;
  }

  if (ParseMemberType(member_info) != SUCCESS) {
    return FAILED;
  }
  GELOGI("[ClusterParser][Env] Local addr is %s, chief addr is %s, member num is %lu", member_info.local_addr.c_str(),
         member_info.chief_addr.c_str(), member_info.cluster_member_num);

  return SUCCESS;
}
}  // namespace ge
