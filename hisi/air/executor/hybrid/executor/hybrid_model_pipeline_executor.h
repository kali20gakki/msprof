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

#ifndef GE_HYBRID_EXECUTOR_HYBRID_MODEL_PIPELINE_EXECUTOR_H_
#define GE_HYBRID_EXECUTOR_HYBRID_MODEL_PIPELINE_EXECUTOR_H_

#include "common/blocking_queue.h"
#include "common/thread_pool.h"
#include "hybrid/executor/hybrid_execution_context.h"
#include "hybrid/executor/rt_callback_manager.h"
#include "hybrid/executor/subgraph_executor.h"
#include "hybrid/executor/hybrid_model_executor.h"

namespace ge {
namespace hybrid {

struct PipeExecutionConfig {
  uint32_t device_id;
  rtContext_t rt_context;
  int32_t num_executors;
  int32_t num_stages;
  int64_t iteration_end;
};

class StageSubject {
 public:
  Status Await(const int32_t stage);
  void Release(const int32_t stage);

 private:
  class Cond {
   public:
    void Release();
    Status Await();
   private:
    std::mutex cond_mu_;
    std::condition_variable cv_;
    bool first_exe_ = true;
    bool is_released_ = false;
  };
  Cond &GetSubject(const int32_t stage);
  std::mutex mu_;
  std::unordered_map<int32_t, Cond> subjects_;
};

class StageExecutor {
 public:
  struct StageTask {
    rtEvent_t event = nullptr;
    int32_t stage = 0;
    int64_t iteration = 0;
  };

  StageExecutor(const int32_t id, HybridModel *const model, PipeExecutionConfig *const config,
                StageSubject *const stage_subject);

  ~StageExecutor();

  Status Init();

  void Reset();

  Status Start(const std::vector<TensorValue> &inputs, const std::vector<ConstGeTensorDescPtr> &input_desc,
               const int32_t iteration_count);

  Status SetInputs(const std::vector<TensorValue> &inputs, const std::vector<ConstGeTensorDescPtr> &input_desc) const;

  Status ExecuteAsync(const StageTask &args);

  Status GetOutputs(std::vector<TensorValue> &outputs, std::vector<ConstGeTensorDescPtr> &output_desc) const;

  Status Synchronize();

  void SetNext(StageExecutor *const next_executor) { next_executor_ = next_executor; }

 private:
  friend class HybridModelPipelineExecutor;
  static Status ResetExecutionContext(GraphExecutionContext &context);
  Status InitExecutionContext();

  int32_t id_;
  HybridModel *model_;

  PipeExecutionConfig *pipe_config_;
  StageSubject *stage_subject_;
  BlockingQueue<StageTask> task_queue_;
  std::unique_ptr<SubgraphExecutor> root_graph_executor_;
  GraphExecutionContext context_;
  StageExecutor *next_executor_ = nullptr;

  rtStream_t stream_ = nullptr;
  rtStream_t hccl_stream_ = nullptr;
};

class HybridModelPipelineExecutor {
 public:
  HybridModelPipelineExecutor(HybridModel *const model, const uint32_t device_id);
  ~HybridModelPipelineExecutor();
  Status Init();
  Status InitStageExecutors();
  Status Execute(HybridModelExecutor::ExecuteArgs &args);

 private:
  HybridModel *model_;
  uint32_t device_id_;

  std::vector<std::unique_ptr<StageExecutor>> stage_executors_;
  StageSubject stage_subject_;
  PipeExecutionConfig config_;
  GraphExecutionContext context_;
  int64_t iteration_ = 0;
};
}  // namespace hybrid
}  // namespace ge

#endif  // GE_HYBRID_EXECUTOR_HYBRID_MODEL_PIPELINE_EXECUTOR_H_
