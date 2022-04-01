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

#include "single_op/task/rts_kernel_task_builder.h"
#include "graph/def_types.h"
#include "single_op/task/build_task_utils.h"
#include "common/plugin/ge_util.h"

namespace ge {
namespace {
const size_t kNumAddresses = 2UL;
}  // namespace

Status RtsKernelTaskBuilder::BuildMemcpyAsyncTask(const OpDescPtr &op_desc,
                                                  const domi::MemcpyAsyncDef &kernel_def,
                                                  const SingleOpModelParam &param,
                                                  std::unique_ptr<MemcpyAsyncTask> &task) {
  task = MakeUnique<MemcpyAsyncTask>();
  GE_CHECK_NOTNULL(task);
  task->SetOpDesc(op_desc);
  task->dst_max_ = kernel_def.dst_max();
  task->count_ = kernel_def.count();
  task->kind_ = static_cast<rtMemcpyKind_t>(kernel_def.kind());
  const auto addresses = BuildTaskUtils::JoinAddresses(BuildTaskUtils::GetAddresses(op_desc, param, false));
  if (addresses.size() != kNumAddresses) {
    GELOGE(INTERNAL_ERROR, "[Build][MemcpyAsyncTask] Invalid address count: %zu", addresses.size());
    return INTERNAL_ERROR;
  }

  task->addresses_[0U] = static_cast<uintptr_t>(PtrToValue(addresses[0UL]));
  task->addresses_[1U] = static_cast<uintptr_t>(PtrToValue(addresses[1UL]));
  return SUCCESS;
}
}  // namespace ge