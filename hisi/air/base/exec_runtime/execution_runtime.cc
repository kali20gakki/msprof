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
#include "exec_runtime/execution_runtime.h"
#include "mmpa/mmpa_api.h"

namespace ge {
namespace {
constexpr const char_t *kHeterogeneousRuntimeLibName = "libgrpc_client.so";
}
std::mutex ExecutionRuntime::mu_;
bool ExecutionRuntime::heterogeneous_enabled_ = true;
void *ExecutionRuntime::handle_;
std::shared_ptr<ExecutionRuntime> ExecutionRuntime::instance_;

void ExecutionRuntime::SetExecutionRuntime(const std::shared_ptr<ExecutionRuntime> &instance) {
  const std::lock_guard<std::mutex> lk(mu_);
  instance_ = instance;
}

ExecutionRuntime *ExecutionRuntime::GetInstance() {
  const std::lock_guard<std::mutex> lk(mu_);
  return instance_.get();
}

Status ExecutionRuntime::InitHeterogeneousRuntime(const std::map<std::string, std::string> &options) {
  GE_CHK_STATUS_RET_NOLOG(LoadHeterogeneousLib());
  GE_CHK_STATUS_RET_NOLOG(SetupHeterogeneousRuntime(options));
  return SUCCESS;
}

Status ExecutionRuntime::LoadHeterogeneousLib() {
  const auto open_flag =
      static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) | static_cast<uint32_t>(MMPA_RTLD_GLOBAL));
  handle_ = mmDlopen(kHeterogeneousRuntimeLibName, open_flag);
  if (handle_ == nullptr) {
    const auto *error_msg = mmDlerror();
    GE_IF_BOOL_EXEC(error_msg == nullptr, error_msg = "unknown error");
    GELOGE(FAILED, "[Dlopen][So] failed, so name = %s, error_msg = %s", kHeterogeneousRuntimeLibName, error_msg);
    return FAILED;
  }
  GELOGD("Open %s succeeded", kHeterogeneousRuntimeLibName);
  return SUCCESS;
}

Status ExecutionRuntime::SetupHeterogeneousRuntime(const std::map<std::string, std::string> &options) {
  using InitFunc = Status(*)(const std::map<std::string, std::string> &);
  const auto init_func = reinterpret_cast<InitFunc>(mmDlsym(handle_, "InitializeHelperRuntime"));
  if (init_func == nullptr) {
    GELOGE(FAILED, "[Dlsym] failed to find function: InitializeHelperRuntime");
    return FAILED;
  }
  GE_CHK_STATUS_RET(init_func(options), "Failed to invoke InitializeHelperRuntime");
  return SUCCESS;
}

void ExecutionRuntime::FinalizeExecutionRuntime() {
  const auto instance = GetInstance();
  if (instance != nullptr) {
    (void) instance->Finalize();
    instance_ = nullptr;
  }

  if (handle_ != nullptr) {
    GELOGD("close so: %s", kHeterogeneousRuntimeLibName);
    (void) mmDlclose(handle_);
    handle_ = nullptr;
  }
}

bool ExecutionRuntime::IsHeterogeneous() {
  return handle_ != nullptr;
}

bool ExecutionRuntime::IsHeterogeneousEnabled() {
  const std::lock_guard<std::mutex> lk(mu_);
  return heterogeneous_enabled_;
}

void ExecutionRuntime::DisableHeterogeneous() {
  const std::lock_guard<std::mutex> lk(mu_);
  heterogeneous_enabled_ = false;
}
}  // namespace ge