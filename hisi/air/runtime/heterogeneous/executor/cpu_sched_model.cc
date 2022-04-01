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

#include "executor/cpu_sched_model.h"
#include "runtime/stream.h"
#include "common/debug/ge_log.h"
#include "graph/def_types.h"

namespace ge {
namespace {
constexpr const char *kCpuSdTaskModelEnqueue = "modelEnqueue";
constexpr const char *kCpuSdTaskWaitEndGraph = "modelWaitEndGraph";
constexpr const char *kCpuSdTaskModelDequeue = "modelDequeue";
constexpr const char *kCpuSdTaskModelRepeat = "modelRepeat";
constexpr const char *kCpuSdTaskActivateModel = "activeModel";
constexpr uint32_t kQueueFlagInput = 0U;
constexpr uint32_t kQueueFlagOutput = 1U;

struct QueueOpTaskParam {
  uint32_t queue_id;
  uint64_t mbuf_addr;
};
}  // namespace

CpuSchedModelBuilder::CpuSchedModelBuilder(CpuSchedModel &model) : model_(model) {
}

uint8_t *CpuSchedModelBuilder::NewTask(const char *kernel_name, size_t param_size) {
  model_.task_params_.emplace_back(std::vector<uint8_t>(param_size));
  auto &param_base = model_.task_params_.back();
  ::ModelTaskInfo task_info{};
  task_info.taskId = task_id_gen_++;
  task_info.kernelName = PtrToValue(kernel_name);
  task_info.paraBase = PtrToValue(param_base.data());
  model_.tasks_.emplace_back(task_info);
  return param_base.data();
}

void CpuSchedModelBuilder::AddDequeueTasks() {
  for (const auto &input_queue_info : input_queue_infos_) {
    model_.queues_.emplace_back(::ModelQueueInfo{input_queue_info.first, kQueueFlagInput});
    AddQueueOpTask(kCpuSdTaskModelDequeue, input_queue_info.first, input_queue_info.second);
  }
}

void CpuSchedModelBuilder::AddActivateTask() {
  auto task_param = NewTask(kCpuSdTaskActivateModel, sizeof(uint32_t));
  *reinterpret_cast<uint32_t *>(task_param) = model_.model_info_.modelId;
}

void CpuSchedModelBuilder::AddWaitEndGraph() {
  auto task_param = NewTask(kCpuSdTaskWaitEndGraph, sizeof(uint32_t));
  *reinterpret_cast<uint32_t *>(task_param) = model_.model_info_.modelId;
}

void CpuSchedModelBuilder::AddEnqueueTasks() {
  for (const auto &output_queue_info : output_queue_infos_) {
    model_.queues_.emplace_back(::ModelQueueInfo{output_queue_info.first, kQueueFlagOutput});
    AddQueueOpTask(kCpuSdTaskModelEnqueue, output_queue_info.first, output_queue_info.second);
  }
}

void CpuSchedModelBuilder::AddModelRepeat() {
  auto task_param = NewTask(kCpuSdTaskModelRepeat, sizeof(uint32_t));
  *reinterpret_cast<uint32_t *>(task_param) = model_.model_info_.modelId;
}

void CpuSchedModelBuilder::AddQueueOpTask(const char *kernel_name,
                                          uint32_t queue_id,
                                          uintptr_t mbuf_addr) {
  auto task_param = reinterpret_cast<QueueOpTaskParam *>(NewTask(kernel_name, sizeof(uint32_t) + sizeof(uintptr_t)));
  task_param->queue_id = queue_id;
  task_param->mbuf_addr = mbuf_addr;
}

void CpuSchedModelBuilder::AddInputQueue(uint32_t queue_id, uintptr_t mbuf_addr) {
  input_queue_infos_.emplace_back(queue_id, mbuf_addr);
}

void CpuSchedModelBuilder::AddOutputQueue(uint32_t queue_id, uintptr_t mbuf_addr) {
  output_queue_infos_.emplace_back(queue_id, mbuf_addr);
}

void CpuSchedModelBuilder::Build() {
  AddDequeueTasks();
  AddActivateTask();
  AddWaitEndGraph();
  AddEnqueueTasks();
  AddModelRepeat();
  auto &model_info = model_.model_info_;
  model_info.queueNum = model_.queues_.size();
  model_info.queues = model_.queues_.data();

  model_.streams_.resize(1);
  auto &stream_info = model_.streams_.back();
  stream_info.streamId = 1;
  stream_info.streamFlag = RT_STREAM_AICPU | RT_STREAM_HEAD;
  stream_info.taskNum = model_.tasks_.size();
  stream_info.tasks = model_.tasks_.data();

  model_info.aicpuStreamNum = model_.streams_.size();
  model_info.streams = model_.streams_.data();
}

void CpuSchedModelBuilder::SetModelId(uint32_t model_id) {
  model_.model_info_.modelId = model_id;
}

void CpuSchedModel::LogModelDesc() const {
  auto num_streams = static_cast<int32_t>(model_info_.aicpuStreamNum);
  int32_t num_tasks = num_streams > 0 ? static_cast<int32_t>(model_info_.streams[0].taskNum) : 0;
  GELOGD("model_id = %u, num_queues = %d, num_streams = %d, num_tasks = %d",
         model_info_.modelId,
         static_cast<int32_t>(model_info_.queueNum),
         num_streams,
         num_tasks);
  for (int32_t i = 0; i < static_cast<int32_t>(model_info_.queueNum); ++i) {
    GELOGD("queue[%d]: queue_flag = %u, queue_id = %u", i, model_info_.queues[i].flag, model_info_.queues[i].queueId);
  }
  for (int32_t i = 0; i < num_tasks; ++i) {
    auto task = model_info_.streams[0].tasks[i];
    GELOGD("Tasks[%d], task_id = %d, kernel_name = %s, param_base = 0x%lx",
           i, task.taskId, reinterpret_cast<const char *>(task.kernelName), task.paraBase);
  }
}
}  // namespace ge