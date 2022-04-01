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

#include "graph/load/model_manager/zero_copy_task.h"

#include "external/runtime/rt_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/util.h"
#include "framework/common/ge_compiler_options.h"
#include "graph/def_types.h"
#include "graph/load/model_manager/model_utils.h"

namespace ge {
ZeroCopyTask::ZeroCopyTask(const std::string &name, const uintptr_t args_base, const size_t size)
    : name_(name), args_addr_(ValueToPtr(args_base)), args_size_(size), is_updated_(false), used_args_size_(0U) {}

ZeroCopyTask::~ZeroCopyTask() { args_addr_ = nullptr; }

/**
 * @ingroup ge
 * @brief Set Task zero copy addr info.
 * @param [in] addr: task addr value.
 * @param [in] offset: saved offset in task args.
 * @return: 0 SUCCESS / others FAILED
 */
Status ZeroCopyTask::SetTaskArgsOffset(const uintptr_t addr, const size_t offset) {
  if ((offset + sizeof(uintptr_t)) > args_size_) {
    REPORT_INNER_ERROR("E19999", "Param offset:%zu + 8 > args_size_:%zu, check invalid", offset, args_size_);
    GELOGE(FAILED, "[Check][Param] [ZCPY] %s set task args failed, args size:%zu, offset:%zu",
           name_.c_str(), args_size_, offset);
    return FAILED;  // unexpected error, need fix.
  }

  const auto it = task_addr_offset_.find(addr);
  if (it == task_addr_offset_.end()) {
    task_addr_offset_[addr] = {offset};
  } else {
    (void)it->second.insert(offset);
  }

  GELOGD("[ZCPY] %s set task, virtual_addr: 0x%lx, args_addr: %p, size: %zu, offset: %zu", name_.c_str(), addr,
         args_addr_, args_size_, offset);
  return SUCCESS;
}

bool ZeroCopyTask::CanMerge(const size_t args_size) const {
  return args_size_ >= (used_args_size_ + args_size);
}

Status ZeroCopyTask::Merge(const std::map<uintptr_t, std::set<size_t>> &args_offset, const size_t args_size,
                           const void *const args_host_copy, void *&args_device_addr) {
  if (!CanMerge(args_size)) {
    return ge::FAILED;
  }
  if (args_info_.empty()) {
    args_info_.resize(args_size_, 0U);
  }
  (void)memcpy_s(&args_info_[used_args_size_], args_size_ - used_args_size_, args_host_copy, args_size);

  for (const auto &item : args_offset) {
    for (const auto &offset : item.second) {
      GE_CHK_STATUS_RET_NOLOG(SetTaskArgsOffset(item.first, used_args_size_ + offset));
    }
  }
  args_device_addr = PtrAdd<uint8_t>(static_cast<uint8_t *>(args_addr_), args_size_, used_args_size_);
  GE_CHK_RT_RET(rtMemcpy(args_device_addr, args_size, args_host_copy, args_size, RT_MEMCPY_HOST_TO_DEVICE));

  const size_t aligned_size = 32U;
  // Keep address of next task in bundle aligned with kAlignedSize
  const size_t aligned_args_size = (((args_size + aligned_size) - 1U) / aligned_size) * aligned_size;
  used_args_size_ = std::min(used_args_size_ + aligned_args_size, args_size_);

  return ge::SUCCESS;
}

/**
 * @ingroup ge
 * @brief Save orignal data of task args.
 * @param [in] info: task args orignal data.
 * @param [in] size: args size.
 * @return: void
 */
void ZeroCopyTask::SetOriginalArgs(const void *const info, const size_t size) {
  GE_CHECK_NOTNULL_JUST_RETURN(info);
  const uint8_t *const data = static_cast<const uint8_t *>(info);
  args_info_.assign(data, data + size);
  GELOGI("[ZCPY] %s set original args info: %p, args_addr: %p, args size: %zu, info size: %zu", name_.c_str(), info,
         args_addr_, args_size_, size);
}

/**
 * @ingroup ge
 * @brief Set user data addr to Task param.
 * @param [in] addr: virtual address value from Op.
 * @param [in] buffer_addr: real_data_buffer_addr from user.
 * @return: void
 */
Status ZeroCopyTask::UpdateTaskParam(const uintptr_t addr, const uintptr_t buffer_addr) {
  const auto iter = task_addr_offset_.find(addr);
  if (iter != task_addr_offset_.end()) {
    for (const auto offset : iter->second) {
      auto &current_addr = *PtrToPtr<uint8_t, uintptr_t>(&args_info_[static_cast<size_t>(offset)]);
      if (current_addr != buffer_addr) {
        current_addr = buffer_addr;
        is_updated_ = true;
      }
    }
  }

  return SUCCESS;
}

/**
 * @ingroup ge
 * @brief Update task param to device.
 * @param [in] async_mode: true for asychronous mode.
 * @param [in] stream: Stream for asychronous update.
 * @return: 0 SUCCESS / others FAILED
 */
Status ZeroCopyTask::DistributeParam(const bool async_mode, rtStream_t stream) {
  if (!is_updated_) {
    return SUCCESS;
  }

  is_updated_ = false;
  GE_CHECK_NOTNULL(args_addr_);
  rtError_t rt_err = RT_ERROR_NONE;
  if (async_mode) {
    rt_err = rtMemcpyAsync(args_addr_, args_size_, args_info_.data(), args_info_.size(), RT_MEMCPY_HOST_TO_DEVICE_EX,
                           stream);
  } else {
    GE_BUILTIN_PREFETCH(args_addr_);
#ifdef ONLY_COMPILE_OPEN_SRC
    rt_err = rtMemcpy(args_addr_, args_size_, args_info_.data(), args_info_.size(), RT_MEMCPY_HOST_TO_DEVICE);
#else
    rt_err = rtMemcpyHostTask(args_addr_, args_size_, args_info_.data(), args_info_.size(), RT_MEMCPY_HOST_TO_DEVICE,
                              stream);
    if (rt_err == ACL_ERROR_RT_FEATURE_NOT_SUPPORT) {
      rt_err = rtMemcpy(args_addr_, args_size_, args_info_.data(), args_info_.size(), RT_MEMCPY_HOST_TO_DEVICE);
    }
#endif
  }

  if (rt_err != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpyAsync or rtMemcpy failed, size:%zu, ret:0x%X", args_size_, rt_err);
    GELOGE(RT_FAILED, "[Distribute][TaskParam] for %s failed, error = 0x%x", name_.c_str(), rt_err);
    return RT_ERROR_TO_GE_STATUS(rt_err);
  }

  GELOGD("[ZCPY] %s refresh task args success, args_addr: %p, size: %zu, args_info_: %p, length: %zu", name_.c_str(),
         args_addr_, args_size_, args_info_.data(), args_info_.size());
  return SUCCESS;
}
}  // namespace ge
