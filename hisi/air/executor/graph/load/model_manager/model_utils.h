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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_UTILS_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_UTILS_H_

#include <vector>

#include "framework/common/ge_inner_error_codes.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "graph/op_desc.h"
#include "graph/utils/tensor_adapter.h"
#include "common/model/ge_root_model.h"


namespace ge {
class ModelUtils {
 public:
  ModelUtils() = default;
  ~ModelUtils() = default;

  ///
  /// @ingroup ge
  /// @brief Get input size.
  /// @return std::vector<uint32_t>
  ///
  static std::vector<int64_t> GetInputSize(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get output size.
  /// @return std::vector<uint32_t>
  ///
  static std::vector<int64_t> GetOutputSize(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get workspace size.
  /// @return std::vector<uint32_t>
  ///
  static std::vector<int64_t> GetWorkspaceSize(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get weight size.
  /// @return std::vector<uint32_t>
  ///
  static std::vector<int64_t> GetWeightSize(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get weights.
  /// @return std::vector<ConstGeTensorPtr>
  ///
  static std::vector<ConstGeTensorPtr> GetWeights(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get AiCpuOp Input descriptor.
  /// @return std::vector<ccAICPUTensor>
  ///
  static std::vector<ccAICPUTensor> GetInputDescs(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get AiCpuOp Output descriptor.
  /// @return std::vector<ccAICPUTensor>
  ///
  static std::vector<ccAICPUTensor> GetOutputDescs(const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get input address.
  /// @return std::vector<void*>
  ///
  static std::vector<void *> GetInputAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get input data address.
  /// @return std::vector<void*>
  ///
  static std::vector<void *> GetInputDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get output address.
  /// @return std::vector<void*>
  ///
  static std::vector<void *> GetOutputAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get output data address.
  /// @return std::vector<void*>
  ///
  static std::vector<void *> GetOutputDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Get output tensor description address, and copy output data address to output tensor description address.
  /// @return Status
  ///
  static Status GetInputOutputDescAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc,
                                        const OpDesc::Vistor<GeTensorDescPtr> &tensor_desc_visitor,
                                        std::vector<void *> &v_addrs);

  ///
  /// @ingroup ge
  /// @brief Get workspace data address.
  /// @return std::vector<void*>
  ///
  static std::vector<void *> GetWorkspaceDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Calculate hccl follw stream
  /// @return Status
  ///
  static Status CalculateFollowStream(const GeModelPtr &ge_model, uint64_t &hccl_fellow_stream_num);

  ///
  /// @ingroup ge
  /// @brief Calculate the sum of follow stream
  /// @return int64_t
  ///
  static uint64_t CalFollowStreamSum(const std::multimap<int64_t, uint64_t> &hccl_stream_map);

  ///
  /// @ingroup ge
  /// @brief Get memory runtime base.
  /// @return Status
  ///
  static Status GetRtAddress(const RuntimeParam &param, const uintptr_t logic_addr, uint8_t *&mem_addr);

  ///
  /// @ingroup ge
  /// @brief Set device.
  /// @return Status
  ///
  static Status SetDevice(const uint32_t device_id, const int64_t die_id);

  ///
  /// @ingroup ge
  /// @brief Reset device.
  /// @return Status
  ///
  static Status ResetDevice(const uint32_t device_id);

  ///
  /// @ingroup ge
  /// @brief Get deploy number.
  /// @return size_t
  ///
  static size_t GetDeployNum(const GeRootModelPtr &ge_root_model, std::vector<size_t> &deploy_id);

 private:
  static bool ValidateMemRange(const ConstOpDescPtr &op_desc, const uint64_t total_size, const int64_t offset,
                               const int64_t size);

  ///
  /// @ingroup ge
  /// @brief Get variable address.
  /// @return Status
  ///
  static Status GetVarAddr(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc, const int64_t offset,
                           const int64_t tensor_size, void *&var_addr);
};
}  // namespace ge

#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_UTILS_H_
