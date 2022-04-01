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
#include "deploy/flowrm/helper_exchange_deployer.h"
#include "deploy/flowrm/datagw_manager.h"
#include "common/data_flow/queue/helper_exchange_service.h"
#include "common/utils/rts_api_utils.h"
#include "framework/common/util.h"
#include "securec.h"
#include "exec_runtime/deploy/exchange_service.h"

namespace ge {
Status ExchangeRoute::GetQueueId(int32_t queue_index, uint32_t &queue_id) const {
  auto endpoint = MutableEndpoint(queue_index, ExchangeRoute::kEndpointTypeQueue);
  GE_CHECK_NOTNULL(endpoint);
  queue_id = endpoint->id;
  return SUCCESS;
}

Status ExchangeRoute::GetQueueIds(const std::vector<int32_t> &queue_indices, std::vector<uint32_t> &queue_ids) const {
  for (auto queue_index : queue_indices) {
    uint32_t queue_id = UINT32_MAX;
    GE_CHK_STATUS_RET_NOLOG(GetQueueId(queue_index, queue_id));
    queue_ids.emplace_back(queue_id);
  }
  return SUCCESS;
}

ExchangeRoute::ExchangeEndpoint *ExchangeRoute::MutableEndpoint(int32_t index, int32_t expect_type) const {
  auto it = endpoints_.find(index);
  if (it == endpoints_.end()) {
    GELOGE(PARAM_INVALID, "Failed to get endpoint by index: %d", index);
    return nullptr;
  }

  if (it->second.type != expect_type) {
    GELOGE(PARAM_INVALID,
           "Endpoint type mismatches, index = %d, expect = %d, but = %d",
           index,
           expect_type,
           it->second.type);
    return nullptr;
  }
  return &it->second;
}

HelperExchangeDeployer::~HelperExchangeDeployer() {
  (void) Undeploy(exchange_service_, deploying_);
}

Status HelperExchangeDeployer::PreDeploy() {
  if (!pre_deployed_) {
    GE_CHK_STATUS_RET(CreateExchangeEndpoints(), "Failed to create endpoints");
    GE_CHK_STATUS_RET(CreateGroups(), "Failed to create groups");
    pre_deployed_ = true;
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::Deploy(ExchangeRoute &deployed) {
  deploying_.device_id_ = local_device_id_;
  GE_CHK_STATUS_RET_NOLOG(PreDeploy());
  GE_CHK_STATUS_RET_NOLOG(BindEndpoints());
  deployed = std::move(deploying_);
  GELOGI("Build exchange route successfully.");
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateExchangeEndpoints() {
  std::set<int32_t> src_endpoint_indices;
  for (int32_t i = 0; i < exchange_plan_.bindings_size(); ++i) {
    const auto &binding = exchange_plan_.bindings(i);
    src_endpoint_indices.emplace(binding.src_queue_index());
  }
  for (int32_t i = 0; i < exchange_plan_.queues_size(); ++i) {
    const auto &queue_desc = exchange_plan_.queues(i);
    ExchangeRoute::ExchangeEndpoint endpoint{};
    endpoint.type = queue_desc.type();
    endpoint.name = queue_desc.name();
    if (queue_desc.type() == ExchangeRoute::kEndpointTypeQueue) {
      uint32_t work_mode = RT_MQ_MODE_PULL;
      if (src_endpoint_indices.find(i) != src_endpoint_indices.end()) {
        GELOGD("Queue[%s] used as a binding source, set work mode to PUSH", endpoint.name.c_str());
        work_mode = RT_MQ_MODE_PUSH;
      }
      GE_CHK_STATUS_RET(exchange_service_.CreateQueue(local_device_id_,
                                                      queue_desc.name(),
                                                      queue_desc.depth(),
                                                      work_mode,
                                                      endpoint.id),
                        "Failed to create queue, queue_name = %s",
                        queue_desc.name().c_str());
      endpoint.initialized = true;
    } else if (queue_desc.type() == ExchangeRoute::kEndpointTypeExternalQueue) {
      GE_CHK_STATUS_RET(RtsApiUtils::MemQueryGetQidByName(local_device_id_, endpoint.name, endpoint.id),
                        "Failed to get queue_id by name: %s", endpoint.name.c_str());
      GELOGD("External queue, queue_name = %s, queue_id = %u", endpoint.name.c_str(), endpoint.id);
    } else {
      // skip
    }
    endpoint.device_id = local_device_id_;
    deploying_.endpoints_.emplace(i, endpoint);
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateGroupSrcTags(const int32_t group_indice,
                                                  const deployer::QueueDesc *src_queue_desc,
                                                  const deployer::QueueDesc *dst_queue_desc,
                                                  ExchangeRoute::ExchangeEndpoint *src_endpoint) {
  for (int32_t i = 0; i < src_queue_desc->queue_indices_size(); ++i) {
    const auto endpoint_indice = src_queue_desc->queue_indices(i);
    src_endpoint->endpoint_indices.emplace_back(endpoint_indice);
    auto queue_desc = GetQueueDesc(endpoint_indice);
    auto endpoint = deploying_.MutableEndpoint(endpoint_indice, queue_desc->type());
    GE_CHECK_NOTNULL(endpoint);
    if (endpoint->type == ExchangeRoute::kEndpointTypeTag) {
      if (!endpoint->initialized) {
        endpoint->name = endpoint->name + queue_desc->key() + "_to_" + dst_queue_desc->key();
        GE_CHK_STATUS_RET_NOLOG(CreateHcomHandle(*dst_queue_desc, *queue_desc, *endpoint));
        GE_CHK_STATUS_RET_NOLOG(CreateTag(*endpoint));
        endpoint->initialized = true;
      }
    }
  }

  if (src_queue_desc->queue_indices_size() > 1) {
    GE_CHK_STATUS_RET_NOLOG(CreateGroup(group_indice, *src_endpoint));
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateGroupDstTags(const int32_t group_indice,
                                                  const deployer::QueueDesc *src_queue_desc,
                                                  const deployer::QueueDesc *dst_queue_desc,
                                                  ExchangeRoute::ExchangeEndpoint *dst_endpoint) {
  for (int32_t i = 0; i < dst_queue_desc->queue_indices_size(); ++i) {
    const auto endpoint_indice = dst_queue_desc->queue_indices(i);
    dst_endpoint->endpoint_indices.emplace_back(endpoint_indice);
    auto queue_desc = GetQueueDesc(endpoint_indice);
    auto endpoint = deploying_.MutableEndpoint(endpoint_indice, queue_desc->type());
    GE_CHECK_NOTNULL(endpoint);
    if (endpoint->type == ExchangeRoute::kEndpointTypeTag) {
      endpoint->name = endpoint->name + src_queue_desc->key() + "_to_" + queue_desc->key();
      GE_CHK_STATUS_RET_NOLOG(CreateHcomHandle(*src_queue_desc, *queue_desc, *endpoint));
      GE_CHK_STATUS_RET_NOLOG(CreateTag(*endpoint));
    }
  }

  if (dst_queue_desc->queue_indices_size() > 1) {
    GE_CHK_STATUS_RET_NOLOG(CreateGroup(group_indice, *dst_endpoint));
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateGroups() {
  for (int32_t i = 0; i < exchange_plan_.bindings_size(); ++i) {
    const auto &binding = exchange_plan_.bindings(i);
    auto src_queue_desc = GetQueueDesc(binding.src_queue_index());
    GE_CHECK_NOTNULL(src_queue_desc);
    auto dst_queue_desc = GetQueueDesc(binding.dst_queue_index());
    GE_CHECK_NOTNULL(dst_queue_desc);
    if ((src_queue_desc->type() == ExchangeRoute::kEndpointTypeGroup) &&
        (dst_queue_desc->type() == ExchangeRoute::kEndpointTypeGroup)) {
      GELOGE(INTERNAL_ERROR,
             "Should not bind group with group, src index = %d, dst index = %d",
             binding.src_queue_index(),
             binding.dst_queue_index());
      return INTERNAL_ERROR;
    }

    auto src_endpoint = deploying_.MutableEndpoint(binding.src_queue_index(), src_queue_desc->type());
    GE_CHECK_NOTNULL(src_endpoint);
    auto dst_endpoint = deploying_.MutableEndpoint(binding.dst_queue_index(), dst_queue_desc->type());
    GE_CHECK_NOTNULL(dst_endpoint);
    if (src_endpoint->type == ExchangeRoute::kEndpointTypeGroup) {
      GE_CHK_STATUS_RET_NOLOG(CreateGroupSrcTags(binding.src_queue_index(),
                                                 src_queue_desc,
                                                 dst_queue_desc,
                                                 src_endpoint));
    } else if (dst_endpoint->type == ExchangeRoute::kEndpointTypeGroup) {
      GE_CHK_STATUS_RET_NOLOG(CreateGroupDstTags(binding.dst_queue_index(),
                                                 src_queue_desc,
                                                 dst_queue_desc,
                                                 dst_endpoint));
    } else {
      GELOGD("Bind [%s] -> [%s]", src_endpoint->name.c_str(), dst_endpoint->name.c_str());
    }
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::BindEndpoints() {
  std::vector<bqs::Route> queue_routes;
  for (int32_t i = 0; i < exchange_plan_.bindings_size(); ++i) {
    const auto &binding = exchange_plan_.bindings(i);
    // pointers were checked in CreateTags
    auto src_queue_desc = GetQueueDesc(binding.src_queue_index());
    auto dst_queue_desc = GetQueueDesc(binding.dst_queue_index());
    auto src_endpoint = deploying_.MutableEndpoint(binding.src_queue_index(), src_queue_desc->type());
    auto dst_endpoint = deploying_.MutableEndpoint(binding.dst_queue_index(), dst_queue_desc->type());
    GELOGD("Bind endpoint[%s] {index[%d], type[%d]} -> {index[%d], type[%d]}",
           src_endpoint->name.c_str(), binding.src_queue_index(), src_endpoint->type,
           binding.dst_queue_index(), dst_endpoint->type);
    bqs::Route queue_route = {};
    if ((src_endpoint->type == ExchangeRoute::kEndpointTypeGroup) &&
        (src_endpoint->endpoint_indices.size() == 1U)) {
      auto indice = src_endpoint->endpoint_indices[0];
      auto queue_desc = GetQueueDesc(indice);
      auto endpoint = deploying_.MutableEndpoint(indice, queue_desc->type());
      queue_route.src = ToBqsEndpoint(*endpoint);
      GELOGD("Bind src endpoint is group, element size = 1, change src index[%d -> %d], type[%d], id[%u]",
             binding.src_queue_index(), indice, queue_desc->type(), endpoint->id);
    } else {
      queue_route.src = ToBqsEndpoint(*src_endpoint);
      GELOGD("Bind src endpoint, bind index[%d], type[%d], id[%u]",
             binding.src_queue_index(), src_endpoint->type, src_endpoint->id);
    }

    if ((dst_endpoint->type == ExchangeRoute::kEndpointTypeGroup) &&
        (dst_endpoint->endpoint_indices.size() == 1U)) {
      auto indice = dst_endpoint->endpoint_indices[0];
      auto queue_desc = GetQueueDesc(indice);
      auto endpoint = deploying_.MutableEndpoint(indice, queue_desc->type());
      queue_route.dst = ToBqsEndpoint(*endpoint);
      GELOGD("Bind dst endpoint is group, element size = 1, change dst index[%d -> %d], type[%d], id[%u]",
             binding.dst_queue_index(), indice, queue_desc->type(), endpoint->id);
    } else {
      queue_route.dst = ToBqsEndpoint(*dst_endpoint);
      GELOGD("Bind dst endpoint, bind index[%d], type[%d], id[%u]",
             binding.dst_queue_index(), dst_endpoint->type, dst_endpoint->id);
    }
    queue_routes.emplace_back(queue_route);
    deploying_.queue_routes_.emplace_back(queue_route);
  }
  GE_CHK_STATUS_RET_NOLOG(BindRoute(queue_routes));
  return SUCCESS;
}

Status HelperExchangeDeployer::CopyAttr(char_t *dst, const size_t dst_size, const char_t *src) {
  if (strcpy_s(dst, dst_size, src) != EOK) {
    GELOGE(FAILED, "Copy attr failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status HelperExchangeDeployer::FillNamedChannelEndpoint(const ExchangeRoute::ExchangeEndpoint &endpoint,
                                                        bqs::Endpoint &ret) {
  ret.type = bqs::EndpointType::NAMED_COMM_CHANNEL;
  GE_CHK_STATUS_RET(CopyAttr(ret.attr.namedChannelAttr.localIp,
                             sizeof(ret.attr.namedChannelAttr.localIp),
                             endpoint.local_ip.c_str()),
                    "Failed to copy local ip, ip = %s", endpoint.local_ip.c_str());
  ret.attr.namedChannelAttr.localPort = endpoint.local_port;
  GE_CHK_STATUS_RET(CopyAttr(ret.attr.namedChannelAttr.peerIp,
                             sizeof(ret.attr.namedChannelAttr.peerIp),
                             endpoint.peer_ip.c_str()),
                    "Failed to copy peer ip, ip = %s", endpoint.peer_ip.c_str());
  ret.attr.namedChannelAttr.peerPort = endpoint.peer_port;
  GE_CHK_STATUS_RET(CopyAttr(ret.attr.namedChannelAttr.name,
                             sizeof(ret.attr.namedChannelAttr.name),
                             endpoint.name.c_str()),
                    "Failed to copy name, name = %s", endpoint.name.c_str());
  return SUCCESS;
}

bqs::Endpoint HelperExchangeDeployer::ToBqsEndpoint(const ExchangeRoute::ExchangeEndpoint &endpoint) {
  bqs::Endpoint ret = {};
  switch (endpoint.type) {
    case ExchangeRoute::kEndpointTypeTag: {
      (void)FillNamedChannelEndpoint(endpoint, ret);
      break;
    }
    case ExchangeRoute::kEndpointTypeExternalQueue:  // fall through
    case ExchangeRoute::kEndpointTypeQueue: {
      ret.type = bqs::EndpointType::QUEUE;
      ret.attr.queueAttr.queueId = endpoint.id;
      break;
    }
    case ExchangeRoute::kEndpointTypeGroup: {
      ret.type = bqs::EndpointType::GROUP;
      ret.attr.groupAttr.groupId = endpoint.id;
      break;
    }
    default: {
      break;
    }
  }
  return ret;
}

const deployer::QueueDesc *HelperExchangeDeployer::GetQueueDesc(int32_t index) {
  if (index < 0 || index >= exchange_plan_.queues_size()) {
    GELOGE(PARAM_INVALID, "index out of range, queue size = %d, index = %d", exchange_plan_.queues_size(), index);
    return nullptr;
  }

  return &exchange_plan_.queues(index);
}

Status HelperExchangeDeployer::CreateHcomHandle(const deployer::QueueDesc &local,
                                                const deployer::QueueDesc &peer,
                                                ExchangeRoute::ExchangeEndpoint &endpoint) {
  bqs::CreateHcomInfo hcom_info{};
  GE_CHK_STATUS_RET(CopyAttr(hcom_info.masterIp, sizeof(hcom_info.masterIp), exchange_plan_.master_ip().c_str()),
                    "Failed to copy master ip, ip = %s", exchange_plan_.master_ip().c_str());
  hcom_info.masterPort = exchange_plan_.master_port();
  GE_CHK_STATUS_RET(CopyAttr(hcom_info.localIp, sizeof(hcom_info.localIp), local.host_ip().c_str()),
                    "Failed to copy local ip, ip = %s", local.host_ip().c_str());
  hcom_info.localPort = local.port();
  endpoint.local_ip = local.host_ip();
  endpoint.local_port = local.port();
  GE_CHK_STATUS_RET(CopyAttr(hcom_info.remoteIp, sizeof(hcom_info.remoteIp), peer.host_ip().c_str()),
                    "Failed to copy remote ip, ip = %s", peer.host_ip().c_str());
  hcom_info.remotePort = peer.port();
  endpoint.peer_ip = peer.host_ip();
  endpoint.peer_port = peer.port();
  hcom_info.handle = 0U;
  GE_CHK_STATUS_RET(DataGwManager::GetInstance().CreateDataGwHandle(static_cast<uint32_t>(local_device_id_), hcom_info),
                    "Failed to create hcom handle");
  endpoint.hcom_handle = hcom_info.handle;
  GELOGD("hcom handle created, handle = %lu", endpoint.hcom_handle);
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateTag(ExchangeRoute::ExchangeEndpoint &endpoint) const {
  int32_t hcom_tag = 0;
  GE_CHK_STATUS_RET(DataGwManager::GetInstance().CreateDataGwTag(endpoint.hcom_handle,
                                                                 endpoint.name,
                                                                 static_cast<uint32_t>(local_device_id_),
                                                                 hcom_tag),
                    "Failed to create tag, name = %s", endpoint.name.c_str());
  endpoint.id = static_cast<uint32_t>(hcom_tag);
  GELOGD("HCom tag created, name = %s, tag = %d", endpoint.name.c_str(), hcom_tag);
  return SUCCESS;
}

Status HelperExchangeDeployer::CreateGroup(const int32_t group_indice,
                                           ExchangeRoute::ExchangeEndpoint &group_endpoint) {
  int32_t group_id = 0;
  std::vector<bqs::Endpoint> endpoint_list;
  for (const auto indice : group_endpoint.endpoint_indices) {
    auto endpoint_desc = GetQueueDesc(indice);
    auto endpoint = deploying_.MutableEndpoint(indice, endpoint_desc->type());
    if (endpoint->type == ExchangeRoute::kEndpointTypeGroup) {
      GELOGE(INTERNAL_ERROR, "Type of endpoint of group can't be group");
      return INTERNAL_ERROR;
    }
    endpoint_list.emplace_back(ToBqsEndpoint(*endpoint));
  }
  GE_CHK_STATUS_RET(DataGwManager::GetInstance().CreateDataGwGroup(static_cast<uint32_t>(local_device_id_),
                                                                   endpoint_list,
                                                                   group_id),
                    "Failed to create group, name = %s", group_endpoint.name.c_str());
  group_endpoint.id = static_cast<uint32_t>(group_id);
  deploying_.groups_[group_indice] = endpoint_list;
  GELOGD("DataGW group created, name = %s, group id = %d", group_endpoint.name.c_str(), group_id);
  return SUCCESS;
}

Status HelperExchangeDeployer::BindRoute(const std::vector<bqs::Route> &queue_routes) {
  return DataGwManager::GetInstance().BindQueues(static_cast<uint32_t>(local_device_id_), queue_routes);
}

Status HelperExchangeDeployer::UnbindRoute(int32_t device_id, const bqs::Route &queue_route) {
  return DataGwManager::GetInstance().UnbindQueues(static_cast<uint32_t>(device_id), queue_route);
}

Status HelperExchangeDeployer::Undeploy(ExchangeService &exchange_service, const ExchangeRoute &deployed) {
  for (const auto &queue_route : deployed.queue_routes_) {
    (void) UnbindRoute(deployed.device_id_, queue_route);
  }
  std::map<int32_t, std::set<uint64_t>> hcom_handles;
  for (const auto &it : deployed.endpoints_) {
    const auto &endpoint = it.second;
    if (endpoint.type == ExchangeRoute::kEndpointTypeQueue) {
      exchange_service.DestroyQueue(endpoint.device_id, endpoint.id);
    } else if (endpoint.type == ExchangeRoute::kEndpointTypeTag) {
      GELOGD("[Destroy][Tag] name = %s, handle = %lu, id = %u",
             endpoint.name.c_str(),
             endpoint.hcom_handle,
             endpoint.id);
      (void) DataGwManager::GetInstance().DestroyDataGwTag(endpoint.device_id,
                                                           endpoint.hcom_handle,
                                                           static_cast<int32_t>(endpoint.id));
      hcom_handles[endpoint.device_id].emplace(endpoint.hcom_handle);
    } else if (endpoint.type == ExchangeRoute::kEndpointTypeGroup) {
      (void) DataGwManager::GetInstance().DestroyDataGwGroup(endpoint.device_id,
                                                             static_cast<int32_t>(endpoint.id));
    } else {
      // do nothing, actually will not happen
    }
  }

  for (const auto &it : hcom_handles) {
    int32_t device_id = it.first;
    const auto &handles = it.second;
    for (auto handle : handles) {
      (void) DataGwManager::GetInstance().DestroyDataGwHandle(device_id, handle);
      GELOGD("[Destroy][HcomHandle] success, device_id = %d, handle = %lu", device_id, handle);
    }
  }

  return SUCCESS;
}
}  // namespace ge
