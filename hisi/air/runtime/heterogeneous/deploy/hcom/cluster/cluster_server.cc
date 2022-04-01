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

#include "deploy/hcom/cluster/cluster_server.h"
#include <sstream>
#include <vector>

#include "grpc++/grpc++.h"
#include "proto/cluster.grpc.pb.h"
#include "common/plugin/ge_util.h"
#include "common/util.h"
#include "framework/common/debug/ge_log.h"
#include "deploy/hcom/cluster/cluster_service_impl.h"

namespace ge {
class ClusterService : public cluster::ClusterService::Service {
 public:
  grpc::Status RegisterMemberToChief(::grpc::ServerContext *context,
                                     const ::cluster::MemberRequest *request,
                                     ::cluster::ClusterResponse *response) override {
    (void) context;
    ClusterServiceImpl::RegisterMemberToChief(request, response);
    return grpc::Status::OK;
  }

  grpc::Status QuerryAllNodes(::grpc::ServerContext *context,
                              const ::cluster::MemberRequest *request,
                              ::cluster::ClusterResponse *response) override {
    (void) context;
    ClusterServiceImpl::QueryAllNodes(request, response);
    return grpc::Status::OK;
  }

  grpc::Status RegisterFinished(::grpc::ServerContext *context,
                                const ::cluster::MemberRequest *request,
                                ::cluster::ClusterResponse *response) override {
    (void) context;
    ClusterServiceImpl::RegisterFinished(request, response);
    return grpc::Status::OK;
  }
};

class ClusterServer::Impl {
 public:
  Status StartClusterServer(const std::string &server_addr) {
    grpc::ServerBuilder builder;
    builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    builder.SetMaxReceiveMessageSize(INT32_MAX);
    builder.SetMaxSendMessageSize(INT32_MAX);
    server_ = builder.BuildAndStart();
    if (server_ == nullptr) {
      GELOGW("[Build][Server] Failed to build and start gRPC service, address = %s is invalid or reused",
             server_addr.c_str());
      return FAILED;
    }
    GELOGI("Server listening on %s.", server_addr.c_str());
    auto server_wait = [&]() { server_->Wait(); };
    grpc_server_thread_ = std::thread(server_wait);
    return SUCCESS;
  }

  Status StopClusterServer() {
    if (server_ == nullptr) {
      REPORT_INNER_ERROR("E19999", "Stop Cluster server，server dose not exist.");
      GELOGE(FAILED, "[Cluster][Server]Server dose not exist.");
      return FAILED;
    }
    server_->Shutdown();
    grpc_server_thread_.join();
    return SUCCESS;
  }

 private:
  friend class ClusterServer;
  ClusterService service_;
  std::unique_ptr<grpc::Server> server_;
  std::thread grpc_server_thread_;
};

ClusterServer::ClusterServer() = default;
ClusterServer::~ClusterServer() = default;

Status ClusterServer::StartClusterServer(const std::string &server_addr) {
  impl_ = MakeUnique<ClusterServer::Impl>();
  GE_CHECK_NOTNULL(impl_);
  return impl_->StartClusterServer(server_addr);
}

Status ClusterServer::StopClusterServer() {
  return impl_ == nullptr ? SUCCESS : impl_->StopClusterServer();
}

std::thread &ClusterServer::GetThread() {
  static std::thread default_thread;
  return impl_ == nullptr ? default_thread : impl_->grpc_server_thread_;
}
}  // namespace ge
