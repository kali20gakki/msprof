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

#ifndef AIR_RUNTIME_HETEROGENEOUS_FLOWRM_FLOW_ROUTE_MANAGER_H_
#define AIR_RUNTIME_HETEROGENEOUS_FLOWRM_FLOW_ROUTE_MANAGER_H_

#include <map>
#include <mutex>
#include "deploy/flowrm/helper_exchange_deployer.h"

namespace ge {
class FlowRouteManager {
 public:
  static FlowRouteManager &GetInstance() {
    static FlowRouteManager instance;
    return instance;
  }

  Status DeployRoutePlan(const deployer::ExchangePlan &exchange_plan, int32_t device_id, int64_t &route_id);
  Status AddRoute(const ExchangeRoute &route, int64_t &route_id);

  void UndeployRoute(int64_t route_id);

  const ExchangeRoute* QueryRoute(int64_t route_id);

 private:
  Status GenerateRouteId(int64_t &route_id);
  std::mutex mu_;
  int64_t route_id_gen_ = 0;
  std::map<int64_t, ExchangeRoute> exchange_routes_;  // key: route id
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_FLOWRM_FLOW_ROUTE_MANAGER_H_
