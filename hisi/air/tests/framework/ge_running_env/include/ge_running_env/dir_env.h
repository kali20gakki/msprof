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

#ifndef AIR_CXX_TESTS_ST_STUBS_UTILS_DIR_ENV_H_
#define AIR_CXX_TESTS_ST_STUBS_UTILS_DIR_ENV_H_
#include "ge_running_env/fake_ns.h"
#include <string>

FAKE_NS_BEGIN
class DirEnv {
 public:
  void InitDir();
  static DirEnv &GetInstance();

 private:
  DirEnv();
  void InitEngineConfJson();
  void InitEngineSo();
  void InitOpsKernelInfoStore();

 private:
  std::string run_root_path_;
  std::string engine_path_;
  std::string ops_kernel_info_store_path_;
  std::string engine_config_path_;
};
FAKE_NS_END
#endif //AIR_CXX_TESTS_ST_STUBS_UTILS_DIR_ENV_H_
