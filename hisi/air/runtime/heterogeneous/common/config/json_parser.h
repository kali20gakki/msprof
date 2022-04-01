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
#ifndef RUNTIME_COMMON_JSON_PARSER_H_
#define RUNTIME_COMMON_JSON_PARSER_H_

#include <string>
#include <map>
#include "mmpa_api.h"
#include "nlohmann/json.hpp"
#include "ge/ge_api_error_codes.h"
#include "common/config/configurations.h"

namespace ge {

class JsonParser {
 public:
  /*
   *  @ingroup ge
   *  @brief   parse host json file
   *  @param   [in]  std::string &
   *  @param   [in]  DeployerConfig &
   *  @return  SUCCESS or FAILED
   */
  static Status ParseHostInfoFromConfigFile(std::string &file_directory, DeployerConfig &deployer_config);

  /*
   *  @ingroup ge
   *  @brief   parse device json file
   *  @param   [in]  std::string &
   *  @param   [in]  NodeConfig &
   *  @return  SUCCESS or FAILED
   */
  static Status ParseDeviceConfigFromConfigFile(std::string &file_directory, NodeConfig &node_config);

 private:

  /*
   *  @ingroup ge
   *  @brief   check file path
   *  @param   [in]  std::string &
   *  @return  SUCCESS or FAILED
   */
  static bool CheckFilePath(const std::string &file_path);

  /*
   *  @ingroup ge
   *  @brief   read config file
   *  @param   [in]  std::string &
   *  @param   [in]  nlohmann::json &
   *  @return  SUCCESS or FAILED
   */
  static Status ReadConfigFile(const std::string &file_path, nlohmann::json &js);
};
}  // namespace ge
#endif  // RUNTIME_COMMON_JSON_PARSER_H_