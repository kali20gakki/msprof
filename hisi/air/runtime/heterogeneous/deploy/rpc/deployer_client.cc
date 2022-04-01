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

#include "deploy/rpc/deployer_client.h"
#include "grpc++/grpc++.h"
#include "framework/common/debug/ge_log.h"
#include "debug/ge_util.h"
#include "common/plugin/ge_util.h"
#include "proto/deployer.grpc.pb.h"

namespace ge {
class DeployerClient::GrpcClient {
 public:
  /*
   *  @ingroup ge
   *  @brief   init deployer client
   *  @param   [in]  std::string &
   *  @return  SUCCESS or FAILED
   */
  Status Init(const std::string &address);

  Status SendRequest(const deployer::DeployerRequest &request, deployer::DeployerResponse &response);

 private:
  std::unique_ptr<deployer::DeployerService::Stub> stub_;
};

Status DeployerClient::GrpcClient::Init(const std::string &address) {
  GELOGI("Start to create channel, address=%s", address.c_str());
  grpc::ChannelArguments channel_arguments;
  channel_arguments.SetMaxReceiveMessageSize(INT32_MAX);
  channel_arguments.SetMaxSendMessageSize(INT32_MAX);
  auto channel = grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), channel_arguments);
  if (channel == nullptr) {
    GELOGE(FAILED, "[Create][Channel]Failed to create channel, address = %s", address.c_str());
    return FAILED;
  }

  stub_ = deployer::DeployerService::NewStub(channel);
  GE_CHECK_NOTNULL(stub_);
  return SUCCESS;
}

Status DeployerClient::GrpcClient::SendRequest(const deployer::DeployerRequest &request,
                                               deployer::DeployerResponse &response) {
  ::grpc::ClientContext context;
  ::grpc::Status status = stub_->DeployerProcess(&context, request, &response);
  if (!status.ok()) {
    GELOGE(FAILED, "RPC failed, gRPC error code =%d, error message=%s", status.error_code(),
           status.error_message().c_str());
    return FAILED;
  }
  return SUCCESS;
}

DeployerClient::DeployerClient() = default;
DeployerClient::~DeployerClient() = default;

Status DeployerClient::Initialize(const std::string &address) {
  grpc_client_ = MakeUnique<DeployerClient::GrpcClient>();
  GE_CHECK_NOTNULL(grpc_client_);
  if (grpc_client_->Init(address) != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Init grpc client failed, address:%s", address.c_str());
    GELOGE(FAILED, "[Init][Client]Init grpc client failed, address:%s", address.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status DeployerClient::SendRequest(const deployer::DeployerRequest &request, deployer::DeployerResponse &response) {
  return grpc_client_->SendRequest(request, response);
}
}  // namespace ge