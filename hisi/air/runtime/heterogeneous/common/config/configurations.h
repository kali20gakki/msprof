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

#ifndef AIR_RUNTIME_COMMON_CONFIG_CONFIGURATIONS_H_
#define AIR_RUNTIME_COMMON_CONFIG_CONFIGURATIONS_H_

#include <cstdint>
#include <string>
#include "ge/ge_api_types.h"

namespace ge {
struct NodeConfig {
  int32_t device_id;
  std::string ipaddr;
  int32_t port;
  std::string token;
  std::string ca_file;
  std::string cert_file;
  std::string key_file;
  NodeConfig()
      : device_id(0),
        ipaddr(),
        port(0),
        token(),
        ca_file(),
        cert_file(),
        key_file() {
  }
};

struct CtrlPanelInfo {
  std::string ipaddr;
  std::string available_ports;
};

struct DataPanelInfo {
  std::string ipaddr;
  std::string available_ports;
};

struct HostInfo {
  CtrlPanelInfo ctrl_panel;
  DataPanelInfo data_panel;
};

struct DeployerConfig {
  HostInfo host_info;
  std::string mode;
  NodeConfig node_config;
  std::vector<NodeConfig> remote_node_config_list;
};

class Configurations {
 public:
  static Configurations &GetInstance() {
    static Configurations instance;
    return instance;
  }

  Status InitHostInformation(const std::string &config_path);

  Status InitDeviceInformation(const std::string &config_path);

  const DeployerConfig& GetHostInformation() const {
    return host_information_;
  }

  const NodeConfig& GetDeviceInfo() const {
    return host_information_.node_config;
  }

  const std::vector<NodeConfig> &GetRemoteNodeConfigList() const {
    return host_information_.remote_node_config_list;
  }

 private:
  DeployerConfig host_information_;

};
}  // namespace ge

#endif  // AIR_RUNTIME_COMMON_CONFIG_CONFIGURATIONS_H_
