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
#include "common/util.h"
#include "deploy/deployer/deployer_proxy.h"
#include "common/plugin/ge_util.h"

namespace ge {
DeployerProxy::~DeployerProxy() {
  Finalize();
}

Status DeployerProxy::Initialize(const std::vector<NodeConfig> &node_config_list) {
  for (size_t i = 0U; i < node_config_list.size(); ++i) {
    const auto &node_config = node_config_list[i];
    auto deployer = CreateDeployer(node_config);
    GE_CHECK_NOTNULL(deployer);
    GE_CHK_STATUS_RET(deployer->Initialize(), "Failed to initialize deployer, id = %zu", i);
    GELOGI("Deployer initialized successfully, node_id = %zu", i);
    deployers_.emplace_back(std::move(deployer));
  }
  return SUCCESS;
}

void DeployerProxy::Finalize() {
  if (deployers_.empty()) {
    return;
  }
  for (auto &deployer : deployers_) {
    (void) deployer->Finalize();
  }
  deployers_.clear();
  GELOGI("Deployer proxy finalized");
}

std::unique_ptr<Deployer> DeployerProxy::CreateDeployer(const NodeConfig &node_config) {
  if (node_config.device_id >= 0) {
    return MakeUnique<RemoteDeployer>(node_config);
  } else {
    return MakeUnique<LocalDeployer>();
  }
}

Status DeployerProxy::SendRequest(int32_t node_id,
                                  deployer::DeployerRequest &request,
                                  deployer::DeployerResponse &response) {
  if (static_cast<size_t>(node_id) >= deployers_.size()) {
    GELOGE(PARAM_INVALID,
           "device id out of range, node id = %d, num_deployers = %zu",
           node_id,
           deployers_.size());
    return PARAM_INVALID;
  }

  auto &deployer = deployers_[node_id];
  GE_CHK_STATUS_RET(deployer->Process(request, response), "Failed to process request, node_id = %d, request = %s",
                    node_id, request.DebugString().c_str());
  return SUCCESS;
}

int32_t DeployerProxy::NumNodes() const {
  return static_cast<int32_t>(deployers_.size());
}

const NodeInfo *DeployerProxy::GetNodeInfo(int32_t node_id) const {
  if (static_cast<size_t>(node_id) >= deployers_.size()) {
    GELOGE(PARAM_INVALID,
           "device id out of range, node id = %d, num_deployers = %zu",
           node_id,
           deployers_.size());
    return nullptr;
  }
  return &deployers_[node_id]->GetNodeInfo();
}
}  // namespace ge
