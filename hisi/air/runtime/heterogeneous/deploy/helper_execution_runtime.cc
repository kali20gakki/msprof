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
#include "deploy/helper_execution_runtime.h"
#include "common/data_flow/queue/helper_exchange_service.h"
#include "deploy/flowrm/datagw_manager.h"
#include "deploy/flowrm/network_manager.h"
#include "common/mem_grp/memory_group_manager.h"
#include "common/utils/rts_api_utils.h"
#include "deploy/deployer/master_model_deployer.h"
#include "exec_runtime/execution_runtime.h"

namespace ge {
namespace {
constexpr int32_t kWaitDataGwServerWorkTimeInSec = 5;
}
class HelperExecutionRuntime : public ExecutionRuntime {
 public:
  static HelperExecutionRuntime& GetInstance() {
    static HelperExecutionRuntime instance;
    return instance;
  }
  Status Initialize(const map<std::string, std::string> &options) override;
  Status Finalize() override;
  ModelDeployer &GetModelDeployer() override;
  ExchangeService &GetExchangeService() override;

 private:
  Status InitDataGwManager();

  int32_t device_id_ = 0;
  MasterModelDeployer model_deployer_;
};

Status HelperExecutionRuntime::InitDataGwManager() {
  pid_t dgw_pid = 0;
  const auto &mem_group_name = MemoryGroupManager::GetInstance().GetQsMemGroupName();
  GE_CHK_STATUS_RET(DataGwManager::GetInstance().InitHostDataGwServer(device_id_, 0U, mem_group_name, dgw_pid),
                    "Failed to init DataGwManager, group_name = %s", mem_group_name.c_str());
  GELOGI("InitHostDataGwServer succeeded, pid = %d", dgw_pid);
  GE_CHK_STATUS_RET_NOLOG(HelperExchangeService::GetInstance().EnsureInitialized(device_id_));
  std::this_thread::sleep_for(std::chrono::seconds(kWaitDataGwServerWorkTimeInSec));
  GE_CHK_STATUS_RET(DataGwManager::GetInstance().InitDataGwClient(dgw_pid, static_cast<uint32_t>(device_id_)),
                    "Failed to init DataGwClient");
  return SUCCESS;
}

Status HelperExecutionRuntime::Initialize(const map<std::string, std::string> &options) {
  constexpr char *kConfigFilePathEnv = "HELPER_RES_FILE_PATH";
  char *path = std::getenv(kConfigFilePathEnv);
  if (path == nullptr) {
    REPORT_CALL_ERROR("E19999", "Check host env path failed.");
    GELOGE(FAILED, "[Check][EnvPath] Check host env path failed.");
    return FAILED;
  }

  GE_CHK_STATUS_RET_NOLOG(Configurations::GetInstance().InitHostInformation(path));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufInit());
  (void) MemoryGroupManager::GetInstance().Initialize();
  GE_CHK_STATUS_RET(NetworkManager::GetInstance().Initialize(), "Failed to init NetworkManager");
  GE_CHK_STATUS_RET_NOLOG(InitDataGwManager());
  GE_CHK_STATUS_RET(HelperExchangeService::GetInstance().Initialize(), "Failed to init model deployer");
  GE_CHK_STATUS_RET(model_deployer_.Initialize(options), "Failed to init model deployer");
  return SUCCESS;
}

Status HelperExecutionRuntime::Finalize() {
  GE_CHK_STATUS(model_deployer_.Finalize());
  GE_CHK_STATUS(HelperExchangeService::GetInstance().Finalize());
  GE_CHK_STATUS(DataGwManager::GetInstance().Shutdown());
  GE_CHK_STATUS(NetworkManager::GetInstance().Finalize());
  return SUCCESS;
}

ModelDeployer &HelperExecutionRuntime::GetModelDeployer() {
  return model_deployer_;
}

ExchangeService &HelperExecutionRuntime::GetExchangeService() {
  return HelperExchangeService::GetInstance();
}
}  // namespace ge

ge::Status InitializeHelperRuntime(const std::map<std::string, std::string> &options) {
  auto helper_runtime = ge::MakeShared<ge::HelperExecutionRuntime>();
  GE_CHECK_NOTNULL(helper_runtime);
  GE_CHK_STATUS_RET_NOLOG(helper_runtime->Initialize(options));
  ge::ExecutionRuntime::SetExecutionRuntime(helper_runtime);
  return ge::SUCCESS;
}