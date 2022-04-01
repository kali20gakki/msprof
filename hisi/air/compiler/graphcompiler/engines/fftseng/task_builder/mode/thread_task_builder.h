/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGINE_TASK_BUILDER_MODE_THREAD_TASK_BUILDER_H_
#define FFTS_ENGINE_TASK_BUILDER_MODE_THREAD_TASK_BUILDER_H_

#include <runtime/rt.h>
#include "common/opskernel/ops_kernel_builder.h"
#include "graph/compute_graph.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "task_builder/fftsplus_task_builder.h"
#include "fftseng/task_builder/thread_ctx/aic_aiv_auto_task_builder.h"
#include "fftseng/task_builder/thread_ctx/mix_aic_aiv_auto_task_builder.h"

#include "fftseng/task_builder/data_ctx/prefetch_auto_task_builder.h"
#include "fftseng/task_builder/data_ctx/out_auto_task_builder.h"
#include "fftseng/task_builder/data_ctx/prefetch_dynamic_task_builder.h"
#include "fftseng/task_builder/data_ctx/out_dynamic_task_builder.h"
#include "task_builder/data_ctx/prefetch_manual_task_builder.h"
#include "task_builder/data_ctx/out_manual_task_builder.h"
#include "task_builder/thread_ctx/aic_aiv_manual_task_builder.h"
#include "task_builder/thread_ctx/aic_aiv_dynamic_task_builder.h"
#include "task_builder/thread_ctx/mix_aic_aiv_manual_task_builder.h"
#include "task_builder/thread_ctx/collection_ops_manual_task_builder.h"
#include "task_builder/thread_ctx/runtime_ops_manual_task_builder.h"
#include "task_builder/thread_ctx/aicpu_manual_task_builder.h"
#include "task_builder/thread_ctx/aicpu_auto_task_builder.h"
#include "task_builder/mixl2_ctx/mix_l2_task_builder.h"
#include "task_builder/mode/data_task_builder.h"

namespace ffts {
using FFTSPlusTaskBuilderPtr = std::shared_ptr<FFTSPlusTaskBuilder>;
using AICAIVTaskBuilderPtr = std::shared_ptr<AICAIVTaskBuilder>;
using AICAIVAutoTaskBuilderPtr = std::shared_ptr<AICAIVAutoTaskBuilder>;
using MixAICAIVTaskBuilderPtr = std::shared_ptr<MixAICAIVTaskBuilder>;
using AICAIVDynamicTaskBuilderPtr = std::shared_ptr<AICAIVDynamicTaskBuilder>;
using MixL2TaskBuilderPtr = std::shared_ptr<MixL2TaskBuilder>;
using MixAICAIVAutoTaskBuilderPtr = std::shared_ptr<MixAICAIVAutoTaskBuilder>;
using CollectionOpsTaskBuilderPtr = std::shared_ptr<CollectionOpsTaskBuilder>;
using AicpuTaskBuilderPtr = std::shared_ptr<AicpuTaskBuilder>;
using AicpuAutoTaskBuilderPtr = std::shared_ptr<AicpuAutoTaskBuilder>;
using RuntimeOpsTaskBuilderPtr = std::shared_ptr<RuntimeOpsTaskBuilder>;

class TheadTaskBuilder {
 public:
  TheadTaskBuilder();
  virtual ~TheadTaskBuilder();

  virtual Status Initialize() = 0;

  virtual Status GenFftsPlusContextId(ge::ComputeGraph &sgt_graph, std::vector<ge::NodePtr> &sub_graph_nodes,
                                      uint64_t &ready_context_num, uint64_t &total_context_number) = 0;

  virtual Status GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes,
                                      domi::TaskDef &task_def) = 0;
  void SetModeType(const ModeType &type);
 protected:
  Status GetNodeContextTypeByNode(const ge::NodePtr &node, TaskBuilderType &task_builder_type) const;

  FFTSPlusTaskBuilderPtr GetTaskBuilder(TaskBuilderType task_builder_type);

  Status GenerateDataTaskDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def,
                             const ModeType &mode_type) const;

  bool IsNoCtx(const ge::NodePtr &node);

  const std::unordered_set<std::string> NO_NEED_GEN_TASK_OP_TYPE = {"Data", "NetOutput", "Variable", "Const",
                                                                    "Constant", "PhonyConcat"};

  ModeType mode_type_{ModeType::MANUAL_MODE_TYPE};

  AICAIVTaskBuilderPtr aic_aiv_task_builder_ptr_;
  AICAIVAutoTaskBuilderPtr aic_aiv_auto_task_builder_ptr_;
  MixAICAIVTaskBuilderPtr mix_aic_aiv_task_builder_ptr_;
  MixAICAIVAutoTaskBuilderPtr mix_aic_aiv_auto_task_builder_ptr_;
  AICAIVDynamicTaskBuilderPtr aic_aiv_dynamic_task_builder_ptr_;
  MixL2TaskBuilderPtr mix_L2_task_builder_ptr_;
  CollectionOpsTaskBuilderPtr collection_ops_task_builder_ptr_;
  AicpuTaskBuilderPtr aicpu_task_builder_ptr_;
  AicpuAutoTaskBuilderPtr aicpu_auto_task_builder_ptr_;
  RuntimeOpsTaskBuilderPtr runtime_ops_task_builder_ptr_;

};
}  // namespace ffts
#endif  // FFTS_ENGINE_TASK_BUILDER_MODE_THREAD_TASK_BUILDER_H_
