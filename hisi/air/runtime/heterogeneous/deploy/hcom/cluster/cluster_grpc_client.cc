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
#include "deploy/hcom/cluster/cluster_grpc_client.h"

#include "common/plugin/ge_util.h"
#include "common/util.h"
#include "framework/common/debug/ge_log.h"
#include "grpc++/grpc++.h"
#include "proto/cluster.grpc.pb.h"

namespace ge {
namespace {
Status CheckStatus(const ::grpc::Status &status) {
  if (status.ok()) {
    return SUCCESS;
  }
  auto error_code = status.error_code();
  const auto &error_msg = status.error_message();
  REPORT_INNER_ERROR("E19999", "RPC failed, gRPC error code =%d, error message=%s.", error_code, error_msg.c_str());
  GELOGE(FAILED, "[Cluster][Rpc] gRPC error code =%d, error message=%s.", error_code, error_msg.c_str());
  return FAILED;
}
}  // namespace


class ClusterGrpcClient::Impl {
 public:
  Status CreateClient(const std::string &server_addr) {
    GELOGI("Start to create cluster channel, server_addr=%s", server_addr.c_str());
    grpc::ChannelArguments channel_arguments;
    channel_arguments.SetMaxReceiveMessageSize(INT32_MAX);
    channel_arguments.SetMaxSendMessageSize(INT32_MAX);
    auto channel = grpc::CreateCustomChannel(server_addr, grpc::InsecureChannelCredentials(), channel_arguments);
    if (channel == nullptr) {
      GELOGE(FAILED, "[Create][Channel]Failed to create channel, address = %s", server_addr.c_str());
      return FAILED;
    }

    stub_ = cluster::ClusterService::NewStub(channel);
    GE_CHECK_NOTNULL(stub_);
    return SUCCESS;
  }
 private:
  friend class ClusterGrpcClient;
  std::unique_ptr<cluster::ClusterService::Stub> stub_;
};

ClusterGrpcClient::ClusterGrpcClient() = default;
ClusterGrpcClient::~ClusterGrpcClient() = default;

Status ClusterGrpcClient::Initialize(const std::string &server_addr) {
  impl_ = MakeUnique<Impl>();
  return impl_->CreateClient(server_addr);
}

Status ClusterGrpcClient::RegisterMemberToChief(const cluster::MemberRequest &request,
                                                cluster::ClusterResponse &response) {
  grpc::ClientContext context;
  return CheckStatus(impl_->stub_->RegisterMemberToChief(&context, request, &response));
}

Status ClusterGrpcClient::QueryAllNodes(const cluster::MemberRequest &request, cluster::ClusterResponse &response) {
  grpc::ClientContext context;
  return CheckStatus(impl_->stub_->QuerryAllNodes(&context, request, &response));
}

Status ClusterGrpcClient::RegisterFinished(const cluster::MemberRequest &request, cluster::ClusterResponse &response) {
  grpc::ClientContext context;
  return CheckStatus(impl_->stub_->RegisterFinished(&context, request, &response));
}
}  // namespace ge