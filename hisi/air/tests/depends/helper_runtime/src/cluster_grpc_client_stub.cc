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
#include "deploy/hcom/cluster/cluster_grpc_client.h"
#include "framework/common/debug/ge_log.h"
#include "debug/ge_util.h"
#include "common/plugin/ge_util.h"
#include "deploy/hcom/cluster/cluster_service_impl.h"
#include "proto/cluster.pb.h"

namespace ge {
class ClusterGrpcClient::Impl {
};

ClusterGrpcClient::ClusterGrpcClient() = default;
ClusterGrpcClient::~ClusterGrpcClient() = default;

Status ClusterGrpcClient::Initialize(const std::string &server_addr) {
  return SUCCESS;
}

Status ClusterGrpcClient::RegisterMemberToChief(const cluster::MemberRequest &request,
                                                cluster::ClusterResponse &response) {
  ClusterServiceImpl::RegisterMemberToChief(&request, &response);
  return SUCCESS;
}
Status ClusterGrpcClient::QueryAllNodes(const cluster::MemberRequest &request, cluster::ClusterResponse &response) {
  ClusterServiceImpl::QueryAllNodes(&request, &response);
  return SUCCESS;
}
Status ClusterGrpcClient::RegisterFinished(const cluster::MemberRequest &request, cluster::ClusterResponse &response) {
  ClusterServiceImpl::RegisterFinished(&request, &response);
  return SUCCESS;
}
}  // namespace geClusterServer