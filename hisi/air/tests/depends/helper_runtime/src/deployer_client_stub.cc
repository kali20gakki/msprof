/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#include <sstream>
#include "deploy/rpc/deployer_client.h"
#include "framework/common/debug/ge_log.h"
#include "debug/ge_util.h"
#include "common/plugin/ge_util.h"
#include "daemon/daemon_service.h"
#define private public
#include "exec_runtime/execution_runtime.h"
#undef private

namespace ge {
class DeployerClient::GrpcClient {
 public:
  DeployContext context_;
  DaemonService daemon_service_;
};

DeployerClient::DeployerClient() = default;
DeployerClient::~DeployerClient() = default;

Status DeployerClient::Initialize(const string &address) {
  grpc_client_ = MakeUnique<DeployerClient::GrpcClient>();
  GE_CHECK_NOTNULL(grpc_client_);
  return SUCCESS;
}

Status DeployerClient::SendRequest(const deployer::DeployerRequest &request, deployer::DeployerResponse &response) {
  auto original_handle = ExecutionRuntime::handle_;
  ExecutionRuntime::handle_ = nullptr;
  std::string peer_uri = "http://127.0.0.1:1234";
  auto ret = grpc_client_->daemon_service_.Process(peer_uri, request, response);
  ExecutionRuntime::handle_ = original_handle;
  return ret;
}
}  // namespace ge