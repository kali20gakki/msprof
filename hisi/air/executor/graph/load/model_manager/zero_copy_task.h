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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_TASK_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_TASK_H_

#include <map>
#include <set>
#include <vector>
#include <string>

#include "external/ge/ge_api_error_codes.h"
#include "runtime/mem.h"

namespace ge {
class ZeroCopyTask {
 public:
  ZeroCopyTask(const std::string &name, const uintptr_t args_base, const size_t size);
  ~ZeroCopyTask();

  bool CanMerge(const size_t args_size) const;

  Status Merge(const std::map<uintptr_t, std::set<size_t>> &args_offset, const size_t args_size,
               const void *const args_host_copy, void *&args_device_addr);
  /**
   * @ingroup ge
   * @brief Set Task zero copy addr info.
   * @param [in] addr: task addr value.
   * @param [in] offset: saved offset in task args.
   * @return: 0 SUCCESS / others FAILED
   */
  Status SetTaskArgsOffset(const uintptr_t addr, const size_t offset);

  const std::map<uintptr_t, std::set<size_t>> &GetTaskArgsOffset() const { return task_addr_offset_; }

  /**
   * @ingroup ge
   * @brief Is need zero copy.
   * @return: true / false
   */
  bool IsTaskArgsSet() const { return !task_addr_offset_.empty(); }

  /**
   * @ingroup ge
   * @brief Save orignal data of task args.
   * @param [in] info: task args orignal data.
   * @param [in] size: args size.
   * @return: void
   */
  void SetOriginalArgs(const void *const info, const size_t size);

  /**
   * @ingroup ge
   * @brief Set user data addr to Task param.
   * @param [in] addr: virtual address value from Op.
   * @param [in] buffer_addr: data buffer_addr from user.
   * @return: 0 SUCCESS / others FAILED
   */
  Status UpdateTaskParam(const uintptr_t addr, const uintptr_t buffer_addr);

  /**
   * @ingroup ge
   * @brief Update task param to device.
   * @param [in] async_mode: true for asychronous mode.
   * @param [in] stream: Stream for asychronous update.
   * @return: 0 SUCCESS / others FAILED
   */
  Status DistributeParam(const bool async_mode, rtStream_t stream);

  void SetBatchLabel(const std::string &batch_label) {
    batch_label_ = batch_label;
  }

  const std::string &GetBatchLabel() const {
    return batch_label_;
  }

  const std::string &GetName() const {
    return name_;
  }

 private:
  const std::string name_;

  void *args_addr_;
  const size_t args_size_;
  std::vector<uint8_t> args_info_;
  bool is_updated_;
  std::string batch_label_;
  // <address from Op, {offset in args}>
  std::map<uintptr_t, std::set<size_t>> task_addr_offset_;
  size_t used_args_size_;
};
} // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_TASK_H_
