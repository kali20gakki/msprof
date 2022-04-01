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

#include "common/util.h"
#include "common/plugin/ge_util.h"
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/cluster/cluster_data.h"
#include "deploy/hcom/cluster/cluster_client.h"

namespace ge {
Status ClusterClient::CreateClient(const std::string &server_addr) {
  grpc_client_ = MakeUnique<ClusterGrpcClient>();
  GE_CHECK_NOTNULL(grpc_client_);
  return grpc_client_->Initialize(server_addr);
}

static inline void GrpcClientErrorHandle(const int32_t error_code, const std::string &error_message) {
  REPORT_INNER_ERROR("E19999", "RPC failed, gRPC error code =%d, error message=%s.", error_code, error_message.c_str());
  GELOGE(FAILED, "[Cluster][Rpc] gRPC error code =%d, error message=%s.", error_code, error_message.c_str());
}

Status ClusterClient::QueryAllNodes() {
  GE_CHECK_NOTNULL(grpc_client_);
  cluster::MemberRequest request;
  cluster::ClusterResponse response;

  GE_CHK_STATUS_RET_NOLOG(grpc_client_->QueryAllNodes(request, response));
  if (response.error_code() != cluster::kClusterSuccess) {
    GrpcClientErrorHandle(response.error_code(), response.error_message());
    return FAILED;
  }

  ClusterMemberData::GetInstance().ClearNodeList();
  for (int32_t i = 0; i < response.node_info_size(); i++) {
    ClusterNodeInfo node;
    const auto &reply = response.node_info(i);
    if (reply.node_type() == cluster::kHostNode) {
      node.node_type = ClusterNodeType::kHost;
    } else {
      node.node_type = ClusterNodeType::kDevice;
    }

    node.ipaddr = reply.ipaddr();
    node.device_id = reply.device_id();
    for (int32_t j = 0; j < reply.rankid_size(); j++) {
      node.ranks.emplace_back(reply.rankid(j));
    }

    ClusterMemberData::GetInstance().AddNode(node);
  }

  return SUCCESS;
}

Status ClusterClient::RegisterFinished() {
  GE_CHECK_NOTNULL(grpc_client_);
  cluster::MemberRequest request;
  cluster::ClusterResponse response;

  auto request_node = request.add_node_info();
  request_node->set_node_type(cluster::kHostNode);

  GE_CHK_STATUS_RET_NOLOG(grpc_client_->RegisterFinished(request, response));
  if (response.error_code() != cluster::kClusterSuccess) {
    GrpcClientErrorHandle(response.error_code(), response.error_message());
    return FAILED;
  }
  return SUCCESS;
}

Status ClusterClient::RegisterMemberToChief() {
  GE_CHECK_NOTNULL(grpc_client_);
  const std::vector<ClusterNodeInfo> &nodes = ClusterMemberData::GetInstance().GetNodeList();
  cluster::MemberRequest request;
  cluster::ClusterResponse response;
  for (const auto &node : nodes) {
    GELOGD("[Register][Chief] Register member to chief, device_id = %d", node.device_id);
    auto request_node = request.add_node_info();
    if (node.node_type == ClusterNodeType::kHost) {
      request_node->set_node_type(cluster::kHostNode);
    } else {
      request_node->set_node_type(cluster::kDeviceNode);
    }

    request_node->set_ipaddr(node.ipaddr);
    request_node->set_device_id(node.device_id);
    for (const auto &rank : node.ranks) {
      request_node->add_rankid(rank);
    }
  }

  GE_CHK_STATUS_RET_NOLOG(grpc_client_->RegisterMemberToChief(request, response));
  if (response.error_code() != cluster::kClusterSuccess) {
    GrpcClientErrorHandle(response.error_code(), response.error_message());
    return FAILED;
  }

  return SUCCESS;
}
}  // namespace ge
