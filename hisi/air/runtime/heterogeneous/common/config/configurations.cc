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
#include "common/config/configurations.h"
#include "common/config/json_parser.h"
#include "common/debug/log.h"

namespace ge {
Status Configurations::InitHostInformation(const std::string &config_path) {
  std::string path = config_path;
  GE_CHK_STATUS_RET_NOLOG(JsonParser::ParseHostInfoFromConfigFile(path, host_information_));
  std::sort(host_information_.remote_node_config_list.begin(), host_information_.remote_node_config_list.end(),
            [](const NodeConfig &lhs, const NodeConfig &rhs) -> bool {
              return (lhs.ipaddr < rhs.ipaddr) || ((lhs.ipaddr == rhs.ipaddr) && (lhs.port < rhs.port));
            });
  return SUCCESS;
}

Status Configurations::InitDeviceInformation(const std::string &config_path) {
  std::string path = config_path;
  return JsonParser::ParseDeviceConfigFromConfigFile(path, host_information_.node_config);
}
}  // namespace ge
