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

#include "daemon/daemon_client_manager.h"
#include <fstream>
#include "common/util/error_manager/error_manager.h"
#include "common/plugin/ge_util.h"
#include "common/util.h"

namespace ge {
namespace {
constexpr int64_t kHeartbeatIntervalSec = 60;
constexpr int32_t kIpIndex = 1;
constexpr int32_t kPortIndex = 2;
}  // namespace

Status DaemonClientManager::Initialize() {
  if (running_) {
    GELOGE(INTERNAL_ERROR, "Repeat initialize");
    return INTERNAL_ERROR;
  }
  running_ = true;
  evict_thread_ = std::thread([this]() {
    while (running_) {
      std::unique_lock<std::mutex> lk(mu_cv_);
      running_cv_.wait_for(lk, std::chrono::seconds(kHeartbeatIntervalSec));
      EvictExpiredClients();
    }
  });
  return SUCCESS;
}

DaemonClientManager::~DaemonClientManager() {
  Finalize();
}

void DaemonClientManager::Finalize() {
  running_ = false;
  running_cv_.notify_all();
  if (evict_thread_.joinable()) {
    evict_thread_.join();
  }

  DeleteAllClientInfo();
}

Status DaemonClientManager::CreateClient(int64_t &client_id) {
  std::lock_guard<std::mutex> lk(mu_);
  int64_t new_client_id = client_id_gen_;
  auto new_client = MakeUnique<DeployerDaemonClient>(new_client_id);
  GE_CHECK_NOTNULL(new_client);
  new_client->Initialize();
  clients_.emplace(new_client_id, std::move(new_client));
  ++client_id_gen_;
  client_id = new_client_id;
  GELOGD("Client added, id = %ld", client_id);
  return SUCCESS;
}

Status DaemonClientManager::CloseClient(const int64_t client_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = clients_.find(client_id);
  if (it == clients_.end()) {
    REPORT_INNER_ERROR("E19999", "Client[%ld] does not exist in client manager.", client_id);
    GELOGE(FAILED, "[Close][Client]Client[%ld] does not exist in client manager.", client_id);
    return FAILED;
  }

  it->second->Finalize();
  clients_.erase(it);
  return SUCCESS;
}

DeployerDaemonClient *DaemonClientManager::GetClient(const int64_t client_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = clients_.find(client_id);
  if (it == clients_.end()) {
    REPORT_INNER_ERROR("E19999", "Get client[%ld] failed.", client_id);
    GELOGE(FAILED, "[Get][Client]Get client[%ld] failed.", client_id);
    return nullptr;
  }
  return it->second.get();
}

void DaemonClientManager::GetClientIpAndPort(const std::string &uri,
                                             std::string &key,
                                             ClientAddr &client) {
  std::vector<std::string> address = StringUtils::Split(uri, ':');
  if (address.size() < kPortIndex || address[kIpIndex].empty() || address[kPortIndex].empty()) {
    return;
  }
  client.ip = address[kIpIndex];
  client.port = address[kPortIndex];
  key = address[kIpIndex] + address[kPortIndex];
}

void DaemonClientManager::RecordClientInfo(const std::string &peer_uri) {
  std::lock_guard<std::mutex> lk(mu_);
  ClientAddr client;
  std::string key;
  GetClientIpAndPort(peer_uri, key, client);
  auto it = client_addrs_.find(key);
  if (it != client_addrs_.end()) {
    return;
  }
  client_addrs_.emplace(key, client);
  UpdateJsonFile();
}

void DaemonClientManager::UpdateJsonFile() {
  client_addr_manager_.client_addrs = client_addrs_;
  nlohmann::json json;
  ConfigClientAddrToJson(json);
  std::ofstream file("client.json");
  file << json << std::endl;
}

void DaemonClientManager::ConfigClientAddrToJson(nlohmann::json &j) {
  for (auto &client : client_addr_manager_.client_addrs) {
    nlohmann::json client_addr = nlohmann::json{{"ip", client.second.ip}, {"port", client.second.port}};
    j["connections"].push_back(client_addr);
  }
}

void DaemonClientManager::DeleteClientInfo(const std::string &peer_uri) {
  std::lock_guard<std::mutex> lk(mu_);
  ClientAddr client;
  std::string key;
  GetClientIpAndPort(peer_uri, key, client);

  auto it = client_addrs_.find(key);
  if (it == client_addrs_.end()) {
    return;
  }
  client_addrs_.erase(key);
  UpdateJsonFile();
}

void DaemonClientManager::DeleteAllClientInfo() {
  GELOGI("DeleteAllClientInfo begin.");
  client_addrs_.clear();
  UpdateJsonFile();
}

void DaemonClientManager::EvictExpiredClients() {
  std::vector<int64_t> expired_clients;
  {
    std::lock_guard<std::mutex> lk(mu_);
    for (auto &it : clients_) {
      if (it.second->IsExpired()) {
        if (it.second->IsExecuting()) {
          GELOGD("Client[%ld] is still executing, check next time.", it.first);
        } else {
          GELOGD("Client[%ld] is not executing.", it.first);
          expired_clients.push_back(it.first);
        }
      }
    }
  }
  for (int64_t client_id : expired_clients) {
    GELOGD("Client[%ld] expired, close it.", client_id);
    (void) CloseClient(client_id);
  }
}
} // namespace ge