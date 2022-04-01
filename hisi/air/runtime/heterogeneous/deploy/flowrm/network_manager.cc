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

#include "deploy/flowrm/network_manager.h"

namespace ge {
namespace {
constexpr uint32_t kMaxPortNum = 128U;
constexpr uint32_t kMaxPort = 65535;
constexpr char_t const *kConfigFilePathEnv = "HELPER_RES_FILE_PATH";
}

Status NetworkManager::BindMainPort() {
  DeployerConfig host_information;
  GE_CHK_STATUS_RET_NOLOG(GetHostInfo(host_information));

  std::string ports_range = host_information.host_info.data_panel.available_ports;
  uint32_t start_port = std::stoi(ge::StringUtils::Split(ports_range, '~')[0]);
  uint32_t end_port = std::stoi(ge::StringUtils::Split(ports_range, '~')[1]);
  if ((end_port <= start_port) || (start_port > kMaxPort) || (end_port > kMaxPort)) {
    REPORT_INNER_ERROR("E19999", "[Invaild][Port] start_port[%d] is larger than end_port[%d].", start_port, end_port);
    GELOGE(FAILED, "[Invaild][Port] start_port[%d] is larger than end_port[%d].", start_port, end_port);
    return FAILED;
  }

  uint32_t num_segments = (end_port - start_port) / kMaxPortNum;
  uint32_t remainder = (end_port - start_port) % kMaxPortNum;
  if (remainder != 0U) {
    num_segments = num_segments + 1;
  }

  for (uint32_t i = 0U; i < num_segments; i++) {
    int32_t main_port = start_port + i * kMaxPortNum;
    Status res = TryToBindPort(main_port);
    if (res == SUCCESS) {
      main_port_ = main_port;
      GELOGD("Bind main port[%d] success.", main_port_);
      return SUCCESS;
    } else {
      GELOGW("Can not bind main port[%d], it may bind by other process, continue.", main_port);
      continue;
    }
  }

  REPORT_INNER_ERROR("E19999", "[Bind][Port]All main port can not be bind, all prots in data panel can not be used.");
  GELOGE(FAILED, "[Bind][Port]All main port can not be bind, all prots in data panel can not be used.");
  return FAILED;
}

uint32_t NetworkManager::GetDataPanelPort() {
  return main_port_ + static_cast<uint32_t>(PortType::kDataGw);
}

Status NetworkManager::TryToBindPort(uint32_t port) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  int32_t res = bind(socket_fd_, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  if (res == -1) {
    GELOGE(FAILED, "[Bind][Port]Bind port[%d] failed.", port);
    REPORT_CALL_ERROR("E19999", "Bind port[%d] failed.", port);
    return FAILED;
  }
  return SUCCESS;
}

Status NetworkManager::GetHostInfo(DeployerConfig &host_info) const {
  host_info = Configurations::GetInstance().GetHostInformation();
  return SUCCESS;
}

Status NetworkManager::GetDataPanelIp(std::string& host_ip) const {
  DeployerConfig host_information;
  GE_CHK_STATUS_RET_NOLOG(GetHostInfo(host_information));
  host_ip = host_information.host_info.data_panel.ipaddr;
  return SUCCESS;
}

std::string NetworkManager::GetCtrlPanelIp() const {
  return Configurations::GetInstance().GetHostInformation().host_info.ctrl_panel.ipaddr;
}

std::string NetworkManager::GetCtrlPanelPorts() const {
  return Configurations::GetInstance().GetHostInformation().host_info.ctrl_panel.available_ports;
}

Status NetworkManager::Initialize() {
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == -1) {
    GELOGE(FAILED, "[Get][Fd] Get socket fd error.");
    return FAILED;
  }
  GE_CHK_STATUS_RET(BindMainPort());
  GELOGD("[Bind][Port] Bind main port success, port = %d", main_port_);
  return SUCCESS;
}

Status NetworkManager::Finalize() {
  if (socket_fd_ == -1) {
    return SUCCESS;
  }

  int32_t res = close(socket_fd_);
  socket_fd_ = -1;
  if (res == -1) {
    GELOGE(FAILED, "[Close][Socket] Close socket failed.");
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge
