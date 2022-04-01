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

#ifndef GE_COMMON_PROFILING_PROFILING_INIT_H_
#define GE_COMMON_PROFILING_PROFILING_INIT_H_

#include <nlohmann/json.hpp>
#include <string>

#include "common/profiling/profiling_properties.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "toolchain/prof_callback.h"

using Json = nlohmann::json;

namespace ge {
class ProfilingInit {
 public:
  static ProfilingInit &Instance();
  Status Init(const Options &options);
  void StopProfiling();
  Status ProfRegisterCtrlCallback();
  void ShutDownProfiling();
  Status SetDeviceIdByModelId(uint32_t model_id, uint32_t device_id);
  Status UnsetDeviceIdByModelId(uint32_t model_id, uint32_t device_id);

 private:
  ProfilingInit() = default;
  ~ProfilingInit() = default;
  Status InitFromOptions(const Options &options, MsprofGeOptions &prof_conf, bool &is_execute_profiling);
  Status ParseOptions(const std::string &options);
  uint64_t GetProfilingModule();
};
}  // namespace ge

#endif  // GE_COMMON_PROFILING_PROFILING_INIT_H_
