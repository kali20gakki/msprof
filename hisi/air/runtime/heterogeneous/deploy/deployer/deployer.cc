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

#include "deploy/deployer/deployer.h"
#include "common/debug/log.h"
#include "common/plugin/ge_util.h"
#include "deployer_service_impl.h"

namespace ge {
namespace {
constexpr int32_t kHeartbeatIntervalInSec = 5;
constexpr int32_t kDataGwPortBase = 16666;
}  // namespace

LocalDeployer::LocalDeployer() : local_context_(true) {
  node_info_.SetDeviceId(-1);
}

const NodeInfo &LocalDeployer::GetNodeInfo() const {
  return node_info_;
}

Status LocalDeployer::Initialize() {
  local_context_.SetName("local_context");
  local_context_.Initialize();
  GELOGD("Local device initialized successfully");
  return SUCCESS;
}

Status LocalDeployer::Finalize() {
  local_context_.Finalize();
  GELOGD("Local device finalized successfully");
  return SUCCESS;
}

Status LocalDeployer::Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) {
  return DeployerServiceImpl::GetInstance().Process(local_context_, request, response);
}

RemoteDeployer::RemoteDeployer(NodeConfig node_config) : node_config_(std::move(node_config)) {
  node_info_.SetHostIp(node_config_.ipaddr);
  node_info_.SetDeviceId(node_config_.device_id);
}

Status RemoteDeployer::Initialize() {
  std::string rpc_address = node_config_.ipaddr + ":" + std::to_string(node_config_.port);
  GELOGI("Initialize remote deploy client started, address = %s", rpc_address.c_str());
  client_ = CreateClient();
  GE_CHECK_NOTNULL(client_);
  GE_CHK_STATUS_RET(client_->Initialize(rpc_address), "Failed to create rpc client");
  GE_CHK_STATUS_RET(Connect(), "Failed to connect to remote deployer");
  keepalive_thread_ = std::thread([&]() {
    Keepalive();
  });
  GELOGI("Initialize remote deploy client successfully");
  return SUCCESS;
}

Status RemoteDeployer::Connect() {
  if (connected_) {
    return SUCCESS;
  }
  deployer::DeployerRequest request;
  request.set_type(deployer::kInitRequest);
  auto init_request = request.mutable_init_request();
  init_request->set_token(node_config_.token);
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(client_->SendRequest(request, response), "Failed to send init request");
  auto error_code = response.error_code();
  GE_CHK_STATUS_RET(error_code, "[Check][Response]Check response failed. error code =%u, error message=%s",
                    error_code, response.error_message().c_str());
  UpdateNodeInfo(response.init_response());
  client_id_ = response.init_response().client_id();
  connected_ = true;
  return SUCCESS;
}

Status RemoteDeployer::Disconnect() {
  deployer::DeployerRequest request;
  request.set_type(deployer::kDisconnect);
  request.set_client_id(client_id_);
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(client_->SendRequest(request, response),
                    "[Send][Request] Send disconnect request failed, response info:%s",
                    response.error_message().c_str());
  return SUCCESS;
}

void RemoteDeployer::Keepalive() {
  GELOGI("Keepalive task started");
  while (keep_alive_) {
    std::this_thread::sleep_for(std::chrono::seconds(kHeartbeatIntervalInSec));
    SendHeartbeat();
  }
  GELOGI("Keepalive task ended.");
}

Status RemoteDeployer::Finalize() {
  if (!keepalive_thread_.joinable()) {
    return SUCCESS;
  }
  GELOGI("Start to finalize remote deployer, remote address = %s, port = %d",
         node_config_.ipaddr.c_str(),
         node_config_.port);
  keep_alive_ = false;
  keepalive_thread_.join();
  GE_CHK_STATUS_RET_NOLOG(Disconnect());
  return SUCCESS;
}

void RemoteDeployer::UpdateNodeInfo(const deployer::InitResponse &init_response) {
  std::vector<int32_t> dgw_ports;
  for (int32_t i = 0; i < init_response.dev_count(); ++i) {
    dgw_ports.emplace_back(kDataGwPortBase + i);
  }
  node_info_.SetDgwPorts(dgw_ports);
  node_info_.SetDeviceCount(init_response.dev_count());
}

const NodeInfo &RemoteDeployer::GetNodeInfo() const {
  return node_info_;
}

std::unique_ptr<DeployerClient> RemoteDeployer::CreateClient() {
  return MakeUnique<DeployerClient>();
}

Status RemoteDeployer::Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) {
  request.set_client_id(client_id_);
  return client_->SendRequest(request, response);
}

void RemoteDeployer::SendHeartbeat() {
  deployer::DeployerRequest request;
  request.set_type(deployer::kHeartbeat);
  deployer::DeployerResponse response;
  if (Process(request, response) != SUCCESS) {
    GELOGW("Send heartbeat request failed, response info:%s", response.error_message().c_str());
  } else {
    GELOGI("Success to send heartbeat to client_id[%ld]", client_id_);
  }
}
}  // namespace ge