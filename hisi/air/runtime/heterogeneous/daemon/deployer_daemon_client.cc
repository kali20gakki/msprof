
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

#include "daemon/deployer_daemon_client.h"

namespace ge {
namespace {
constexpr int64_t kHeartbeatExpireSec = 60;
}  // namespace

DeployerDaemonClient::DeployerDaemonClient(int64_t client_id) : client_id_(client_id) {}

void DeployerDaemonClient::Initialize() {
  std::string ctx_name = std::string("client_") + std::to_string(client_id_);
  context_.SetName(ctx_name);
  context_.Initialize();
  OnHeartbeat();
}

void DeployerDaemonClient::Finalize() {
  context_.Finalize();
}

bool DeployerDaemonClient::IsExpired() {
  std::lock_guard<std::mutex> lk(mu_);
  int64_t diff =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_heartbeat_ts_).count();
  return diff > kHeartbeatExpireSec;
}

bool DeployerDaemonClient::IsExecuting() {
  std::lock_guard<std::mutex> lk(mu_);
  return is_executing_;
}

void DeployerDaemonClient::SetIsExecuting(bool is_executing) {
  std::lock_guard<std::mutex> lk(mu_);
  is_executing_ = is_executing;
}

void DeployerDaemonClient::OnHeartbeat() {
  std::lock_guard<std::mutex> lk(mu_);
  last_heartbeat_ts_ = std::chrono::steady_clock::now();
}

DeployContext &DeployerDaemonClient::GetContext() {
  return context_;
}
} // namespace ge