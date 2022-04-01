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

#include "deploy/hcom/cluster/cluster_service_impl.h"
#include <sstream>
#include <vector>
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/cluster/cluster_data.h"

namespace ge {
void ClusterServiceImpl::RegisterMemberToChief(const cluster::MemberRequest *request,
                                               cluster::ClusterResponse *response) {
  if (request->node_info_size() < 1) {
    REPORT_INNER_ERROR("E19999", "Register node size:[%d] is wrong.", request->node_info_size());
    GELOGE(FAILED, "[Cluster][Api] Register node size:[%d] is wrong.", request->node_info_size());
    response->set_error_code(cluster::kClusterFailed);
    response->set_error_message("Request node number is wrong.");
    return;
  }

  for (int32_t i = 0; i < request->node_info_size(); i++) {
    ClusterNodeInfo node;
    const auto &request_node = request->node_info(i);
    if (request_node.node_type() == cluster::kHostNode) {
      node.node_type = ClusterNodeType::kHost;
    } else {
      node.node_type = ClusterNodeType::kDevice;
    }

    node.ipaddr = request_node.ipaddr();
    node.device_id = request_node.device_id();
    for (int32_t j = 0; j < request_node.rankid_size(); j++) {
      node.ranks.emplace_back(request_node.rankid(j));
    }

    if (ClusterChiefData::GetInstance().AddNode(node) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Register node addr:[%s] is wrong.", request_node.ipaddr().c_str());
      GELOGE(FAILED, "[Cluster][Api] Register node addr:[%s] is wrong.", request_node.ipaddr().c_str());
      response->set_error_code(cluster::kClusterFailed);
      response->set_error_message("Request node addr is wrong.");
      return;
    }
  }

  ClusterChiefData::GetInstance().AddMember();
  response->set_error_code(cluster::kClusterSuccess);
}

void ClusterServiceImpl::QueryAllNodes(const cluster::MemberRequest *request,
                                       cluster::ClusterResponse *response) {
  if (!ClusterChiefData::GetInstance().IsRegisterOk()) {
    response->set_error_code(cluster::kClusterWaiting);
    std::stringstream reply;
    reply << "Chief is waiting:" << ClusterChiefData::GetInstance().GetCurrentNum();
    reply << ":" << ClusterChiefData::GetInstance().GetAllNum();
    response->set_error_message(reply.str());
    return;
  }

  const std::vector<ClusterNodeInfo> &node_list = ClusterChiefData::GetInstance().GetNodeList();
  for (const auto &node : node_list) {
    auto reply_node = response->add_node_info();
    if (node.node_type == ClusterNodeType::kHost) {
      reply_node->set_node_type(cluster::kHostNode);
    } else {
      reply_node->set_node_type(cluster::kDeviceNode);
    }

    reply_node->set_ipaddr(node.ipaddr);
    reply_node->set_device_id(node.device_id);
    for (const auto &rank : node.ranks) {
      reply_node->add_rankid(rank);
    }
  }
  response->set_error_code(cluster::kClusterSuccess);
}

void ClusterServiceImpl::RegisterFinished(const cluster::MemberRequest *request, cluster::ClusterResponse *response) {
  if (request->node_info_size() != 1) {
    REPORT_INNER_ERROR("E19999", "Register finished node size:[%d] is wrong.", request->node_info_size());
    GELOGE(FAILED, "[Cluster][Api] Register finished node size:[%d] is wrong.", request->node_info_size());
    response->set_error_code(cluster::kClusterFailed);
    response->set_error_message("Register finished node size is wrong");
    return;
  }

  const auto &request_node = request->node_info(0);
  ClusterChiefData::GetInstance().SetFinished(request_node.ipaddr());
  response->set_error_code(cluster::kClusterSuccess);
}
}  // namespace ge
