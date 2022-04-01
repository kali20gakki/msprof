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

#include "daemon/daemon_service.h"
#include <fstream>
#include "ge/ge_api_error_codes.h"
#include "common/util/error_manager/error_manager.h"
#include "common/util.h"
#include "common/config/configurations.h"
#include "daemon/daemon_client_manager.h"
#include "common/utils/rts_api_utils.h"
#include "deploy/deployer/deployer_service_impl.h"

namespace ge {
Status DaemonService::Process(const std::string &peer_uri,
                              const deployer::DeployerRequest &request,
                              deployer::DeployerResponse &response) {
  auto request_type = request.type();
  if (request_type == deployer::kInitRequest) {
    ProcessInitRequest(peer_uri, request, response);
  } else if (request_type == deployer::kDisconnect) {
    ProcessDisconnectRequest(peer_uri, request, response);
  } else if (request_type == deployer::kHeartbeat) {
    ProcessHeartbeatRequest(request, response);
  } else {
    ProcessDeployRequest(request, response);
  }
  return SUCCESS;
}

void DaemonService::ProcessInitRequest(const std::string &peer_uri,
                                       const deployer::DeployerRequest &request,
                                       deployer::DeployerResponse &response) {
  auto request_token = request.init_request().token();
  const auto &device_config = Configurations::GetInstance().GetDeviceInfo();
  if (request_token != device_config.token) {
    REPORT_INNER_ERROR("E19999", "Check token failed, request token = %s, device token = %s",
                       request_token.c_str(), device_config.token.c_str());
    GELOGE(FAILED, "[Check][Token] Check token failed.request token = %s, device token = %s",
           request_token.c_str(), device_config.token.c_str());
    response.set_error_code(FAILED);
    response.set_error_message("Check token failed.");
    return;
  }

  int64_t client_id = 0;
  auto res = client_manager_.CreateClient(client_id);
  if (res != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Create client failed.");
    GELOGE(FAILED, "[Create][Client] Create client failed.");
    response.set_error_code(FAILED);
    response.set_error_message("Create client id failed.");
    return;
  }

  int32_t dev_count = 1;
  res = RtsApiUtils::GetDeviceCount(dev_count);
  if (res != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get device count failed.");
    GELOGE(FAILED, "[Get][DeviceCount] failed.");
    response.set_error_code(FAILED);
    response.set_error_message("Get device count failed.");
    return;
  }
  GELOGI("Get device count[%d].", dev_count);

  client_manager_.RecordClientInfo(peer_uri);
  response.mutable_init_response()->set_client_id(client_id);
  response.mutable_init_response()->set_dev_count(dev_count);
}

void DaemonService::ProcessDisconnectRequest(const std::string &peer_uri,
                                             const deployer::DeployerRequest &request,
                                             deployer::DeployerResponse &response) {
  int64_t client_id = request.client_id();
  DeployerDaemonClient *client = nullptr;
  if (GetClient(client_id, &client, response)) {
    client_manager_.CloseClient(client_id);
    client_manager_.DeleteClientInfo(peer_uri);
  }
}

void DaemonService::ProcessHeartbeatRequest(const deployer::DeployerRequest &request,
                                            deployer::DeployerResponse &response) {
  int64_t client_id = request.client_id();
  DeployerDaemonClient *client = nullptr;
  if (GetClient(client_id, &client, response)) {
    client->SetIsExecuting(true);
    client->OnHeartbeat();
    client->SetIsExecuting(false);

    if (client->IsExpired()) {
      GELOGI("Client[%ld] was expired during execution.", client_id);
      client_manager_.CloseClient(client_id);
      response.set_error_code(FAILED);
      response.set_error_message("Time expired");
      return;
    }
    GELOGI("Client[%ld] heartbeat dose not expired.", client_id);
  }
}

bool DaemonService::GetClient(int64_t client_id, DeployerDaemonClient **client, deployer::DeployerResponse &response) {
  *client = client_manager_.GetClient(client_id);
  if (*client != nullptr) {
    return true;
  }

  REPORT_CALL_ERROR("E19999", "Get client[%ld] failed.", client_id);
  GELOGE(FAILED, "[Get][Client] Get client[%ld] failed.", client_id);
  response.set_error_code(FAILED);
  response.set_error_message("Not exist client id");
  return false;
}

void DaemonService::ProcessDeployRequest(const deployer::DeployerRequest &request,
                                         deployer::DeployerResponse &response) {
  int64_t client_id = request.client_id();
  DeployerDaemonClient *client = nullptr;
  if (GetClient(client_id, &client, response)) {
    (void) DeployerServiceImpl::GetInstance().Process(client->GetContext(), request, response);
  }
}

Status DaemonService::Initialize() {
  GE_CHK_STATUS_RET(client_manager_.Initialize(), "Failed to initialize ClientManager");
  GELOGI("DaemonService initialized successfully");
  return SUCCESS;
}

void DaemonService::Finalize() {
  client_manager_.Finalize();
  GELOGI("DaemonService finalized successfully");
}
} // namespace ge
