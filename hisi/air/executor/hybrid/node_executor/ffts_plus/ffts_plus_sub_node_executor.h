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

#ifndef GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_NODE_EXECUTOR_H_
#define GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_NODE_EXECUTOR_H_

#include "hybrid/node_executor/node_executor.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task.h"

#include "common/plugin/plugin_manager.h"

namespace ge {
namespace hybrid {
class FftsPlusSubNodeExecutor : public NodeExecutor {
 public:
  Status LoadTask(const HybridModel &model, const NodePtr &node, NodeTaskPtr &task) const override;

  Status PrepareTask(NodeTask &task, TaskContext &context) const override {
    (void)task;
    (void)context;
    return SUCCESS;
  }

  Status Initialize() override;
  Status Finalize() override;
};
}  // namespace hybrid
} // namespace ge
#endif // GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_NODE_EXECUTOR_H_
