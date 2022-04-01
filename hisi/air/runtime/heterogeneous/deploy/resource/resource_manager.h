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

#include "common/debug/log.h"
#include "deploy/deployer/deployer_proxy.h"
#include "common/config/configurations.h"

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_RESOURCE_MANAGER_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_RESOURCE_MANAGER_H_

namespace ge {
class ResourceManager {
 public:
  static ResourceManager &GetInstance() {
    static ResourceManager instance;
    return instance;
  }

  Status Initialize();

  void Finalize();

  const DeviceInfo *GetDeviceInfo(int32_t node_id, int32_t device_id) const;

  const std::vector<DeviceInfo> &GetDeviceInfoList() const;

  const std::vector<DeviceInfo> &GetNpuDeviceInfoList() const;

 private:
  std::vector<DeviceInfo> device_info_list_;
  std::vector<DeviceInfo> npu_device_info_list_;
  std::map<int32_t, std::map<int32_t, const DeviceInfo *>> device_info_map_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RESOURCE_RESOURCE_MANAGER_H_
