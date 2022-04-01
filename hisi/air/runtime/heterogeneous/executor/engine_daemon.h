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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_ENGINE_DAEMON_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_ENGINE_DAEMON_H_

#include <string>
#include "runtime/rt.h"
#include "framework/common/debug/ge_log.h"
#include "framework/executor/ge_executor.h"
#include "common/utils/rts_api_utils.h"
#include "executor/executor_context.h"
#include "executor/event_handler.h"
#include "executor_event_defs.h"

namespace ge {
class EngineDaemon {
 public:
  explicit EngineDaemon(bool is_host_cpu = false);

  Status InitializeWithArgs(int32_t argc, char *argv[]);
  void Finalize();
  Status LoopEvents();

 private:
  Status InitializeGeExecutor();
  Status InitializeMaintenanceFromOption();
  Status FinalizeMaintenance();
  Status InitDumpFromOption(std::map<std::string, std::string> &options);
  Status InitProfilingFromOption(const std::map<std::string, std::string> &options_all);
  Status ParseCmdLineArgs(int32_t argc, char *argv[]);
  static void GetGlobalEnvOptions(std::map<std::string, std::string> &env_option);

  Status WaitEvent(ExecutorEventType &event_type, bool &is_timeout);

  Status HandleEvent();

  Status InitMessageQueue();

  Status NotifyInitialized();

  Status SendEvent(ExecutorEventType type, const std::string &msg = "") const;

  template<typename T>
  Status ToNumber(const char *num_str, T &value) {
    GE_CHECK_NOTNULL(num_str);
    std::stringstream  ss(num_str);
    ss >> value;
    if (ss.fail()) {
      GELOGE(PARAM_INVALID, "Failed to convert [%s] to number", num_str);
      return PARAM_INVALID;
    }
    if (!ss.eof()) {
      GELOGE(PARAM_INVALID, "Failed to convert [%s] to number", num_str);
      return PARAM_INVALID;
    }
    return SUCCESS;
  }
  void TransArray2ArgsOption(const int32_t start, const int32_t end, char_t **argv);

  GeExecutor ge_executor_;
  EventHandler event_handler_;
  std::string mem_group_name_;
  int32_t device_id_ = -1;
  uint32_t event_group_id_ = UINT32_MAX;
  uint32_t msg_queue_id_ = UINT32_MAX;
  pid_t parent_pid_;
  bool is_host_cpu_ = false;
  bool is_dump_inited_ = false;
  std::map<std::string, std::string> args_option_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_ENGINE_DAEMON_H_
