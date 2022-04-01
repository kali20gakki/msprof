/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_UTILS_ADAPTER_ADAPTER_ITF_TASK_BUILDER_ADAPTER_H_
#define FUSION_ENGINE_UTILS_ADAPTER_ADAPTER_ITF_TASK_BUILDER_ADAPTER_H_

#include <memory>
#include <string>
#include <vector>
#include "proto/task.pb.h"
#include "graph/node.h"
#include "runtime/base.h"
#include "common/resource_def.h"
#include "register/graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {

class TaskBuilderAdapter;
using TaskBuilderAdapterPtr = std::shared_ptr<TaskBuilderAdapter>;

struct TaskBuilderContext {
  uint64_t dataMemSize = 0;
  uint8_t *dataMemBase = nullptr;
  uint64_t weightMemSize = 0;
  uint8_t *weightMemBase = nullptr;
  ge::Buffer weightBufferHost;
  rtModel_t model = nullptr;
  rtStream_t stream = nullptr;
  ccHandle_t handle = nullptr;
  std::map<int64_t, uint8_t *> mem_type_to_data_mem_base;
  std::map<int64_t, uint64_t> mem_type_to_data_mem_size;
};

struct TaskArgs {
  vector<void *> input_addrs;
  vector<void *> output_addrs;
  // Workspace
  vector<void *> workspace_addrs;
};

class TaskBuilderAdapter {
 public:
  TaskBuilderAdapter(const ge::Node &node, TaskBuilderContext &context);
  virtual ~TaskBuilderAdapter();

  static Status GetHandleStream(ccHandle_t handle, rtStream_t *streamId);

  /*
   * @ingroup fe
   * @brief   Init TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  virtual Status Init();

  /*
   * @ingroup fe
   * @brief   Run TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  virtual Status Run(domi::TaskDef &task_def);

  ge::ConstOpDescPtr GetOpDesc() { return op_desc_; }

  void GetTaskArgs(TaskArgs &args_info) const;

 protected:
  virtual Status InitInput() = 0;
  virtual Status InitOutput();
  virtual Status InitWorkspace();
  virtual Status InitInputL1Addr();
  virtual Status CheckOffsetAndSize(int64_t offset, uint64_t space_size, uint64_t total_size);

 protected:
  const ge::Node &node_;
  ge::OpDescPtr op_desc_;
  TaskBuilderContext &context_;

  vector<void *> input_addrs_;

  vector<void *> output_addrs_;

  // Workspace
  vector<void *> workspace_addrs_;
  vector<uint32_t> workspace_sizes_;

  vector<void *> input_l1_addrs_;

 private:
  TaskBuilderAdapter(const TaskBuilderAdapter &) = delete;
  TaskBuilderAdapter &operator=(const TaskBuilderAdapter &) = delete;

  Status VerifyWeights();
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_ADAPTER_ADAPTER_ITF_TASK_BUILDER_ADAPTER_H_