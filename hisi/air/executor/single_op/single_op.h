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

#ifndef GE_SINGLE_OP_SINGLE_OP_H_
#define GE_SINGLE_OP_SINGLE_OP_H_

#include <cstdint>
#include <memory>
#include <queue>

#include "framework/executor/ge_executor.h"
#include "single_op/task/op_task.h"
#include "hybrid/executor/hybrid_model_executor.h"
#include "graph/object_pool.h"

namespace ge {
constexpr uint64_t kFuzzDeviceBufferSize = 1U * 1024U * 1024U;

class SingleOp {
 public:
  SingleOp(StreamResource *const stream_resource, std::mutex *const stream_mutex, const rtStream_t stream);
  ~SingleOp();

  Status ExecuteAsync(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  int64_t GetProfilingNodeIndex() const noexcept;

 private:
  Status ValidateArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  Status UpdateArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  Status GetArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  bool CheckHostMemInputOptimization(const std::vector<DataBuffer> &input_buffers) const;

  friend class SingleOpModel;
  StreamResource *stream_resource_ = nullptr;
  std::mutex *stream_mutex_;
  rtStream_t stream_ = nullptr;
  std::vector<const void *> input_addr_list_;
  std::vector<size_t> input_sizes_;
  std::vector<const void *> output_addr_list_;
  std::vector<size_t> output_sizes_;
  std::vector<uintptr_t> args_;

  std::vector<std::unique_ptr<OpTask>> tasks_;
  std::vector<std::vector<uintptr_t *>> arg_table_;
  std::unique_ptr<SingleOpModelParam> running_param_;
  std::unique_ptr<hybrid::HybridModel> hybrid_model_;
  std::unique_ptr<hybrid::HybridModelExecutor> hybrid_model_executor_;
  std::vector<GeTensorDesc> inputs_desc_;
  int64_t profiling_node_type_index_;
  std::vector<NodePtr> node_with_hostmem_;
};

class DynamicSingleOp {
 public:
  DynamicSingleOp(ObjectPool<GeTensor> *const tensor_pool, const uintptr_t resource_id,
                  std::mutex *const stream_mutex, const rtStream_t stream);
  ~DynamicSingleOp() = default;

  Status ExecuteAsync(const std::vector<GeTensorDesc> &input_desc,
                      const std::vector<DataBuffer> &input_buffers,
                      std::vector<GeTensorDesc> &output_desc,
                      std::vector<DataBuffer> &output_buffers);

  int64_t GetProfilingNodeIndex() const noexcept;
 private:
  friend class SingleOpModel;
  Status ValidateParams(const std::vector<GeTensorDesc> &input_desc,
                        const std::vector<DataBuffer> &inputs,
                        const std::vector<GeTensorDesc> &output_desc,
                        const std::vector<DataBuffer> &outputs) const;
  Status SetHostTensorValue(const std::vector<std::pair<size_t, uint64_t>> &inputs_size,
                            const std::vector<GeTensorDesc> &input_desc,
                            const std::vector<DataBuffer> &input_buffers);

  Status SetHostTensorValue(const std::vector<GeTensorDesc> &input_desc,
                            const std::vector<DataBuffer> &input_buffers);

  bool CheckHostMemInputOptimization(const std::vector<DataBuffer> &input_buffers);
  void InjectRuntimeContext();
  std::unique_ptr<OpTask> op_task_;
  std::unique_ptr<hybrid::HybridModel> hybrid_model_;
  std::unique_ptr<hybrid::HybridModelExecutor> hybrid_model_executor_;
  std::map<int32_t, int64_t> hostmem_node_id_map_;
  std::map<int32_t, std::pair<int32_t, int32_t>> input_node_anchor_map_;
  std::vector<NodePtr> node_with_hostmem_;

  ObjectPool<GeTensor> *tensor_pool_{nullptr};
  int64_t profiling_node_type_index_ = -1;
  uintptr_t resource_id_ = 0U;
  std::mutex *stream_mutex_;
  rtStream_t stream_ = nullptr;
  size_t num_inputs_ = 0U;
  size_t num_outputs_ = 0U;
  ComputeGraphPtr compute_graph_;
  std::queue<std::unique_ptr<GeTensor>> shared_tensors_;
  RuntimeInferenceContext runtime_context_;
};
}  // namespace ge
#endif  // GE_SINGLE_OP_SINGLE_OP_H_
