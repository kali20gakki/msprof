/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_RUNTIME_HETEROGENEOUS_FLOWRM_NETWORK_MANAGER_H_
#define AIR_RUNTIME_HETEROGENEOUS_FLOWRM_NETWORK_MANAGER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "framework/common/debug/log.h"
#include "ge/ge_api_error_codes.h"
#include "common/config/json_parser.h"
#include "inc/external/graph/types.h"

namespace ge {
enum class PortType {
  kDataGw = 1
};

class NetworkManager {
 public:
  static NetworkManager &GetInstance() {
    static NetworkManager instance;
    return instance;
  }

  /*
   *  @ingroup ge
   *  @brief   initialize all port
   *  @return: SUCCESS or FAILED
   */
  Status Initialize();

  /*
   *  @ingroup ge
   *  @brief   finalize all socket fd
   *  @param   [in]  None
   *  @return  SUCCESS or FAILED
   */
  Status Finalize();

  /*
   *  @ingroup ge
   *  @brief   get data panel ip
   *  @return: ip address
   */
  Status GetDataPanelIp(std::string& ip) const;

  /*
   *  @ingroup ge
   *  @brief   get port by type
   *  @return: port number
   */
  uint32_t GetDataPanelPort();

  /*
   *  @ingroup ge
   *  @brief   get ctrl panel ip
   *  @return: ctrl ip address
   */
  std::string GetCtrlPanelIp() const;

  /*
   *  @ingroup ge
   *  @brief   get ctrl panel ports
   *  @return: ctrl port range
   */
  std::string GetCtrlPanelPorts() const;

 private:
  NetworkManager() = default;
  ~NetworkManager() = default;

  /*
   *  @ingroup ge
   *  @brief   bind main port
   *  @return: SUCCESS or FAILED
   */
  Status BindMainPort();

  /*
   *  @ingroup ge
   *  @brief   try to bind port
   *  @param   [in]  uint32_t
   *  @return: SUCCESS or FAILED
   */
  Status TryToBindPort(uint32_t port);

  /*
   *  @ingroup ge
   *  @brief   get host json info
   *  @param   [in]  DeployerConfig &
   *  @return: SUCCESS or FAILED
   */
  Status GetHostInfo(DeployerConfig &hostInformation) const;

  uint32_t main_port_ = 0U;
  uint32_t datagw_port_ = 0U;
  int32_t socket_fd_ = -1;
};
}  // namespace ge
#endif //AIR_RUNTIME_HETEROGENEOUS_FLOWRM_NETWORK_MANAGER_H_
