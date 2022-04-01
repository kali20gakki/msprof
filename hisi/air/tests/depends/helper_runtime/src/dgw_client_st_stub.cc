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

#include "queue_schedule/dgw_client.h"
#include "deploy/flowrm/queue_schedule_manager.h"

namespace bqs {
DgwClient::DgwClient(const uint32_t deviceId) {

}

std::shared_ptr<DgwClient> DgwClient::GetInstance(const uint32_t deviceId) {
  std::shared_ptr<DgwClient> ptr = std::make_shared<DgwClient>(deviceId);
  return ptr;
}

int32_t DgwClient::Initialize(const uint32_t dgwPid, const std::string procSign) {
  return 0;
}

int32_t DgwClient::CreateHcomHandle(const std::string masterIp, const uint32_t masterPort, std::string localIp, const uint32_t localPort,
                                    const std::string remoteIp, const uint32_t remotePort, uint64_t &handle) {
  return 0;
}

int32_t DgwClient::CreateHcomTag(const uint64_t handle, const std::string tagName, int32_t &tag) {
  return 0;
}

int32_t DgwClient::BindQueRoute(const std::vector<QueueRoute> &qRoutes, std::vector<int32_t> &bindResults) {
  return 0;
}

int32_t DgwClient::UnbindQueRoute(const std::vector<QueueRoute> &qRoutes, std::vector<int32_t> &results) {
  return 0;
}

int32_t DgwClient::DestroyHcomTag(const uint64_t handle, const int32_t tag) {
  return 0;
}

int32_t DgwClient::DestroyHcomHandle(const uint64_t handle) {
  return 0;
}

int32_t DgwClient::UpdateConfig(ConfigInfo &cfgInfo, std::vector<int32_t> &cfgRets) const {
  return 0;
}
}

namespace ge {
Status QueueScheduleManager::StartQueueSchedule(uint32_t device_id,
                                                uint32_t vf_id,
                                                const std::string &group_name,
                                                pid_t &pid) {
  pid = 666;
  return 0;
}

Status QueueScheduleManager::Shutdown(pid_t pid) {
  return 0;
}
}