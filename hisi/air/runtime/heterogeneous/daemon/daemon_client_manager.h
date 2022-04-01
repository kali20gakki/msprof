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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_CLIENT_MANAGER_H_
#define AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_CLIENT_MANAGER_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include "daemon/deployer_daemon_client.h"
#include "deploy/flowrm/helper_exchange_deployer.h"
#include "common/config/device_debug_config.h"
#include "ge/ge_api_error_codes.h"
#include "proto/deployer.pb.h"
#include "proto/var_manager.pb.h"

namespace ge {
class DaemonClientManager {
 public:
  DaemonClientManager() = default;
  ~DaemonClientManager();

  /*
   *  @ingroup ge
   *  @brief   init heartbeat thread
   *  @param   [in]  None
   *  @return  None
   */
  Status Initialize();

  /*
   *  @ingroup ge
   *  @brief   finalize client manager
   *  @param   [in]  None
   *  @return  None
   */
  void Finalize();

  /*
   *  @ingroup ge
   *  @brief   create client
   *  @param   [in]  int64_t &
   *  @return  SUCCESS or FAILED
   */
  Status CreateClient(int64_t &client_id);

  /*
   *  @ingroup ge
   *  @brief   close client
   *  @param   [in]  int64_t
   *  @return  SUCCESS or FAILED
   */
  Status CloseClient(int64_t client_id);

  /*
   *  @ingroup ge
   *  @brief   get client by client_id
   *  @param   [in]  uint32_t
   *  @return  model
   */
  DeployerDaemonClient *GetClient(int64_t client_id);

  void RecordClientInfo(const std::string &peer_uri);

  void DeleteClientInfo(const std::string &peer_uri);

 private:
  struct ClientAddr {
    std::string ip;
    std::string port;
  };

  struct ClientAddrManager {
    std::string connect_status;
    std::map<std::string, ClientAddr> client_addrs;
  };

  static void GetClientIpAndPort(const std::string &uri, std::string &key, ClientAddr &client);
  void UpdateJsonFile();
  void ConfigClientAddrToJson(nlohmann::json &j);
  void DeleteAllClientInfo();
  void EvictExpiredClients();

  std::map<int64_t, std::unique_ptr<DeployerDaemonClient>> clients_;
  std::atomic_bool running_{};
  std::mutex mu_;
  std::mutex mu_cv_;
  std::condition_variable running_cv_;
  std::thread evict_thread_;
  int64_t client_id_gen_ = 0;
  ClientAddrManager client_addr_manager_;
  std::map<std::string, ClientAddr> client_addrs_;
};
} // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DAEMON_DAEMON_CLIENT_MANAGER_H_
