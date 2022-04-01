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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_SERVICE_H_
#define AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_SERVICE_H_

#include "ge/ge_api_types.h"
#include "proto/deployer.pb.h"
#include "daemon/daemon_client_manager.h"

namespace ge {
class DaemonService {
 public:
  Status Initialize();
  void Finalize();
  Status Process(const std::string &peer_uri,
                 const deployer::DeployerRequest &request,
                 deployer::DeployerResponse &response);


 private:
  void ProcessInitRequest(const std::string &peer_uri,
                          const deployer::DeployerRequest &request,
                          deployer::DeployerResponse &response);

  void ProcessDisconnectRequest(const std::string &peer_uri,
                                const deployer::DeployerRequest &request,
                                deployer::DeployerResponse &response);

  void ProcessHeartbeatRequest(const deployer::DeployerRequest &request,
                               deployer::DeployerResponse &response);

  void ProcessDeployRequest(const deployer::DeployerRequest &request,
                            deployer::DeployerResponse &response);

  bool GetClient(int64_t client_id, DeployerDaemonClient **client, deployer::DeployerResponse &response);

  DaemonClientManager client_manager_;
};
} // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_SERVICE_H_
