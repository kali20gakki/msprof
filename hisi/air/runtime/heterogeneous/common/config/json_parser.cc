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
#include "json_parser.h"
#include <string>
#include <fstream>
#include <exception>
#include <algorithm>
#include "framework/common/debug/ge_log.h"

namespace ge {
namespace {
const char *const kConfigFileName = "/resource.json";
}  // namespace

bool JsonParser::CheckFilePath(const std::string &file_path) {
  char trusted_path[MMPA_MAX_PATH] = {'\0'};
  int32_t ret = mmRealPath(file_path.c_str(), trusted_path, sizeof(trusted_path));
  if (ret != EN_OK) {
    GELOGE(FAILED, "[Check][Path]The file path %s is not like a realpath,mmRealPath return %d, errcode is %d",
           file_path.c_str(), ret, mmGetErrorCode());
    REPORT_CALL_ERROR("E19999", "The file path %s is not like a realpath,mmRealPath return %d, errcode is %d",
                      file_path.c_str(), ret, mmGetErrorCode());
    return false;
  }

  mmStat_t stat = {};
  ret = mmStatGet(trusted_path, &stat);
  if (ret != EN_OK) {
    GELOGE(FAILED, "[Check][File]Cannot get config file status,which path is %s, maybe not exist, return %d, errcode %d",
           trusted_path, ret, mmGetErrorCode());
    REPORT_CALL_ERROR("E19999", "Cannot get config file status,which path is %s, maybe not exist, return %d, errcode %d",
                      trusted_path, ret, mmGetErrorCode());
    return false;
  }
  if ((stat.st_mode & S_IFMT) != S_IFREG) {
    GELOGE(FAILED, "[Check][File]Config file is not a common file,which path is %s, mode is %u", trusted_path,
           stat.st_mode);
    REPORT_CALL_ERROR("E19999", "Config file is not a common file,which path is %s, mode is %u",
                      trusted_path, stat.st_mode);
    return false;
  }
  return true;
}

Status JsonParser::ReadConfigFile(const std::string &file_path, nlohmann::json &js) {
  if (file_path.empty()) {
    GELOGE(FAILED, "[Check][Path]File path is nullptr, fail to parser json file.");
    REPORT_INNER_ERROR("E19999", "File path is nullptr, fail to parser json file.");
    return FAILED;
  }

  if (!CheckFilePath(file_path)) {
    GELOGE(FAILED, "[Check][Path]Invalid config file path[%s]", file_path.c_str());
    REPORT_CALL_ERROR("E19999", "Invalid config file path[%s]", file_path.c_str());
    return FAILED;
  }

  std::ifstream fin(file_path);
  if (!fin.is_open()) {
    GELOGE(FAILED, "[Read][File]Read file %s failed", file_path.c_str());
    REPORT_CALL_ERROR("E19999", "Read file %s failed", file_path.c_str());
    return FAILED;
  }

  try {
    fin >> js;
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Check][File]Invalid json file,exception:%s", e.what());
    REPORT_CALL_ERROR("E19999", "Invalid json file,exception:%s", e.what());
    return FAILED;
  }
  GELOGI("Parse json from file [%s] successfully", file_path.c_str());
  return SUCCESS;
}

Status JsonParser::ParseHostInfoFromConfigFile(std::string &file_directory, DeployerConfig &deployer_config) {
  HostInfo host_info;
  std::vector<NodeConfig> device_config_list;
  if (file_directory.empty()) {
    GELOGE(FAILED, "[Check][Directory]Host directory is null.");
    REPORT_INNER_ERROR("E19999", "Directory is null.");
    return FAILED;
  }
  std::string file_path = file_directory.append(kConfigFileName);
  GELOGI("Get path[%s]successfully", file_path.c_str());

  nlohmann::json json_host;
  Status ret = ReadConfigFile(file_path, json_host);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Read][File]Read host config file:%s failed", file_path.c_str());
    REPORT_CALL_ERROR("E19999", "Read host config file:%s failed", file_path.c_str());
    return FAILED;
  }
  try {
    nlohmann::json js_host_info = json_host.at("host").get<nlohmann::json>();
    nlohmann::json js_ctrl_panel = js_host_info.at("ctrlPanel").get<nlohmann::json>();
    host_info.ctrl_panel.ipaddr = js_ctrl_panel.at("ipaddr").get<std::string>();
    auto ctrl_port_arry = js_ctrl_panel["availPorts"];
    host_info.ctrl_panel.available_ports = ctrl_port_arry[0];

    nlohmann::json js_data_panel = js_host_info.at("dataPanel").get<nlohmann::json>();
    host_info.data_panel.ipaddr = js_data_panel.at("ipaddr").get<std::string>();
    auto data_port_arry = js_data_panel["availPorts"];
    host_info.data_panel.available_ports = data_port_arry[0];

    deployer_config.mode = json_host.at("mode").get<std::string>();
    nlohmann::json json_dev_list = json_host.at("devList").get<nlohmann::json>();
    for (const auto &dev_json : json_dev_list) {
      NodeConfig device_config;
      device_config.device_id = dev_json.at("deviceId").get<int32_t>();
      device_config.ipaddr = dev_json.at("ipaddr").get<std::string>();
      device_config.ca_file = dev_json.at("caFile").get<std::string>();
      device_config.cert_file = dev_json.at("certFile").get<std::string>();
      device_config.key_file = dev_json.at("keyFile").get<std::string>();
      device_config.port = dev_json.at("port").get<int32_t>();
      device_config.token = dev_json.at("token").get<std::string>();
      device_config_list.emplace_back(std::move(device_config));
    }
    GELOGI("Finish parsing the configuration file");
  } catch (std::exception &e) {
    GELOGE(FAILED, " [Check][Format]The format of the configuration file is wrong, %s", e.what());
    REPORT_CALL_ERROR("E19999", "The format of the configuration file is wrong,%s", e.what());
    return FAILED;
  }

  deployer_config.host_info = std::move(host_info);
  deployer_config.remote_node_config_list = std::move(device_config_list);
  return SUCCESS;
}

Status JsonParser::ParseDeviceConfigFromConfigFile(std::string &file_directory, NodeConfig &node_config) {
  if (file_directory.empty()) {
    GELOGE(FAILED, "[Check][Directory]Device directory is null.");
    REPORT_INNER_ERROR("E19999", "Device directory is null.");
    return FAILED;
  }
  std::string file_path = file_directory + kConfigFileName;
  GELOGI("Get path[%s]successfully", file_path.c_str());

  nlohmann::json js;
  Status ret = ReadConfigFile(file_path, js);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Read][File]Read device config file failed.");
    REPORT_CALL_ERROR("E19999", "Read device config file failed.");
    return FAILED;
  }
  try {
    node_config.ipaddr = js.at("ipaddr").get<std::string>();
    node_config.ca_file = js.at("caFile").get<std::string>();
    node_config.cert_file = js.at("certFile").get<std::string>();
    node_config.key_file = js.at("keyFile").get<std::string>();
    node_config.port = js.at("port").get<int32_t>();
    node_config.token = js.at("token").get<std::string>();
    node_config.device_id = js.at("deviceId").get<int32_t>();
    GELOGI("Finish parsing the configuration file");
  } catch (std::exception &e) {
    GELOGE(FAILED, "The format of the configuration file is wrong,%s", e.what());
    REPORT_CALL_ERROR("E19999", "The format of the configuration file is wrong%s", e.what());
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge