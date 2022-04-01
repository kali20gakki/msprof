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
#ifndef AIR_RUNTIME_DEPLOY_DAEMON_EXECUTOR_MANAGER_H_
#define AIR_RUNTIME_DEPLOY_DAEMON_EXECUTOR_MANAGER_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "ge/ge_api_error_codes.h"
#include "executor/executor_event_defs.h"
#include "common/config/device_debug_config.h"
#include "proto/deployer.pb.h"

namespace ge {
class ExecutorProcess {
 public:
  ExecutorProcess(int32_t pid, int32_t device_id, uint32_t msg_queue_id, std::string group_name);
  virtual ~ExecutorProcess();
  Status Initialize();
  Status Finalize();
  Status SendRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &resp) const;
  int32_t GetPid() const;

 private:
  Status InitEventGroup();
  Status InitMessageQueue();
  Status SendEvent(ExecutorEventType event_type) const;
  Status WaitForExecutorInitialized();
  Status WaitEvent(ExecutorEventType expected_event_type) const;
  int32_t pid_;
  int32_t device_id_;
  uint32_t event_group_id_ = 1U;
  uint32_t msg_queue_id_ = UINT32_MAX;
  std::string group_name_;
};

class ExecutorManager {
 public:
  struct ExecutorId {
    int64_t context_id;
    int32_t device_id;
    bool operator<(const ExecutorId &other) const {
      if (context_id != other.context_id) {
        return context_id < other.context_id;
      } else {
        return device_id < other.device_id;
      }
    }
  };

  explicit ExecutorManager(bool is_host = false) : is_host_(is_host), dev_maintenance_cfg_(nullptr) {
  }

  Status GetOrForkExecutorProcess(const ExecutorId &context, ExecutorProcess **executor_process);
  Status GetExecutorProcess(const ExecutorId &context, ExecutorProcess **executor_process) const;
  static Status GetCfgArgs(const char_t **fixed_argv, size_t fix_size,
                           const std::vector<std::string> &args_strings, const char_t **var_args);
  void SetMaintenanceCfg(DeviceMaintenanceClientCfg *dev_maintenance_cfg) {
    dev_maintenance_cfg_ = dev_maintenance_cfg;
  }

  bool IsHost() const;
 private:
  Status LoadAndExecuteChildProcess(int32_t device_id,
                                    uint32_t msg_queue_id,
                                    const std::string &group_name);
  Status ForAndInit(int32_t device_id, std::unique_ptr<ExecutorProcess> &executor_process);

  mutable std::mutex mu_;
  bool is_host_ = false; // TODO replace by executor type
  std::map<ExecutorId, std::unique_ptr<ExecutorProcess>> processes_;
  DeviceMaintenanceClientCfg *dev_maintenance_cfg_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_DAEMON_EXECUTOR_MANAGER_H_
