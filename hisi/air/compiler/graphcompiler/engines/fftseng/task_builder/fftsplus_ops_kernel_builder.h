/**
 * @file ffts_plus_ops_kernel_builder.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * @brief aicore param calculator and genreate task
 *
 * @version 1.0
 *
 */

#ifndef FFTS_ENGIINE_TASK_BUILDER_FFTSPLUS_OPS_KERNEL_BUILDER_H_
#define FFTS_ENGIINE_TASK_BUILDER_FFTSPLUS_OPS_KERNEL_BUILDER_H_

#include "inc/ffts_error_codes.h"
#include "common/opskernel/ops_kernel_builder.h"
#include "task_builder/mode/data_task_builder.h"
#include "task_builder/data_ctx/cache_persistent_manual_task_builder.h"
#include "task_builder/mode/thread_task_builder.h"
#include "task_builder/mode/manual/manual_thread_task_builder.h"
#include "task_builder/mode/mixl2/mixl2_mode_task_builder.h"
#include "task_builder/mode/auto/auto_thread_task_builder.h"

namespace ffts {
using TheadTaskBuilderPtr = std::shared_ptr<TheadTaskBuilder>;
using ManualTheadTaskBuilderPtr = std::shared_ptr<ManualTheadTaskBuilder>;
using AutoTheadTaskBuilderPtr = std::shared_ptr<AutoTheadTaskBuilder>;
using Mixl2ModeTaskBuilderPtr = std::shared_ptr<Mixl2ModeTaskBuilder>;

class FFTSPlusOpsKernelBuilder : public ge::OpsKernelBuilder {
 public:
  /**
   * Constructor for AICoreOpsKernelBuilder
   */
  FFTSPlusOpsKernelBuilder();

  /**
   * Deconstruction for AICoreOpsKernelBuilder
   */
  ~FFTSPlusOpsKernelBuilder() override;

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
  Status GenerateTask(const ge::Node &node, ge::RunContext &context, std::vector<domi::TaskDef> &task_defs) override;

 private:
  TheadTaskBuilderPtr GetNormBuilder(const ge::Node &node, ge::ComputeGraphPtr &sgt_graph,
                                     domi::TaskDef &task_def, uint64_t &ready_num, uint64_t &total_num);
  Status GenPersistentContext(const ge::Node &node, uint64_t &ready_context_num, uint64_t &total_context_number,
                              domi::TaskDef &task_def) const;
  TheadTaskBuilderPtr GetFftsPlusMode(const ge::ComputeGraph &sgt_graph);
  Status GenerateAutoThreadTask();
  Status GenerateManualThreadTask();
  Status WritePrefetchBitmapToFirst64Bytes(domi::FftsPlusTaskDef *ffts_plus_task_def);
  Status GenSubGraphSqeDef(domi::TaskDef &task_def, const uint64_t &ready_context_num) const;

 private:
  ManualTheadTaskBuilderPtr manual_thread_task_builder_ptr_;
  AutoTheadTaskBuilderPtr auto_thread_task_builder_ptr_;
  Mixl2ModeTaskBuilderPtr mixl2_mode_task_builder_ptr_;
};
}  // namespace ffts
#endif  // FFTS_ENGIINE_TASK_BUILDER_FFTSPLUS_OPS_KERNEL_BUILDER_H_
