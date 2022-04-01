/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "deploy/flowrm/flow_route_manager.h"
#include "common/data_flow/queue/helper_exchange_service.h"
#include "common/debug/log.h"

namespace ge {
Status FlowRouteManager::DeployRoutePlan(const deployer::ExchangePlan &exchange_plan,
                                         int32_t device_id,
                                         int64_t &route_id) {
  GE_CHK_STATUS_RET_NOLOG(GenerateRouteId(route_id));
  HelperExchangeDeployer exchange_deployer(HelperExchangeService::GetInstance(), exchange_plan, device_id);
  ExchangeRoute route;
  GE_CHK_STATUS_RET_NOLOG(exchange_deployer.Deploy(route));
  std::lock_guard<std::mutex> lk(mu_);
  exchange_routes_[route_id] = std::move(route);
  return SUCCESS;
}

Status FlowRouteManager::AddRoute(const ExchangeRoute &route, int64_t &route_id) {
  GE_CHK_STATUS_RET_NOLOG(GenerateRouteId(route_id));
  std::lock_guard<std::mutex> lk(mu_);
  exchange_routes_[route_id] = route;
  return SUCCESS;
}

Status FlowRouteManager::GenerateRouteId(int64_t &route_id) {
  std::lock_guard<std::mutex> lk(mu_);
  if (route_id_gen_ < 0) {
    GELOGE(FAILED, "Run out of route id");  // will not happen
    return FAILED;
  }
  route_id = route_id_gen_++;
  return SUCCESS;
}

void FlowRouteManager::UndeployRoute(int64_t route_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = exchange_routes_.find(route_id);
  if (it == exchange_routes_.end()) {
    GELOGW("Route not found, id = %ld", route_id);
    return;
  }

  (void) HelperExchangeDeployer::Undeploy(HelperExchangeService::GetInstance(), it->second);
  exchange_routes_.erase(it);
}

const ExchangeRoute *FlowRouteManager::QueryRoute(int64_t route_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = exchange_routes_.find(route_id);
  if (it == exchange_routes_.end()) {
    GELOGE(PARAM_INVALID, "Route not found, id = %ld", route_id);
    return nullptr;
  }

  return &it->second;
}
}  // namespace ge
