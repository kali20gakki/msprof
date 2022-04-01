/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef AICPU_AICPU_CUST_KERNEL_INFO_H_
#define AICPU_AICPU_CUST_KERNEL_INFO_H_

#include "common/aicpu_ops_kernel_info_store/kernel_info.h"

namespace aicpu {
using KernelInfoPtr = std::shared_ptr<KernelInfo>;

class AicpuCustKernelInfo : public KernelInfo {
 public:
  /**
   * Singleton access method, Thread safe
   * @return return unique instance in global scope
   */
  static KernelInfoPtr Instance();

  virtual ~AicpuCustKernelInfo() = default;

 protected:
  /**
   * Read json file in specified path(based on source file's current path) e.g.
   * ops_data/aicpu_kernel.json
   * @return whether read file successfully
   */
  bool ReadOpInfoFromJsonFile() override;

 private:
  /**
   * Constructor
   */
  AicpuCustKernelInfo() = default;

  // Copy prohibited
  AicpuCustKernelInfo(const AicpuCustKernelInfo& aicpu_cust_kernel_info) =
      delete;

  // Move prohibited
  AicpuCustKernelInfo(const AicpuCustKernelInfo&& aicpu_cust_kernel_info) =
      delete;

  // Copy prohibited
  AicpuCustKernelInfo& operator=(
      const AicpuCustKernelInfo& aicpu_cust_kernel_info) = delete;

  // Move prohibited
  AicpuCustKernelInfo& operator=(AicpuCustKernelInfo&& aicpu_cust_kernel_info) =
      delete;

 private:
  // singleton instance
  static KernelInfoPtr instance_;
};
}  // namespace aicpu
#endif  // AICPU_AICPU_CUST_KERNEL_INFO_H_
