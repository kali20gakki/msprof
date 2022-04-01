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

#ifndef DAVINCI_HOST_CPU_OPTIMIZER_H_
#define DAVINCI_HOST_CPU_OPTIMIZER_H_

#include "common/optimizer/cpu_optimizer.h"

namespace aicpu {
using OptimizerPtr = std::shared_ptr<Optimizer>;

class HostCpuOptimizer : public CpuOptimizer {
 public:
  /**
   * get optimizer instance
   * @return ptr stored aicpu optimizer
   */
  static OptimizerPtr Instance();

  /**
   * Destructor
   */
  virtual ~HostCpuOptimizer() = default;

 private:
  /**
   * Constructor
   */
  HostCpuOptimizer() = default;

  // Copy prohibited
  HostCpuOptimizer(const HostCpuOptimizer& hostcpu_optimizer) = delete;

  // Move prohibited
  HostCpuOptimizer(const HostCpuOptimizer&& hostcpu_optimizer) = delete;

  // Copy prohibited
  HostCpuOptimizer& operator=(const HostCpuOptimizer& hostcpu_optimizer) = delete;

  // Move prohibited
  HostCpuOptimizer& operator=(HostCpuOptimizer&& hostcpu_optimizer) = delete;

 private:
  // singleton instance
  static OptimizerPtr instance_;
};
}  // namespace aicpu

#endif  // DAVINCI_HOSTCPU_OPTIMIZER_H
