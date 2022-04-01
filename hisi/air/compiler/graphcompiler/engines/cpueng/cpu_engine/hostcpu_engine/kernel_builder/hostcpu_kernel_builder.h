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
#ifndef AICPU_HOST_CPU_KERNEL_BUILDER_H_
#define AICPU_HOST_CPU_KERNEL_BUILDER_H_

#include "common/kernel_builder/cpu_kernel_builder.h"

namespace aicpu {
using KernelBuilderPtr = std::shared_ptr<KernelBuilder>;

class HostCpuKernelBuilder : public CpuKernelBuilder {
 public:
  /**
   * Singleton access method, Thread safe
   * @return return unique instance in global scope
   */
  static KernelBuilderPtr Instance();

  /**
   * Destructor
   */
  virtual ~HostCpuKernelBuilder() = default;

 private:
  /**
   * Constructor
   */
  HostCpuKernelBuilder() = default;

  // Copy prohibited
  HostCpuKernelBuilder(const HostCpuKernelBuilder &hostcpu_kernel_builder) = delete;

  // Move prohibited
  HostCpuKernelBuilder(const HostCpuKernelBuilder &&hostcpu_kernel_builder) = delete;

  // Copy prohibited
  HostCpuKernelBuilder &operator=(
      const HostCpuKernelBuilder &hostcpu_kernel_builder) = delete;

  // Move prohibited
  HostCpuKernelBuilder &operator=(
      HostCpuKernelBuilder &&hostcpu_kernel_builder) = delete;

 private:
  // singleton instance
  static KernelBuilderPtr instance_;
};
}  // namespace aicpu
#endif  // AICPU_HOST_CPU_KERNEL_BUILDER_H_
