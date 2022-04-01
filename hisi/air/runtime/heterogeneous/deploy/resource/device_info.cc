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

#include "deploy/resource/device_info.h"

namespace ge {
DeviceInfo::DeviceInfo(int32_t node_id, int32_t device_id) noexcept
    : node_id_(node_id), device_id_(device_id) {
}

int32_t DeviceInfo::GetNodeId() const {
  return node_id_;
}

int32_t DeviceInfo::GetDeviceId() const {
  return device_id_;
}

int32_t DeviceInfo::GetFlowRankId() const {
  return flow_rank_id_;
}

void DeviceInfo::SetFlowRankId(int32_t flow_rank_id) {
  flow_rank_id_ = flow_rank_id;
}

const std::string &DeviceInfo::GetHostIp() const {
  return host_ip_;
}

void DeviceInfo::SetHostIp(const std::string &host_ip) {
  host_ip_ = host_ip;
}

int32_t DeviceInfo::GetDgwPort() const {
  return dgw_port_;
}

void DeviceInfo::SetDgwPort(int32_t dgw_port) {
  dgw_port_ = dgw_port;
}
}  // namespace ge