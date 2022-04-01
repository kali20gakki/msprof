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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DAEMON_DEPLOYER_DAEMON_CLIENT_H_
#define AIR_RUNTIME_HETEROGENEOUS_DAEMON_DEPLOYER_DAEMON_CLIENT_H_

#include <cstdint>
#include <map>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include "common/config/device_debug_config.h"
#include "deploy/execfwk/executor_manager.h"
#include "ge/ge_api_error_codes.h"
#include "proto/deployer.pb.h"
#include "proto/var_manager.pb.h"
#include "deploy/deployer/deploy_context.h"
#include "deploy/flowrm/helper_exchange_deployer.h"

namespace ge {
class DeployerDaemonClient {
 public:
  explicit DeployerDaemonClient(int64_t client_id);
  ~DeployerDaemonClient() = default;

  /*
   *  @ingroup ge
   *  @brief   record client
   *  @param   [in]  None
   *  @return  None
   */
  void Initialize();

  /*
   *  @ingroup ge
   *  @brief   finalize client
   *  @param   [in]  None
   *  @return  None
   */
  void Finalize();

  /*
   *  @ingroup ge
   *  @brief   check client is expired
   *  @param   [in]  None
   *  @return  Expired or not
   */
  bool IsExpired();

  /*
   *  @ingroup ge
   *  @brief   set client is executing
   *  @param   [in]  bool
   *  @return  None
   */
  void SetIsExecuting(bool is_executing);

  /*
   *  @ingroup ge
   *  @brief   check client is executing
   *  @param   [in]  None
   *  @return  None
   */
  bool IsExecuting();

  /*
   *  @ingroup ge
   *  @brief   record client last heartbeat time
   *  @param   [in]  None
   *  @return  None
   */
  void OnHeartbeat();

  DeployContext &GetContext();

 private:
  std::mutex mu_;
  std::chrono::steady_clock::time_point last_heartbeat_ts_;
  bool is_executing_ = false;
  int64_t client_id_;
  DeployContext context_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DAEMON_DEPLOYER_DAEMON_CLIENT_H_
