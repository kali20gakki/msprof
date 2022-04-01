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

#include "deploy/resource/resource_manager.h"
#include "deploy/deployer/deployer_proxy.h"
#include "common/debug/log.h"

namespace ge {
Status ResourceManager::Initialize() {
  auto num_nodes = DeployerProxy::GetInstance().NumNodes();
  int32_t flow_rank_id = 0;
  for (int32_t node_id = 0; node_id < num_nodes; ++node_id) {
    const auto *node_info = DeployerProxy::GetInstance().GetNodeInfo(node_id);
    GE_CHECK_NOTNULL(node_info);
    for (int32_t i = 0; i < node_info->GetDeviceCount(); ++i) {
      DeviceInfo device_info(node_id, node_info->GetDeviceId() + i);
      int32_t dgw_port = -1;
      GE_CHK_STATUS_RET_NOLOG(node_info->GetDgwPort(i, dgw_port));
      device_info.SetHostIp(node_info->GetHostIp());
      device_info.SetDgwPort(dgw_port);
      device_info.SetFlowRankId(flow_rank_id++);
      device_info_list_.push_back(device_info);
      if (node_info->GetDeviceId() >= 0) {
        npu_device_info_list_.emplace_back(std::move(device_info));
      }
    }
  }
  for (const auto &device_info : device_info_list_) {
    device_info_map_[device_info.GetNodeId()][device_info.GetDeviceId()] = &device_info;
  }
  return SUCCESS;
}

const std::vector<DeviceInfo> &ResourceManager::GetDeviceInfoList() const {
  return device_info_list_;
}

const std::vector<DeviceInfo> &ResourceManager::GetNpuDeviceInfoList() const {
  return npu_device_info_list_;
}

void ResourceManager::Finalize() {
  device_info_list_.clear();
  npu_device_info_list_.clear();
}

const DeviceInfo *ResourceManager::GetDeviceInfo(int32_t node_id, int32_t device_id) const {
  auto node_it = device_info_map_.find(node_id);
  if (node_it == device_info_map_.cend()) {
    GELOGE(PARAM_INVALID, "Invalid node id: %d", node_id);
    return nullptr;
  }

  auto dev_it = node_it->second.find(device_id);
  if (dev_it == node_it->second.cend()) {
    GELOGE(PARAM_INVALID, "Invalid device id: %d, node_id = %d", device_id, node_id);
    return nullptr;
  }
  return dev_it->second;
}
}  // namespace ge