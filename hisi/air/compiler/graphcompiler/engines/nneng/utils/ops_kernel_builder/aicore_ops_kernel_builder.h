/**
 * @file aicore_ops_kernel_builder.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * @brief aicore param calculator and genreate task
 *
 * @version 1.0
 *
 */

#ifndef FUSION_ENGINE_UTILS_OPS_KERNEL_BUILDER_AICORE_OPS_KERNEL_BUILDER_H_
#define FUSION_ENGINE_UTILS_OPS_KERNEL_BUILDER_AICORE_OPS_KERNEL_BUILDER_H_

#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "common/opskernel/ops_kernel_builder.h"
#include "adapter/adapter_itf/task_builder_adapter.h"

namespace fe {

using FftsPlusCtxDefPtr = std::shared_ptr<domi::FftsPlusCtxDef>;

class AICoreOpsKernelBuilder : public ge::OpsKernelBuilder {
 public:
  /**
   * Constructor for AICoreOpsKernelBuilder
   */
  AICoreOpsKernelBuilder();

  /**
   * Deconstruction for AICoreOpsKernelBuilder
   */
  ~AICoreOpsKernelBuilder() override;

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
  Status GenContextArgs(const ge::Node &node, ge::RunContext &context, TaskArgs &args_info) const;
  Status FillContextData(const ge::Node &node, ge::RunContext &context,
                         domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def) const;
  Status GenerateMixL2Task(const ge::Node &node, ge::RunContext &context);
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_OPS_KERNEL_BUILDER_AICORE_OPS_KERNEL_BUILDER_H_
