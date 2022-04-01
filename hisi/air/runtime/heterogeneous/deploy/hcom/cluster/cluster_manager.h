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
#ifndef RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_MANAGER_H_
#define RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_MANAGER_H_

#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <memory>

#include "ge/ge_api_error_codes.h"
#include "deploy/hcom/cluster/cluster_data.h"

namespace ge {
class ClusterManager {
 public:
  ClusterManager() = default;
  virtual ~ClusterManager() = default;
  virtual Status Init() = 0;
  virtual const std::vector<ClusterNodeInfo> &GetNodeList() = 0;

 protected:
  virtual void SetTimeout(uint32_t timeout) = 0;
  virtual void SetWaitInterval(uint32_t wait_interval) = 0;
  Status TryUntil(uint32_t wait_interval, uint32_t timeout, std::function<bool()> try_function);
};
 
class ClusterManagerFactory {
 public:
  std::unique_ptr<ClusterManager> Create(std::string local_addr, const std::vector<DeviceInfo> &devices);
};
}  // namespace ge
#endif  // RUNTIME_COMMUNICATION_CLUSTER_CLUSTER_MANAGER_H_
