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
#ifndef AIR_RUNTIME_HETEROGENEOUS_FLOWRM_HELPER_EXCHANGE_DEPLOYER_H_
#define AIR_RUNTIME_HETEROGENEOUS_FLOWRM_HELPER_EXCHANGE_DEPLOYER_H_

#include <cstdint>
#include <vector>
#include "exec_runtime/deploy/exchange_service.h"
#include "aicpu/queue_schedule/qs_client.h"
#include "aicpu/queue_schedule/dgw_client.h"
#include "external/ge/ge_api_error_codes.h"
#include "proto/deployer.pb.h"

namespace ge {
class ExchangeRoute {
 public:
  Status GetQueueId(int32_t queue_index, uint32_t &queue_id) const;
  Status GetQueueIds(const std::vector<int32_t> &queue_indices, std::vector<uint32_t> &queue_ids) const;
  const std::vector<bqs::Route> &GetQueueRoutes() const {
    return queue_routes_;
  }

  static constexpr int32_t kEndpointTypeExternalQueue = 0;
  static constexpr int32_t kEndpointTypeQueue = 1;
  static constexpr int32_t kEndpointTypeTag = 2;
  static constexpr int32_t kEndpointTypeGroup = 3;

 private:
  friend class HelperExchangeDeployer;
  struct ExchangeEndpoint {
    uint32_t id = UINT32_MAX;
    int32_t type = 0;
    uint64_t hcom_handle = UINT64_MAX;
    int32_t device_id = -1;
    std::string name;
    bool initialized = false;
    std::string local_ip;
    uint32_t local_port;
    std::string peer_ip;
    uint32_t peer_port;
    std::vector<int32_t> endpoint_indices;
  };

  ExchangeEndpoint* MutableEndpoint(int32_t index, int32_t expect_type) const;

  // key: queue index, value: queue_handle
  int32_t device_id_ = 0;
  mutable std::map<int32_t, ExchangeEndpoint> endpoints_;
  std::vector<bqs::Route> queue_routes_;
  std::map<int32_t, std::vector<bqs::Endpoint>> groups_;
};

class HelperExchangeDeployer {
 public:
  HelperExchangeDeployer(ExchangeService &exchange_service,
                         const deployer::ExchangePlan &exchange_plan,
                         int32_t local_device_id)
      : exchange_service_(exchange_service), exchange_plan_(exchange_plan), local_device_id_(local_device_id) {}
  virtual ~HelperExchangeDeployer();

  Status PreDeploy();
  Status Deploy(ExchangeRoute &deployed);
  static Status Undeploy(ExchangeService &exchange_service, const ExchangeRoute &deployed);

 private:
  Status CreateExchangeEndpoints();
  Status CreateGroupSrcTags(const int32_t group_indice,
                            const deployer::QueueDesc *src_queue_desc,
                            const deployer::QueueDesc *dst_queue_desc,
                            ExchangeRoute::ExchangeEndpoint *src_endpoint);
  Status CreateGroupDstTags(const int32_t group_indice,
                            const deployer::QueueDesc *src_queue_desc,
                            const deployer::QueueDesc *dst_queue_desc,
                            ExchangeRoute::ExchangeEndpoint *dst_endpoint);
  Status CreateGroups();
  Status BindEndpoints();
  virtual Status BindRoute(const std::vector<bqs::Route> &queue_routes);
  static Status UnbindRoute(int32_t device_id, const bqs::Route &queue_route);
  virtual Status CreateTag(ExchangeRoute::ExchangeEndpoint &endpoint) const;
  virtual Status CreateHcomHandle(const deployer::QueueDesc &local,
                                  const deployer::QueueDesc &peer,
                                  ExchangeRoute::ExchangeEndpoint &endpoint);
  virtual Status CreateGroup(const int32_t group_indice, ExchangeRoute::ExchangeEndpoint &group_endpoint);
  const deployer::QueueDesc *GetQueueDesc(int32_t index);
  Status CopyAttr(char_t *dst, const size_t dst_size, const char_t *src);
  Status FillNamedChannelEndpoint(const ExchangeRoute::ExchangeEndpoint &endpoint, bqs::Endpoint &ret);
  bqs::Endpoint ToBqsEndpoint(const ExchangeRoute::ExchangeEndpoint &endpoint);

  ExchangeService &exchange_service_;
  const deployer::ExchangePlan &exchange_plan_;
  bool pre_deployed_ = false;
  int32_t local_device_id_ = -1;
  ExchangeRoute deploying_;
};
}  // namespace ge
#endif  // AIR_RUNTIME_HETEROGENEOUS_FLOWRM_HELPER_EXCHANGE_DEPLOYER_H_
