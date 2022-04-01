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
#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_MODEL_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_MODEL_H_

#include <cstdint>
#include <vector>
#include "aicpu/aicpu_schedule/aicpusd_info.h"

namespace ge {
class CpuSchedModel {
 public:
  void LogModelDesc() const;

 private:
  friend class CpuSchedModelBuilder;
  friend class DynamicModelExecutor;
  ::ModelInfo model_info_;
  std::vector<::ModelTaskInfo> tasks_;
  std::vector<::ModelQueueInfo> queues_;
  std::vector<::ModelStreamInfo> streams_;
  std::vector<std::vector<uint8_t>> task_params_;
};

class CpuSchedModelBuilder {
 public:
  explicit CpuSchedModelBuilder(CpuSchedModel &model);
  ~CpuSchedModelBuilder() = default;

  void Build();
  void SetModelId(uint32_t model_id);
  void AddInputQueue(uint32_t queue_id, uintptr_t mbuf_addr);
  void AddOutputQueue(uint32_t queue_id, uintptr_t mbuf_addr);

 private:
  void AddDequeueTasks();
  void AddEnqueueTasks();
  void AddActivateTask();
  void AddWaitEndGraph();
  void AddModelRepeat();
  void AddQueueOpTask(const char *kernel_name, uint32_t queue_id, uintptr_t mbuf_addr);

  uint8_t *NewTask(const char *kernel_name, size_t param_size);

  CpuSchedModel &model_;
  std::vector<std::pair<uint32_t, uintptr_t>> input_queue_infos_;
  std::vector<std::pair<uint32_t, uintptr_t>> output_queue_infos_;
  uint32_t task_id_gen_ = 0;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_CPU_SCHED_MODEL_H_
