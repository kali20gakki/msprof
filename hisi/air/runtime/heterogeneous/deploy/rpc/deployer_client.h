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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RPC_DEPLOY_SERVICE_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RPC_DEPLOY_SERVICE_H_

#include <string>
#include <memory>
#include "common/plugin/ge_util.h"
#include "proto/deployer.pb.h"
#include "ge/ge_api_error_codes.h"

namespace ge {
class DeployerClient {
 public:
  explicit DeployerClient();
  virtual ~DeployerClient();
  GE_DELETE_ASSIGN_AND_COPY(DeployerClient);

  Status Initialize(const std::string &address);

  Status SendRequest(const deployer::DeployerRequest &request, deployer::DeployerResponse &response);

 private:
  class GrpcClient;
  std::unique_ptr<GrpcClient> grpc_client_;
};
}  // namespace ge
#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_RPC_DEPLOY_SERVICE_H_