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

#ifndef GE_SINGLE_OP_SINGLE_OP_MODEL_H_
#define GE_SINGLE_OP_SINGLE_OP_MODEL_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "framework/common/helper/model_helper.h"
#include "single_op/single_op.h"
#include "single_op/stream_resource.h"
#include "single_op/task/op_task.h"

namespace ge {
struct SingleOpModelParam {
  uint64_t base_addr = 0U;
  uint64_t memory_size = 0U;
  uint64_t weight_addr = 0U;
  uint64_t weight_size = 0U;
  uint64_t zero_copy_mem_size = 0U;

  uint8_t *mem_base = nullptr;
  uint8_t *weight_base = nullptr;

  std::map<uintptr_t, int32_t> addr_mapping_;
  int64_t core_type = 0;
  bool graph_is_dynamic = false;
};

class SingleOpModel {
 public:
  SingleOpModel(const std::string &model_name, const void *const model_data, const uint32_t model_size);
  ~SingleOpModel() = default;

  Status Init();
  Status BuildOp(StreamResource &resource, SingleOp &single_op);
  Status BuildDynamicOp(StreamResource &resource, DynamicSingleOp &single_op);

 private:
  Status InitModel();
  Status LoadAllNodes();
  Status LoadAtomicWorkspace(const OpDescPtr &op_desc) const;
  Status ParseInputsAndOutputs();
  Status SetInputsAndOutputs(SingleOp &single_op);

  Status InitModelMem(StreamResource &resource);
  Status InitOverflowAddr(StreamResource &resource);

  Status ParseInputNode(const OpDescPtr &op_desc);
  void ParseOutputNode(const OpDescPtr &op_desc);

  Status BuildTaskList(StreamResource *const stream_resource, SingleOp &single_op);
  Status BuildTaskListForDynamicOp(StreamResource *const stream_resource, DynamicSingleOp &single_op);
  Status BuildKernelTask(const domi::TaskDef &task_def, TbeOpTask **const task, void *const overflow_addr);
  Status BuildAtomicTask(const domi::TaskDef &task_def, AtomicAddrCleanOpTask **const task);
  Status BuildKernelExTask(const domi::KernelExDef &kernel_def, AiCpuTask **const task, const uint64_t kernel_id);
  Status BuildCpuKernelTask(const domi::KernelDef &kernel_def, AiCpuCCTask **const task, const uint64_t kernel_id);

  static void ParseOpModelParams(ModelHelper &model_helper, SingleOpModelParam &param);
  void ParseArgTable(OpTask *const task, SingleOp &op);
  Status InitHybridModelExecutor(const StreamResource &resource, const GeModelPtr &ge_model, SingleOp &single_op);
  void SetHostMemTensorAndNode(DynamicSingleOp &single_op) const;
  Status NeedHybridModel(const GeModelPtr &ge_model, bool &need_hybrid_model) const;
  Status ParseTasks();
  Status SetHostMemNode(std::vector<NodePtr> &node_with_hostmem);
  void SetHostMemInputFlagToTask(const OpDescPtr &op_desc, OpTask &task) const;

  std::map<NodePtr, std::vector<domi::TaskDef>> node_tasks_;
  std::string model_name_;
  uint32_t model_id_ = 0U;
  const void *ori_model_data_;
  uint32_t ori_model_size_;

  ModelHelper model_helper_;

  std::map<uint32_t, NodePtr> op_list_;
  std::map<int32_t, NodePtr> op_with_hostmem_;
  SingleOpModelParam model_params_;

  std::vector<ptrdiff_t> input_offset_list_;
  std::vector<size_t> input_sizes_;
  std::vector<ptrdiff_t> output_offset_list_;
  std::vector<size_t> output_sizes_;
  std::vector<OpDescPtr> data_ops_;
  OpDescPtr netoutput_op_;
  bool has_weight_ = false;
};
}  // namespace ge

#endif  // GE_SINGLE_OP_SINGLE_OP_MODEL_H_
