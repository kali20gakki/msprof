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

#ifndef GE_GRAPH_EXECUTE_GRAPH_EXECUTE_H_
#define GE_GRAPH_EXECUTE_GRAPH_EXECUTE_H_

#include <cstdarg>

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "framework/common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "framework/common/ge_types.h"
#include "common/properties_manager.h"
#include "framework/common/string_util.h"
#include "framework/common/types.h"
#include "framework/common/util.h"
#include "external/ge/ge_api_types.h"
#include "graph/compute_graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/model.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
struct BufferInfo {
  void *addr;
  uint64_t size;

  bool operator==(const BufferInfo &other) const {
    return other.size == size;
  }
};

class GraphExecutor {
 public:
  GraphExecutor();

  virtual ~GraphExecutor();

  Status ExecuteGraph(const GraphId graph_id, const GeRootModelPtr &ge_root_model,
                      const std::vector<GeTensor> &input_tensor, std::vector<GeTensor> &output_tensor);

  Status ExecuteGraphAsync(const GraphId graph_id, const GeRootModelPtr &ge_root_model,
                           const std::vector<Tensor> &input_tensor, const RunAsyncCallback &callback);

  Status ExecuteGraphWithStream(const GraphId graph_id,
                                const rtStream_t stream,
                                const GeRootModelPtr &ge_root_model,
                                const std::vector<GeTensor> &input_tensor,
                                const std::vector<GeTensor> &output_tensor);

  static Status SetDynamicSize(const uint32_t model_id, const std::vector<uint64_t> &batch_num,
                               const int32_t dynamic_type);

  Status FreeExecuteMemory();

  static Status GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                       std::vector<InputOutputDescInfo> &output_desc);

  static Status GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                       std::vector<InputOutputDescInfo> &output_desc,
                                       std::vector<uint32_t> &input_formats,
                                       std::vector<uint32_t> &out_formats, const bool new_model_desc = false);

  static Status GetAippInfo(const uint32_t model_id, const uint32_t index, AippConfigInfo &aipp_info);

  static Status GetAippType(const uint32_t model_id, const uint32_t index, InputAippType &type, size_t &aipp_index);

  ///
  /// @ingroup ge
  /// @brief Get dynamic batch_info
  /// @param [in] model_id
  /// @param [out] batch_info
  /// @param [out] dynamic_type
  /// @return execute result
  ///
  static Status GetDynamicBatchInfo(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                    int32_t &dynamic_type);

  ///
  /// @ingroup ge
  /// @brief Get combined dynamic dims info
  /// @param [in] model_id
  /// @param [out] batch_info
  /// @return execute result
  ///
  static Status GetCombinedDynamicDims(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info);

  ///
  /// @ingroup ge
  /// @brief Get user designate shape order
  /// @param [in] model_id
  /// @param [out] user_input_shape_order
  /// @return execute result
  ///
  static Status GetUserDesignateShapeOrder(const uint32_t model_id, std::vector<std::string> &user_input_shape_order);

  static Status GetCurrentShape(const uint32_t model_id, std::vector<int64_t> &batch_info, int32_t &dynamic_type);

  static Status GetNodeAttr(const uint32_t model_id, const std::string &op_name, const std::string &attr_name,
                            std::string &attr_value);

  static Status GetOutputShapeInfo(const uint32_t model_id, std::vector<std::string> &dynamic_output_shape_info);

  static Status GetOrigInputInfo(const uint32_t model_id, const uint32_t index, OriginInputInfo &orig_input_info);
  static Status GetAllAippInputOutputDims(const uint32_t model_id, const uint32_t index,
                                          std::vector<InputOutputDims> &input_dims,
                                          std::vector<InputOutputDims> &output_dims);

  static Status GetOpDescInfo(const uint32_t device_id, const uint32_t stream_id, const uint32_t task_id,
                              OpDescInfo &op_desc_info);

  static uint32_t GetExecuteModelId(const GeRootModelPtr &ge_root_model);

 private:
  Status PrepareInputData(const std::vector<GeTensor> &input_tensor, InputData &graph_input_data,
                          OutputData &graph_output_data, const std::vector<InputOutputDescInfo> &output_desc);

  Status SyncExecuteModel(const uint32_t model_id, const std::vector<GeTensor> &input_tensor,
                          std::vector<GeTensor> &output_tensor, const error_message::Context &error_context);

  Status SyncMultiExecuteModel(const GeRootModelPtr &ge_root_model,
                               const std::vector<GeTensor> &input_tensor,
                               std::vector<GeTensor> &output_tensor);

  Status AsyncExecuteModel(const GeRootModelPtr &ge_root_model, const uint32_t model_id,
                           const std::vector<Tensor> &inputs,
                           const error_message::Context &error_context,
                           const RunAsyncCallback &callback) const;

  Status AsyncMultiExecuteModel(const GeRootModelPtr &ge_root_model, const std::vector<Tensor> &inputs,
                                const RunAsyncCallback &callback);

  Status FreeInOutBuffer();

  static Status FreeInOutBuffer(std::vector<BufferInfo> &buffers);

  Status MallocInOutBuffer(const uint32_t model_id, std::vector<BufferInfo> &buffer_info);

  std::vector<InputOutputDescInfo> outputs_desc_;
  GraphId last_graph_id_{std::numeric_limits<uint32_t>::max()};

  std::mutex cache_mutex_;
  std::map<uint32_t, std::vector<BufferInfo>> buffer_cache_; // key is model_id
};

using SyncExecuteModelFunc = std::function<Status(GraphExecutor *executor,
                                                  const uint32_t model_id,
                                                  const std::vector<GeTensor> &input_tensor,
                                                  std::vector<GeTensor> &output_tensor,
                                                  const error_message::Context &error_context)>;

using AsyncExecuteModelFunc = std::function<Status(GraphExecutor *executor,
                                                   const GeRootModelPtr &ge_root_model,
                                                   const uint32_t model_id,
                                                   const std::vector<Tensor> &inputs,
                                                   const error_message::Context &error_context,
                                                   const RunAsyncCallback &callback)>;
}  // namespace ge

#endif  // GE_GRAPH_EXECUTE_GRAPH_EXECUTE_H_
