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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_NODE_INFO_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_NODE_INFO_H_

#include <cstdint>
#include <string>
#include <vector>
#include "ge/ge_api_error_codes.h"

namespace ge {
class NodeInfo {
 public:
  int32_t GetDeviceCount() const;

  void SetDeviceCount(int32_t device_count);

  const std::vector<int32_t> &GetDgwPorts() const;

  Status GetDgwPort(int32_t device_id, int32_t &port) const;

  void SetDgwPorts(const std::vector<int32_t> &dgw_ports);

  int32_t GetDeviceId() const;

  void SetDeviceId(int32_t device_id);

  const std::string &GetHostIp() const;
  void SetHostIp(const std::string &host_ip);

 private:
  std::string host_ip_;

  int32_t device_id_ = -1;
  int32_t device_count_ = 1;
  std::vector<int32_t> dgw_ports_{16666};  // default data gw ports
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_NODE_INFO_H_
