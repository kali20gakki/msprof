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
#ifndef RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_PARSER_H_
#define RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_PARSER_H_

#include <map>
#include <string>
#include "nlohmann/json.hpp"
#include "ge/ge_api_error_codes.h"

namespace ge {
enum class ClusterMemberType {
  kChief = 0,
  kMember = 1,
};

enum class ClusterMemberStatus {
  kInit = 0,
  kRegisted = 1,
};

struct ClusterMemberInfo {
  ClusterMemberType member_type;
  std::string local_addr;
  std::string chief_addr;
  uint64_t cluster_member_num;
  std::map<std::string, ClusterMemberStatus> members;
};

class ClusterParser {
 public:
  ge::Status MemberParser(ClusterMemberInfo &member_info);

 private:
  Status CheckIpAddr(const std::string &ipaddr);
  Status ParseMembers(const nlohmann::json &j, ClusterMemberInfo &member_info);
  Status ParseChief(const nlohmann::json &j, ClusterMemberInfo &member_info);
  Status ParseMemberType(ClusterMemberInfo &member_info);
};
}  // namespace ge
#endif  // RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_PARSER_H_
