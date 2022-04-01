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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_DEVICE_INFO_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_DEVICE_INFO_H_

#include <cstdint>
#include <string>

namespace ge {
class DeviceInfo {
 public:
  DeviceInfo() = default;
  DeviceInfo(int32_t node_id, int32_t device_id) noexcept;

  int32_t GetNodeId() const;
  int32_t GetDeviceId() const;
  const std::string &GetHostIp() const;
  void SetHostIp(const std::string &host_ip);
  int32_t GetDgwPort() const;
  void SetDgwPort(int32_t dgw_port);
  int32_t GetFlowRankId() const;
  void SetFlowRankId(int32_t flow_rank_id);

 private:
  int32_t node_id_ = 0;
  int32_t device_id_ = -1;
  int32_t flow_rank_id_ = -1;
  int32_t dgw_port_ = -1;
  std::string host_ip_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_DEVICE_INFO_H_
