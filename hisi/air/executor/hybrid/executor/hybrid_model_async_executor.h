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

#ifndef GE_HYBRID_EXECUTOR_MODEL_HYBRID_MODEL_ASYNC_EXECUTOR_H_
#define GE_HYBRID_EXECUTOR_MODEL_HYBRID_MODEL_ASYNC_EXECUTOR_H_
#include <atomic>
#include <mutex>
#include <future>
#include "external/ge/ge_api_error_codes.h"
#include "external/ge/ge_api_types.h"
#include "common/dump/opdebug_register.h"
#include "graph/load/model_manager/data_inputer.h"
#include "common/dump/data_dumper.h"
#include "hybrid/executor/hybrid_model_executor.h"
#include "hybrid/executor/hybrid_model_pipeline_executor.h"
#include "runtime/stream.h"

namespace ge {
namespace hybrid {
class HybridModelAsyncExecutor {
 public:
  explicit HybridModelAsyncExecutor(HybridModel *const model);
  ~HybridModelAsyncExecutor();

  Status Init();

  Status Execute(const std::vector<DataBuffer> &inputs,
                 const std::vector<GeTensorDesc> &input_desc,
                 std::vector<DataBuffer> &outputs,
                 std::vector<GeTensorDesc> &output_desc);

  Status Execute(const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs);

  Status Start(const std::shared_ptr<ModelListener> &listener);

  void SetDeviceId(const uint32_t device_id);

  void SetModelId(const uint32_t model_id);

  Status Stop();

  Status EnqueueData(const std::shared_ptr<InputDataWrapper> &data);

  uint32_t GetDataInputerSize() const { return data_inputer_->Size(); }

  bool GetRunningFlag() const { return running_flag_; }

  void SetRunningFlag(const bool flag) { running_flag_ = flag; }

  const GraphExecutionContext *GeContext() const { return executor_->GetContext(); }

 private:
  Status InitInputDesc();

  Status RunInternal();

  Status SyncVarData() const;

  Status HandleResult(const Status exec_ret,
                      const uint32_t data_id,
                      HybridModelExecutor::ExecuteArgs &args,
                      OutputData *const output_data);

  Status CopyOutputs(HybridModelExecutor::ExecuteArgs &args, OutputData *const output_data,
                     std::vector<ge::Tensor> &outputs) const;
  Status BuildDeviceTensor(TensorValue &output_tensor, GeTensorDesc &ge_tensor_desc, const int64_t output_size,
                           std::vector<ge::Tensor> &outputs) const;

  Status OnComputeDone(const uint32_t data_index, const uint32_t result_code, std::vector<ge::Tensor> &outputs);

  Status PreRun(const InputData &current_data, HybridModelExecutor::ExecuteArgs &args);

  Status PrepareInputs(const InputData &current_data, HybridModelExecutor::ExecuteArgs &args);

  Status DumpOpDebug();

  static rtStream_t default_stream_;
  static uint32_t stream_ref_count_;
  static std::mutex mu_;

  HybridModel *model_;
  uint32_t device_id_ = 0U;
  uint32_t model_id_ = 0U;
  std::atomic_bool run_flag_;
  // check whether model is running with data
  bool running_flag_ = false;
  std::unique_ptr<DataInputer> data_inputer_;
  std::unique_ptr<HybridModelExecutor> executor_;
  std::unique_ptr<HybridModelPipelineExecutor> pipe_executor_;
  std::future<Status> future_;
  uint64_t iterator_count_ = 0U;

  rtStream_t stream_ = nullptr;
  bool own_stream_ = false;
  std::map<uint32_t, int64_t> input_sizes_;
  std::map<uint32_t, GeTensorDescPtr> input_tensor_desc_;
  std::vector<bool> is_input_dynamic_;
  std::shared_ptr<ModelListener> listener_;
  DataDumper data_dumper_;
  bool is_op_debug_reg_ = false;
  OpdebugRegister op_debug_register_;
  std::vector<TensorValue> output_cache_;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_EXECUTOR_MODEL_HYBRID_MODEL_ASYNC_EXECUTOR_H_
