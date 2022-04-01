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

#ifndef GE_HYBRID_EXECUTOR_HYBRID_MODEL_EXECUTOR_H_
#define GE_HYBRID_EXECUTOR_HYBRID_MODEL_EXECUTOR_H_
#include "common/thread_pool.h"
#include "graph/load/model_manager/data_inputer.h"
#include "hybrid/executor/hybrid_execution_context.h"
#include "hybrid/executor/callback_manager.h"
#include "hybrid/executor/subgraph_executor.h"

namespace ge {
namespace hybrid {
class HybridModelExecutor {
 public:
  struct ExecuteArgs {
    std::vector<TensorValue> inputs;
    std::vector<ConstGeTensorDescPtr> input_desc;
    std::vector<TensorValue> outputs;
    std::vector<ConstGeTensorDescPtr> output_desc;
    bool is_eos = false;
    int32_t num_loops = 10;
  };

  HybridModelExecutor(HybridModel *const model, const uint32_t device_id, const rtStream_t stream,
                      ThreadPool *const thread_pool = nullptr);

  ~HybridModelExecutor();

  Status Init(CallbackManager *const callback_manager = nullptr);

  GraphExecutionContext* GetContext() {
    return &context_;
  }

  Status Execute(ExecuteArgs &args);

  Status ExecuteForSingleOp(const HybridModelExecutor::ExecuteArgs &args);

 private:
  Status ExecuteGraphInternal(SubgraphExecutor &executor, ExecuteArgs &args);
  Status Cleanup();
  Status InitExecutionContext(CallbackManager *const callback_manager = nullptr);
  static Status ResetExecutionContext(GraphExecutionContext &context);
  static Status CheckInputShapeByShapeRange(const GraphItem *const graph_item,
                                            const HybridModelExecutor::ExecuteArgs &args);

  HybridModel *model_;
  uint32_t device_id_;
  rtStream_t stream_;
  GraphExecutionContext context_;
  SubgraphExecutor executor_;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_EXECUTOR_HYBRID_MODEL_EXECUTOR_H_
