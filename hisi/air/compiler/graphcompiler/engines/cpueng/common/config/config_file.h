/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef AICPU_CONFIG_FILE_H_
#define AICPU_CONFIG_FILE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "error_code/error_code.h"

namespace aicpu {
class ConfigFile {
 public:
  /**
   * Get instance
   * @return ConfigFile reference
   */
  static ConfigFile &GetInstance();

  /**
   * Destructor
   */
  virtual ~ConfigFile() = default;

  /**
   * Load config file in specific path
   * @param file_name, file path
   * @return whether read file successfully
   */
  aicpu::State LoadFile(const std::string &file_name);
  /**
   * split string
   * @param str, src string
   * @param result, vector used to store result
   * @param blacklist, engine prohibit
   */
  void SplitValue(
      const std::string &str, std::vector<std::string> &result,
      const std::set<std::string> &blacklist = std::set<std::string>()) const; //lint !e562

  /**
   * Get value by specific key
   * @param key, config key name
   * @param value, config key's value
   * @return config file's value of specific key
   */
  bool GetValue(const std::string &key, std::string &value) const;

  // Copy prohibit
  ConfigFile(const ConfigFile &) = delete;
  // Copy prohibit
  ConfigFile &operator=(const ConfigFile &) = delete;
  // Move prohibit
  ConfigFile(ConfigFile &&) = delete;
  // Move prohibit
  ConfigFile &operator=(ConfigFile &&) = delete;

 private:
  // Constructor
  ConfigFile() : is_load_file_(false) {}
  /**
   * Trim string
   * @param source, source string to be processed
   * @param delims, delimit exist in source string e.g. space
   * @return trim source string result
   */
  std::string Trim(const std::string &source, const char delims = ' ') const;

 private:
  // store config item by key-value pair
  std::map<std::string, std::string> data_map_;

  bool is_load_file_;
};
}  // namespace aicpu

#endif  // AICPU_CONFIG_FILE_H_
