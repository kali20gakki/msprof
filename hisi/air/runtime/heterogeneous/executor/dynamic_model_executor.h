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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_DYNAMIC_MODEL_EXECUTOR_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_DYNAMIC_MODEL_EXECUTOR_H_

#include <thread>
#include <vector>
#include "ge/ge_api_error_codes.h"
#include "common/blocking_queue.h"
#include "common/model/ge_root_model.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "executor/ge_executor.h"
#include "executor/cpu_sched_model.h"

namespace ge {
class DynamicModelExecutor {
 public:
  explicit DynamicModelExecutor(bool is_host);
  virtual ~DynamicModelExecutor();
  Status LoadModelWithQueue(const GeRootModelPtr &root_model,
                            const std::vector<uint32_t> &input_queues,
                            const std::vector<uint32_t> &output_queues);

  Status ExecuteAsync(const std::function<void(Status)> &callback);
  Status ExecuteInternal();
  void UnloadModel();

 private:
  void Run();
  void Stop();
  virtual Status DoLoadModel(const shared_ptr<GeRootModel> &root_model);
  virtual Status DoExecuteModel(const RunModelData &inputs, RunModelData &outputs);
  Status ParseModelDesc(const GeRootModelPtr &root_model);
  Status LoadWithAiCpuSd();
  static Status UpdateTensorDesc(const RuntimeTensorDesc &runtime_tensor_desc, GeTensorDesc &tensor_desc);
  static Status UpdateRuntimeTensorDesc(const GeTensorDesc &tensor_desc, RuntimeTensorDesc &runtime_tensor_desc);
  static Status UpdateRuntimeShape(const GeShape &shape, int64_t (&shape_buffer)[33]);
  Status PrepareInputs(RunModelData &model_inputs);
  Status PrepareOutputs(RunModelData &model_outputs);
  Status UpdateOutputs(RunModelData &model_outputs);
  static Status GetOutputTensorSize(const GeTensorDesc &tensor_desc, int64_t &tensor_size);

  std::thread run_thread_;
  BlockingQueue<std::function<void(Status)>> task_queue_;
  size_t num_inputs_ = 0U;
  size_t num_outputs_ = 0U;
  GeExecutor ge_executor_;
  std::vector<uint32_t> input_queue_ids_;
  std::vector<uint32_t> output_queue_ids_;
  std::vector<void *> input_mbuf_addresses_;
  std::vector<void *> output_mbuf_addresses_;
  std::vector<GeTensorDesc> input_tensor_descs_;
  std::vector<GeTensorDesc> output_tensor_descs_;
  std::vector<int64_t> output_tensor_sizes_;
  std::vector<RuntimeTensorDesc> output_runtime_tensor_descs_;
  std::vector<bool> is_input_dynamic_;
  std::vector<bool> is_output_dynamic_;
  uint32_t model_id_ = UINT32_MAX;
  int32_t device_id_ = 0;
  bool is_host_ = false;
  rtStream_t stream_ = nullptr;
  rtContext_t rt_context_ = nullptr;
  CpuSchedModel model_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_DYNAMIC_MODEL_EXECUTOR_H_
