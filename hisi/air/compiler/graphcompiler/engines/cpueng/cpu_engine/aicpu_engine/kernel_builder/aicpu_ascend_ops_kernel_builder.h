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
#ifndef AICPU_AICPU_ASCEND_OPS_KERNEL_BUILDER_H_
#define AICPU_AICPU_ASCEND_OPS_KERNEL_BUILDER_H_

#include <memory>
#include <vector>
#include "common/aicpu_ops_kernel_builder/kernel_builder.h"
#include "common/opskernel/ops_kernel_builder.h"
#include "common/util/constant.h"

namespace aicpu {
using KernelBuilderPtr = std::shared_ptr<KernelBuilder>;

class AicpuAscendOpsKernelBuilder : public ge::OpsKernelBuilder {
 public:
  /**
   * Contructor
   */
  explicit AicpuAscendOpsKernelBuilder() : engine_name_(kAicpuEngine) {}

  /**
   * Destructor
   */
  virtual ~AicpuAscendOpsKernelBuilder() = default;

  /**
   * Initialize related resources of the aicpu kernelinfo store
   * @return status whether this operation success
   */
  ge::Status Initialize(
      const std::map<std::string, std::string> &options) override;

  /**
   * Release related resources of the aicpu kernelinfo store
   * @return status whether this operation success
   */
  ge::Status Finalize() override;

  /**
   * Calc the running size of Operator,then GE will alloc the memsize from
   * runtime
   * @param node Node information, return task_memsize in node's attr
   * @return status whether this operation success
   */
  ge::Status CalcOpRunningParam(ge::Node &node) override;

  /**
   * Copy the data from host to device, then address is allocated by GE, then
   * invoked the runtime's interface to generate the task
   * @param node Node information
   * @param runContext Run context from Ge
   * @return task memsize in node
   */
  ge::Status GenerateTask(const ge::Node &node, ge::RunContext &context,
                          std::vector<domi::TaskDef> &tasks) override;

  // Copy prohibited
  AicpuAscendOpsKernelBuilder(const AicpuAscendOpsKernelBuilder
                                  &aicpu_ascend_ops_kernel_builder) = delete;

  // Move prohibited
  AicpuAscendOpsKernelBuilder(const AicpuAscendOpsKernelBuilder
                                  &&aicpu_ascend_ops_kernel_builder) = delete;

  // Copy prohibited
  AicpuAscendOpsKernelBuilder &operator=(
      const AicpuAscendOpsKernelBuilder &aicpu_ascend_ops_kernel_builder) =
      delete;

  // Move prohibited
  AicpuAscendOpsKernelBuilder &operator=(
      AicpuAscendOpsKernelBuilder &&aicpu_ascend_ops_kernel_builder) = delete;

 private:
  std::string engine_name_;

  // kernel_util map
  std::map<std::string, KernelBuilderPtr> kernel_builder_map_;
};
}  // namespace aicpu

#endif  // AICPU_AICPU_ASCEND_OPS_KERNEL_BUILDER_H_
