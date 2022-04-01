/**
 * Copyright 2022-2022 Huawei Technologies Co., Ltd
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

#ifndef GE_COMMON_UTILS_EXECUTOR_UTILS_H_
#define GE_COMMON_UTILS_EXECUTOR_UTILS_H_

#include "graph/op_desc.h"
#include "hybrid/node_executor/task_context.h"
#include "single_op/task/op_task.h"
#include "graph/manager/graph_var_manager.h"

namespace ge {
class ExecutorUtils {
 public:
  static bool HasHostMemInput(const OpDescPtr &op_desc);
  static Status UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs,
                                       const OpTask &op_task,
                                       void *const io_base,
                                       const size_t io_size,
                                       vector<rtHostInputInfo_t> &host_inputs);
  static Status UpdateHostMemInputArgs(const hybrid::TaskContext &context,
                                       void *const io_addrs,
                                       const size_t io_addrs_size,
                                       const size_t host_mem_input_data_offset_in_args,
                                       vector<rtHostInputInfo_t> &host_inputs, bool need_64b_aligned = false);
  static bool IsReuseZeroCopyMemory();
  static bool IsGeUseExtendSizeStaticMemory();
};
}

#endif  // GE_COMMON_UTILS_EXECUTOR_UTILS_H_
