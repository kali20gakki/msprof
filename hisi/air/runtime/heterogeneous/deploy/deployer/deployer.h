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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_H_

#include <string>
#include <memory>
#include <thread>
#include "proto/deployer.pb.h"
#include "ge/ge_api_error_codes.h"
#include "common/config/configurations.h"
#include "deploy/deployer/deploy_context.h"
#include "deploy/resource/node_info.h"
#include "deploy/resource/device_info.h"
#include "deploy/rpc/deployer_client.h"

namespace ge {
class Deployer {
 public:
  Deployer() = default;
  virtual ~Deployer() = default;
  virtual const NodeInfo &GetNodeInfo() const = 0;
  virtual Status Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) = 0;

  virtual Status Initialize() = 0;
  virtual Status Finalize() = 0;
};

class LocalDeployer : public Deployer {
 public:
  LocalDeployer();
  ~LocalDeployer() override = default;
  Status Initialize() override;
  const NodeInfo &GetNodeInfo() const override;
  Status Finalize() override;

  Status Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) override;
 private:
  NodeInfo node_info_;
  DeployContext local_context_;
};

class RemoteDeployer : public Deployer {
 public:
  explicit RemoteDeployer(NodeConfig node_config);
  Status Initialize() override;
  Status Finalize() override;
  Status Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) override;

  const NodeInfo &GetNodeInfo() const override;

 protected:
  virtual std::unique_ptr<DeployerClient> CreateClient();

 private:
  Status Connect();
  Status Disconnect();
  void SendHeartbeat();

  void Keepalive();
  void UpdateNodeInfo(const ::deployer::InitResponse &init_response);

  NodeConfig node_config_;
  NodeInfo node_info_;
  std::vector<DeviceInfo> device_list_;
  std::unique_ptr<DeployerClient> client_;
  std::thread keepalive_thread_;
  std::atomic_bool keep_alive_{true};
  std::atomic_bool connected_{false};
  int64_t client_id_ = 0;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOYER_H_
