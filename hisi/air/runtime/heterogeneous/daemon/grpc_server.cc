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

#include "daemon/grpc_server.h"
#include "common/plugin/ge_util.h"
#include "framework/common/debug/log.h"
#include "daemon/daemon_service.h"
#include "common/config/configurations.h"
#include "grpc++/grpc++.h"
#include "proto/deployer.grpc.pb.h"

namespace ge {
namespace {
class DeployerGrpcService final : public deployer::DeployerService::Service {
 public:
  Status Initialize() {
    return daemon_service_.Initialize();
  }

  void Finalize() {
    daemon_service_.Finalize();
  }
  ::grpc::Status DeployerProcess(::grpc::ServerContext *context, const deployer::DeployerRequest *request,
                                 deployer::DeployerResponse *response) override {
    if (response == nullptr) {
      REPORT_INNER_ERROR("E19999", "Response param is null.");
      GELOGE(FAILED, "[Check][Response] Response param is null.");
      return ::grpc::Status::CANCELLED;
    }

    if ((context == nullptr) || (request == nullptr)) {
      REPORT_INNER_ERROR("E19999", "Input params is null.");
      GELOGE(FAILED, "[Check][Params] Input params is null.");
      response->set_error_code(FAILED);
      response->set_error_message("Input params is null");
      return ::grpc::Status::OK;
    }

    (void) daemon_service_.Process(context->peer(), *request, *response);
    return ::grpc::Status::OK;
  }

 private:
  DaemonService daemon_service_;
};
}  // namespace


class GrpcServer::Impl {
 public:
  Status Run() {
    GE_CHK_STATUS_RET_NOLOG(service_.Initialize());
    const auto &device_info = Configurations::GetInstance().GetDeviceInfo();
    std::string port = std::to_string(device_info.port);
    std::string ip = device_info.ipaddr;
    std::string server_addr = ip + ":" + port;
    grpc::ServerBuilder server_builder;
    server_builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    server_builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    server_builder.RegisterService(&service_);
    server_builder.SetMaxReceiveMessageSize(INT32_MAX);
    server_builder.SetMaxSendMessageSize(INT32_MAX);
    server_ = server_builder.BuildAndStart();
    if (server_ == nullptr) {
      REPORT_INNER_ERROR("E19999", "Failed to build and start gRPC service, address = %s is invalid or reused",
                         server_addr.c_str());
      GELOGE(FAILED, "[Build][Server] Failed to build and start gRPC service, address = %s is invalid or reused",
             server_addr.c_str());
      return FAILED;
    }
    GELOGI("Server listening on %s.", server_addr.c_str());
    server_->Wait();
    return SUCCESS;
  }

  void Finalize() {
    if (server_ != nullptr) {
      server_->Shutdown();
    }
    service_.Finalize();
  }

 private:
  friend class GrpcServer;
  DeployerGrpcService service_;
  std::unique_ptr<grpc::Server> server_;
};

GrpcServer::GrpcServer() = default;
GrpcServer::~GrpcServer() = default;

Status GrpcServer::Run() {
  impl_ = MakeUnique<GrpcServer::Impl>();
  GE_CHECK_NOTNULL(impl_);
  return impl_->Run();
}

void GrpcServer::Finalize() {
  if (impl_ != nullptr) {
    impl_->Finalize();
  }
}
} // namespace ge
