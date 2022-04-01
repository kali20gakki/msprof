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

#ifndef GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_NODE_TASK_H_
#define GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_NODE_TASK_H_

#include "hybrid/node_executor/node_executor.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_subgraph_executor.h"
#include "hybrid/model/hybrid_model.h"
#include "graph/load/model_manager/tbe_kernel_handle.h"
#include "register/op_tiling.h"
#include "runtime/rt.h"

namespace ge {
namespace hybrid {
class FftsPlusNodeTask : public NodeTask {
 public:
  explicit FftsPlusNodeTask(const GraphItem *const graph_item) : NodeTask(), graph_item_(graph_item) {}
  ~FftsPlusNodeTask() override;

  /**
   * @ingroup ge
   * @param task_context: instance of TaskContext
   * @return SUCCESS / other error code.
   */
  Status Init(TaskContext &context) override;

  /**
   * @ingroup ge
   * @brief Load dynamic shape model for FFTS Plus Graph.
   * @return SUCCESS / other error code.
   */
  Status Load(const HybridModel &model, const NodePtr &node, const ComputeGraphPtr &subgraph);

  /**
   * @ingroup ge
   * @brief Get runtime task Start PC and prefetch count.
   * @return SUCCESS / other error code.
   */
  Status GetAddrAndPrefCnt(const OpDesc &op_desc, const uint64_t bin_idx,
                           uintptr_t &start_pc, uint32_t &prefetch_cnt) const {
    return bin_kernel_handle_.GetAddrAndPrefCnt(op_desc, bin_idx, start_pc, prefetch_cnt);
  }

  std::string GetAiCpuArgData(const OpDesc &op_desc) const {
    const std::map<int64_t, std::string>::const_iterator it = aicpu_arg_data_.find(op_desc.GetId());
    return (it != aicpu_arg_data_.cend()) ? it->second : "";
  }

  std::string GetAiCpuExtInfo(const OpDesc &op_desc) const {
    const std::map<int64_t, std::string>::const_iterator it = aicpu_ext_info_.find(op_desc.GetId());
    return (it != aicpu_ext_info_.cend()) ? it->second : "";
  }

  Status UpdateArgs(TaskContext &context) override {
    (void)context;
    return SUCCESS;
  }

  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

 private:
  Status Callback(const std::function<void()> &done_callback);
  Status SetSaveAicpuCtxHandle(const OpDescPtr &op_desc, const domi::aicpuKernelDef &kernel_def);

  const GraphItem *graph_item_{nullptr};
  ComputeGraphPtr subgraph_;
  std::unique_ptr<FftsPlusSubgraphExecutor> subgraph_executor_;

  std::vector<void *> ext_args_;
  TBEKernelHandle bin_kernel_handle_;

  friend class FftsPlusSubTask;
  int32_t device_id_{-1};
  rtFftsPlusTaskInfo_t ffts_plus_task_info_{nullptr, nullptr, 0U};
  std::map<int64_t, std::string> aicpu_arg_data_;
  std::map<int64_t, std::string> aicpu_ext_info_;
};
} // namespace hybrid
} // namespace ge
#endif // GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_NODE_TASK_H_
