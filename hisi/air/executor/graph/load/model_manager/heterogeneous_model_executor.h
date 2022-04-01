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
#ifndef EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_HETEROGENEOUS_MODEL_EXECUTOR_H_
#define EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_HETEROGENEOUS_MODEL_EXECUTOR_H_

#include <thread>
#include "common/blocking_queue.h"
#include "exec_runtime/deploy/exchange_service.h"
#include "framework/common/ge_types.h"
#include "graph/ge_tensor.h"
#include "exec_runtime/deploy/model_deployer.h"

namespace ge {
class HeterogeneousModelExecutor {
 public:
  ///
  /// @ingroup ge
  /// @brief Constructor
  /// @param [in]  root_model        Root model
  /// @param [in]  deploy_result     Model deployment info
  ///
  HeterogeneousModelExecutor(const PneModelPtr &root_model, const DeployResult &deploy_result);

  ~HeterogeneousModelExecutor();

  ///
  /// @ingroup ge
  /// @brief Initialize executor
  /// @return SUCCESS success / others failure
  ///
  Status Initialize();

  ///
  /// @ingroup ge
  /// @brief Execute model
  /// @param [in]  inputs     inputs
  /// @param [out] outputs    outputs
  /// @return SUCCESS success / others failure
  ///
  Status Execute(const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs);

  ///
  /// @ingroup ge
  /// @brief Execute model async
  /// @param [in]  inputs     inputs
  /// @param [out] callback   callback function
  /// @return SUCCESS success / others failure
  ///
  Status ExecuteAsync(const std::vector<Tensor> &inputs, const RunAsyncCallback &callback);

  ///
  /// @ingroup ge
  /// @brief create model std::thread,
  /// @brief start to execute Model
  /// @return Status create model thread and execute result
  ///
  Status ModelRunStart();

  ///
  /// @ingroup ge
  /// @brief call API provided by data inputer and destroy model Thread
  /// @return Status Destroy result
  ///
  Status ModelRunStop();

  ///
  /// @ingroup ge
  /// @brief Set listener
  /// @param [in]  listener    listener
  ///
  void SetListener(const std::shared_ptr<ModelListener> &listener);

  ///
  /// @ingroup ge
  /// @brief Get Id of the DeployedModel
  /// @param [in]  listener    listener
  ///
  uint32_t GetDeployedModelId() const;

  void SetModelId(const uint32_t model_id);

  void SetDeviceId(const uint32_t device_id);

 private:
  struct RunAsyncRequest {
    RunAsyncCallback callback;
  };

  Status WrapSingleModel();
  Status ParseInputTensorInfo();
  Status ParseOutputTensorInfo();
  Status BuildInputTensorDescMapping(std::map<std::string, GeTensorDescPtr> &mapping);
  Status BuildOutputTensorDescMapping(std::map<std::string, GeTensorDescPtr> &mapping);
  Status SetTensorInfo(std::map<std::string, GeTensorDescPtr> &mapping, const std::vector<std::string> &queue_names,
                       const std::vector<bool> &is_no_tiling, const bool is_input);
  Status EnqueueInputTensors(const std::vector<GeTensor> &inputs);
  Status DequeueOutputTensors(std::vector<GeTensor> &outputs);
  Status ValidateInputTensors(const vector<GeTensor> &inputs);
  void Run();

  PneModelPtr root_model_;
  bool is_dynamic_ = false;
  uint32_t deployed_model_id_;
  uint32_t device_id_ = 0U;
  uint32_t model_id_ = 0U;
  ExchangeService *exchange_service_ = nullptr;
  std::vector<uint32_t> input_queue_ids_;
  std::vector<uint32_t> control_input_queue_ids_;
  std::vector<uint32_t> output_queue_ids_;
  std::shared_ptr<ModelListener> listener_;
  std::vector<GeTensorDescPtr> input_tensor_desc_;
  std::vector<GeTensorDescPtr> output_tensor_desc_;
  std::vector<bool> input_is_no_tiling_;
  std::vector<bool> output_is_no_tiling_;
  std::vector<int64_t> input_tensor_sizes_;
  std::vector<int64_t> output_tensor_sizes_;
  std::vector<int64_t> input_tensor_raw_sizes_;
  std::vector<int64_t> output_tensor_raw_sizes_;
  std::mutex mu_;
  std::thread run_thread_;
  std::atomic_bool run_flag_{false};
  BlockingQueue<std::shared_ptr<RunAsyncRequest>> input_queue_;
};
}  // namespace ge
#endif  // EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_HETEROGENEOUS_MODEL_EXECUTOR_H_
