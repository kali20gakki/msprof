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
#ifndef EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_AICPU_RESOURCES_H_
#define EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_AICPU_RESOURCES_H_

#include <mutex>

#include "ge/ge_api_types.h"
#include "common/plugin/ge_util.h"
#include "graph/node.h"

namespace ge {
class AiCpuResources {
 public:
  AiCpuResources() = default;
  ~AiCpuResources();
  GE_DELETE_ASSIGN_AND_COPY(AiCpuResources);

  static const std::string &ResourceTypeQueue();
  static const std::string &ResourceTypeChannel();
  static const std::string &ResourceTypeVdecChannel();
  Status AllocateChannelResource(const OpDescPtr &op_desc,
                                 const int32_t rt_stream_id);
  Status AllocateQueueResource(const OpDescPtr &op_desc,
                               const NamedAttrs &resource_attr,
                               int32_t &input_idx,
                               uint32_t &queue_id);
  Status AllocateVdecChannelResource(const OpDescPtr &op_desc,
                                     const int32_t rt_stream_id);
  Status GetOrCreateQueue(const std::string &queue_name, const uint32_t queue_depth, uint32_t &queue_id);
  void ReleaseResources();

 private:
  static Status CreateQueue(const std::string &name, const uint32_t depth, uint32_t &queue_id);
  static Status DestroyQueue(const uint32_t queue_id);

  static Status BuildCreateQueueTask(const uintptr_t queue_id_dev,
                                     const std::string &name,
                                     const uint32_t depth,
                                     std::vector<uint8_t> &task_args);
  static Status BuildDestroyQueueTask(const uint32_t queue_id, std::vector<uint8_t> &task_args);

  static Status CreateChannel(const int32_t rt_stream_id);
  static Status DestroyChannel(const int32_t rt_stream_id);
  static Status BuildCreateChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args);
  static Status BuildDestroyChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args);

  static Status CreateVdecChannel(const int32_t rt_stream_id);
  static Status BuildCreateVdecChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args);
  static Status DestroyVdecChannel(const int32_t rt_stream_id);
  static Status BuildDestroyVdecChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args);

  static Status ExecuteKernel(const char_t *const so_name,
                              const std::string &kernel_name,
                              const std::vector<uint8_t> &task_args);
  static Status ExecuteKernel(const std::string &kernel_name, const std::vector<uint8_t> &task_args);

  std::mutex mu_;
  std::map<std::string, uint32_t> aicpu_queues_;
  std::set<int32_t> aicpu_channels_;
  std::set<int32_t> aicpu_vdec_channels_;
};
}  // namespace ge
#endif  // EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_AICPU_RESOURCES_H_
