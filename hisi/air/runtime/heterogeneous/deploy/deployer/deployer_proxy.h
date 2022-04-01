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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_PROXY_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_PROXY_H_

#include <memory>
#include "proto/deployer.pb.h"
#include "common/config/configurations.h"
#include "deploy/resource/node_info.h"
#include "deploy/deployer/deployer.h"

namespace ge {
class DeployerProxy {
 public:
  static DeployerProxy &GetInstance() {
    static DeployerProxy instance;
    return instance;
  }

  Status Initialize(const std::vector<NodeConfig> &node_config_list);
  void Finalize();
  int32_t NumNodes() const;

  const NodeInfo *GetNodeInfo(int32_t node_id) const;

  Status SendRequest(int32_t node_id, deployer::DeployerRequest &request, deployer::DeployerResponse &response);

 private:
  DeployerProxy() = default;
  ~DeployerProxy();
  static std::unique_ptr<Deployer> CreateDeployer(const NodeConfig &node_config);

  std::vector<std::unique_ptr<Deployer>> deployers_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_PROXY_H_
