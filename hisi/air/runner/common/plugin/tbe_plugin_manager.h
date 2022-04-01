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

#ifndef GE_COMMON_GE_TBE_PLUGIN_MANAGER_H_
#define GE_COMMON_GE_TBE_PLUGIN_MANAGER_H_

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "external/ge/ge_api_error_codes.h"
#include "external/register/register.h"

namespace ge {
using SoHandlesVec = std::vector<void *>;

class TBEPluginManager {
 public:
  Status Finalize();

  // Get TBEPluginManager singleton instance
  static TBEPluginManager& Instance();

  static std::string GetPath();

  static void InitPreparation(const std::map<std::string, std::string> &options);

  void LoadPluginSo(const std::map< std::string, std::string> &options);

 private:
  TBEPluginManager() = default;
  ~TBEPluginManager() = default;
  Status ClearHandles_();

  static void ProcessSoFullName(std::vector<std::string> &file_list, std::string &caffe_parser_path,
                                const std::string &full_name, const std::string &caffe_parser_so_suff);
  static void FindParserUsedSo(const std::string &path, std::vector<std::string> &file_list,
                               std::string &caffe_parser_path, const uint32_t recursive_depth = 0U);
  static void GetOpPluginSoFileList(const std::string &path, std::vector<std::string> &file_list,
                                    std::string &caffe_parser_path);
  static void GetCustomOpPath(std::string &customop_path);
  void LoadCustomOpLib();

  SoHandlesVec handles_vec_;
  static std::map<std::string, std::string> options_;
};
}  // namespace ge

#endif  // GE_COMMON_GE_TBE_PLUGIN_MANAGER_H_
