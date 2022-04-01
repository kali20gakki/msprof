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

#include "node_info.h"
#include "common/ge_inner_error_codes.h"
#include "common/debug/ge_log.h"

namespace ge {
const std::vector<int32_t> &NodeInfo::GetDgwPorts() const {
  return dgw_ports_;
}

void NodeInfo::SetDgwPorts(const std::vector<int32_t> &dgw_ports) {
  dgw_ports_ = dgw_ports;
}

int32_t NodeInfo::GetDeviceCount() const {
  return device_count_;
}

void NodeInfo::SetDeviceCount(int32_t device_count) {
  device_count_ = device_count;
}

int32_t NodeInfo::GetDeviceId() const {
  return device_id_;
}

void NodeInfo::SetDeviceId(int32_t device_id) {
  device_id_ = device_id;
}

const std::string &NodeInfo::GetHostIp() const {
  return host_ip_;
}

void NodeInfo::SetHostIp(const std::string &host_ip) {
  host_ip_ = host_ip;
}

Status NodeInfo::GetDgwPort(int32_t device_id, int32_t &port) const {
  if (device_id == -1) {
    device_id = 0;
  }
  auto index = static_cast<size_t>(device_id);
  if (index >= dgw_ports_.size()) {
    GELOGE(PARAM_INVALID, "device id out of range, device_id = %d", device_id);
    return PARAM_INVALID;
  }
  port = dgw_ports_[index];
  return SUCCESS;
}
}  // namespace ge