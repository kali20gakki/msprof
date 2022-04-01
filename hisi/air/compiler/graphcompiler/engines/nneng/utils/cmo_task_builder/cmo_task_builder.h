/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_BUILDER_H_
#define AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_BUILDER_H_

#include <vector>
#include "common/comm_log.h"
#include "cmo_task/generate_cmo_task_base.h"
#include "cmo_task/generate_cmo_barrier_task.h"
#include "cmo_task/generate_cmo_invalid_task.h"
#include "cmo_task/generate_cmo_prefetch_task.h"
#include "cmo_task/generate_cmo_writeback_task.h"
#include "graph/node.h"
#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "proto/task.pb.h"

namespace fe {
class CMOTaskBuilder;

using CMOTaskBuilderPtr = std::shared_ptr<CMOTaskBuilder>;
using GenerateCMOTaskBasePtr = std::shared_ptr<GenerateCMOTaskBase>;
using GenerateCMOBarrierTaskPtr = std::shared_ptr<GenerateCMOBarrierTask>;
using GenerateCMOInvalidTaskPtr = std::shared_ptr<GenerateCMOInvalidTask>;
using GenerateCMOPrefetchTaskPtr = std::shared_ptr<GenerateCMOPrefetchTask>;
using GenerateCMOWritebackTaskPtr = std::shared_ptr<GenerateCMOWritebackTask>;

class CMOTaskBuilder {
 public:
  CMOTaskBuilder();
  virtual ~CMOTaskBuilder();

  Status GenerateCMOTask(const ge::Node &node, std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                         TaskBuilderContext context, const bool &pre_cmo_task);
 private:
  GenerateCMOTaskBasePtr GetCMOTaskType(const ge::Node &node, TaskBuilderContext &context,
                                        const std::string &task_type, const bool &pre_cmo_task);
  std::string GetPosition(const bool &pre_task) const;
  GenerateCMOBarrierTaskPtr generate_cmo_barrier_task_ptr_;
  GenerateCMOInvalidTaskPtr generate_cmo_invalid_task_ptr_;
  GenerateCMOPrefetchTaskPtr generate_cmo_prefetch_task_ptr_;
  GenerateCMOWritebackTaskPtr generate_cmo_writeback_task_ptr_;
};
}  // namespace fe
#endif  // AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_BUILDER_H_
