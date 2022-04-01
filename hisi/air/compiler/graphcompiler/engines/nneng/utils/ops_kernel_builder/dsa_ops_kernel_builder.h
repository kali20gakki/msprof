/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_UTILS_OPS_KERNEL_BUILDER_DSA_OPS_KERNEL_BUILDER_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_UTILS_OPS_KERNEL_BUILDER_DSA_OPS_KERNEL_BUILDER_H_

#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "common/opskernel/ops_kernel_builder.h"

namespace fe {
class DsaOpsKernelBuilder : public ge::OpsKernelBuilder {
 public:
  /**
   * Constructor for DsaOpsKernelBuilder
   */
  DsaOpsKernelBuilder() {};

  /**
   * Deconstruction for DsaOpsKernelBuilder
   */
  ~DsaOpsKernelBuilder() override {};

  /**
   * Initialization
   * @param options
   * @return Status SUCCESS / FAILED or others
   */
  Status Initialize(const std::map<std::string, std::string> &options) override;

  /**
   * Finalization
   * @return Status SUCCESS / FAILED or others
   */
  Status Finalize() override;

  /**
   * Calculate the running parameters for node
   * @param node node object
   * @return Status SUCCESS / FAILED or others
   */
  Status CalcOpRunningParam(ge::Node &node) override;

  /**
   * Generate task for node
   * @param node node object
   * @param context context object
   * @param tasks Task list
   * @return Status SUCCESS / FAILED or others
   */
  Status GenerateTask(const ge::Node &node, ge::RunContext &context, std::vector<domi::TaskDef> &tasks) override;
};
}  // namespace fe

#endif  // AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_UTILS_OPS_KERNEL_BUILDER_DSA_OPS_KERNEL_BUILDER_H_
