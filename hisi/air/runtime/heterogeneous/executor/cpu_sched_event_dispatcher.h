/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_EVENT_DISPATCHER_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_EVENT_DISPATCHER_H_

#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "runtime/rt_mem_queue.h"
#include "ge/ge_api_error_codes.h"

namespace ge {
class DynamicModelExecutor;
class CpuSchedEventDispatcher {
 public:
  static CpuSchedEventDispatcher &GetInstance() {
    static CpuSchedEventDispatcher instance;
    return instance;
  }

  ~CpuSchedEventDispatcher();

  Status Initialize(int32_t device_id);

  void Finalize();

  void Register(uint32_t model_id, DynamicModelExecutor *executor);
  void Deregister(uint32_t model_id);

  void ProcessEvents();

 private:
  CpuSchedEventDispatcher() = default;
  Status OnInputsReady(rtEschedEventSummary_t &in_event);
  Status OnModelExecuted(uint32_t model_id, Status status) const;

  int32_t device_id_ = -1;
  int32_t aicpu_sd_pid_ = -1;
  uint32_t event_group_id_ = 10U;

  std::thread event_handle_thread_;
  std::mutex mu_;
  std::atomic_bool running_{};
  // key: model_id
  std::map<uint32_t, DynamicModelExecutor *> models_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_EVENT_DISPATCHER_H_