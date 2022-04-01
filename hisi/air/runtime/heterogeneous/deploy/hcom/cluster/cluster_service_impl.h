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

#ifndef AIR_RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_SERVICE_IMPL_H_
#define AIR_RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_SERVICE_IMPL_H_

#include "ge/ge_api_error_codes.h"
#include "proto/cluster.pb.h"

namespace ge {
class ClusterServiceImpl {
 public:
  static void RegisterMemberToChief(const cluster::MemberRequest *request, cluster::ClusterResponse *response);
  static void QueryAllNodes(const cluster::MemberRequest *request, cluster::ClusterResponse *response);
  static void RegisterFinished(const cluster::MemberRequest *request, cluster::ClusterResponse *response);
};
}  // namespace ge

#endif  // AIR_RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_SERVICE_IMPL_H_
