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

#ifndef GE_HYBRID_KERNEL_AICORE_TASK_BUILDER_H_
#define GE_HYBRID_KERNEL_AICORE_TASK_BUILDER_H_

#include <vector>
#include <string>
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/attr_utils.h"
#include "graph/op_kernel_bin.h"
#include "proto/task.pb.h"
#include "hybrid/model/hybrid_model.h"

namespace ge {
namespace hybrid {
class AiCoreNodeTask;

class AiCoreTaskBuilder {
 public:
  AiCoreTaskBuilder(const NodePtr &node, const std::vector<domi::TaskDef> &task_defs, const HybridModel &model,
                    AiCoreNodeTask &aicore_node_task);
  ~AiCoreTaskBuilder() = default;

  Status BuildTask();
  Status LoadAicpuTask();
  Status LoadFusedTask();

 private:
  bool ExpectAtomicAddrCleanTask() const;
  Status InitTaskDef();
  Status LoadAtomicWorkspace();

  NodePtr node_;
  OpDescPtr op_desc_;
  const std::vector<domi::TaskDef> &task_defs_;
  const HybridModel &model_;
  AiCoreNodeTask &aicore_node_task_;
  std::vector<domi::TaskDef> aicore_task_defs_;
  std::vector<const domi::TaskDef *> aicpu_task_defs_;  // aicpu executor references taskdef during execution.
};
}  // namespace hybrid
}  // namespace ge
#endif //GE_HYBRID_KERNEL_AICORE_TASK_BUILDER_H_
