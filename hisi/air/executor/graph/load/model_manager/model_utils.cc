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

#include "graph/load/model_manager/model_utils.h"

#include <string>

#include "framework/common/op/ge_op_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/ge_context.h"
#include "graph/utils/graph_utils.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "graph/passes/mds_kernels/mds_utils.h"

namespace ge {
namespace {
const uint64_t kSessionScopeMemoryMask = 0x100000000UL;
// deploy info
const std::string kMultiMode = "MultiMode";
const std::string kSingleMode = "SingleMode";
const uint32_t kInvalidDeviceId = UINT32_MAX;
const char_t *const kUsedStreamNum = "used_stream_num";
}

bool ModelUtils::ValidateMemRange(const ConstOpDescPtr &op_desc, const uint64_t total_size, const int64_t offset,
                                  const int64_t size) {
  if (CheckInt64AddOverflow(offset, size) != SUCCESS) {
    GELOGE(PARAM_INVALID, "Int64 %ld and %ld addition can result in overflow!", offset, size);
    return false;
  }
  const int64_t mem_range = offset + size;
  if (total_size < static_cast<uint64_t>(mem_range)) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) memory out of range, offset:%ld, size:%ld, exceed total size:%lu.",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), offset, size, total_size);
    GELOGE(OUT_OF_MEMORY, "[Check][Param]Node:%s(%s) memory out of range, offset:%ld, size:%ld, exceed total size:%lu.",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), offset, size, total_size);
    return false;
  }
  return true;
}

///
/// @ingroup ge
/// @brief Get input size.
/// @return std::vector<int64_t>
///
std::vector<int64_t> ModelUtils::GetInputSize(const ConstOpDescPtr &op_desc) {
  std::vector<int64_t> v_input_size;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_input_size);

  const size_t inputs_size = op_desc->GetAllInputsSize();
  for (size_t i = 0U; i < inputs_size; ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
      continue;
    }

    int64_t tensor_size = 0;
    GE_IF_BOOL_EXEC(
      TensorUtils::GetSize(*tensor_desc, tensor_size) != GRAPH_SUCCESS,
      GELOGI("Get size from TensorDesc failed, op : %s, input index : %zu", op_desc->GetName().c_str(), i);
      continue);

    GELOGI("GetInputSize op: %s, index: %zu, size:%ld", op_desc->GetName().c_str(), i, tensor_size);
    v_input_size.push_back(tensor_size);
  }

  return v_input_size;
}

///
/// @ingroup ge
/// @brief Get output size.
/// @return std::vector<int64_t>
///
std::vector<int64_t> ModelUtils::GetOutputSize(const ConstOpDescPtr &op_desc) {
  std::vector<int64_t> v_output_size;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_output_size);

  const size_t outputs_size = op_desc->GetOutputsSize();
  const std::vector<int64_t> v_output_offset = op_desc->GetOutputOffset();
  GE_IF_BOOL_EXEC(v_output_offset.size() != outputs_size,
                  GELOGW("Output param invalid: output_offset=%zu, outputs=%zu.", v_output_offset.size(), outputs_size);
                  return v_output_size;);

  for (size_t i = 0U; i < outputs_size; ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableOutputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
      continue;
    }

    int64_t string_max_size = 0;
    if ((tensor_desc->GetDataType() == DT_STRING) && AttrUtils::GetInt(op_desc, "_op_max_size", string_max_size)) {
      GELOGI("Get op max size value = %ld", string_max_size);
      v_output_size.push_back(string_max_size);
      continue;
    }

    int64_t tensor_size = 0;
    GE_IF_BOOL_EXEC(
      TensorUtils::GetSize(*tensor_desc, tensor_size) != GRAPH_SUCCESS,
      GELOGI("Get size from TensorDesc failed, op : %s, output index : %zu", op_desc->GetName().c_str(), i);
      continue);

    GELOGI("GetOutputSize op: %s, index: %zu, size:%ld", op_desc->GetName().c_str(), i, tensor_size);
    v_output_size.push_back(tensor_size);
  }

  return v_output_size;
}

///
/// @ingroup ge
/// @brief Get workspace size.
/// @return std::vector<int64_t>
///
std::vector<int64_t> ModelUtils::GetWorkspaceSize(const ConstOpDescPtr &op_desc) {
  std::vector<int64_t> v_workspace_size;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_workspace_size);

  const std::vector<int64_t> v_workspace_num = op_desc->GetWorkspace();
  const std::vector<int64_t> v_workspace_bytes = op_desc->GetWorkspaceBytes();
  if (v_workspace_num.size() != v_workspace_bytes.size()) {
    GELOGW("workspace_num[%zu]!= workspace_bytes[%zu]", v_workspace_num.size(), v_workspace_bytes.size());
    return v_workspace_size;
  }

  return v_workspace_bytes;
}

///
/// @ingroup ge
/// @brief Get weight size.
/// @return std::vector<int64_t>
///
std::vector<int64_t> ModelUtils::GetWeightSize(const ConstOpDescPtr &op_desc) {
  std::vector<int64_t> v_weight_size;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_weight_size);

  // const op, get weight directly
  const std::string type_name = op_desc->GetType();
  if ((type_name == CONSTANT) || (type_name == CONSTANTOP)) {
    ConstGeTensorPtr weight = nullptr;
    if (AttrUtils::GetTensor(*op_desc, ATTR_NAME_WEIGHTS, weight)) {
      v_weight_size.push_back(static_cast<int64_t>(TensorUtils::GetWeightSize(weight)));
    }

    return v_weight_size;
  }

  // other ops get weight from connected constop
  const size_t inputs_size = op_desc->GetAllInputsSize();
  const std::vector<bool> v_is_input_const = op_desc->GetIsInputConst();
  for (size_t i = 0U; i < inputs_size; ++i) {
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {
      const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
      if (tensor_desc == nullptr) {
        GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
        continue;
      }

      int64_t tensor_size = 0;
      (void)TensorUtils::GetSize(*tensor_desc, tensor_size);
      v_weight_size.push_back(tensor_size);
    }
  }

  return v_weight_size;
}

///
/// @ingroup ge
/// @brief Get weights.
/// @return std::vector<ConstGeTensorPtr>
///
std::vector<ConstGeTensorPtr> ModelUtils::GetWeights(const ConstOpDescPtr &op_desc) {
  std::vector<ConstGeTensorPtr> v_weights;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_weights);

  // const op, get weight directly
  const std::string op_type = op_desc->GetType();
  if ((op_type == CONSTANT) || (op_type == CONSTANTOP)) {
    ConstGeTensorPtr weight = nullptr;
    if (AttrUtils::GetTensor(*op_desc, ATTR_NAME_WEIGHTS, weight)) {
      v_weights.push_back(weight);
    }

    return v_weights;
  }

  // other ops get weight from connected constop
  const size_t inputs_size = op_desc->GetAllInputsSize();
  const std::vector<bool> v_is_input_const = op_desc->GetIsInputConst();
  for (size_t i = 0U; i < inputs_size; ++i) {
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {
      const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
      if (tensor_desc == nullptr) {
        GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
        continue;
      }

      ConstGeTensorPtr weight = nullptr;
      if (AttrUtils::GetTensor(*tensor_desc, ATTR_NAME_WEIGHTS, weight)) {
        v_weights.push_back(weight);
      }
    }
  }

  return v_weights;
}

///
/// @ingroup ge
/// @brief Get AiCpuOp Input descriptor.
/// @return std::vector<ccAICPUTensor>
///
std::vector<ccAICPUTensor> ModelUtils::GetInputDescs(const ConstOpDescPtr &op_desc) {
  // AiCpuOp::GetInputDescs
  std::vector<ccAICPUTensor> v_input_descs;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_input_descs);

  const size_t inputs_size = op_desc->GetAllInputsSize();
  const std::vector<bool> v_is_input_const = op_desc->GetIsInputConst();

  for (size_t i = 0U; i < inputs_size; ++i) {
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {  // skip Const input node
      continue;
    }

    const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
      continue;
    }

    uint32_t dim_cnt = 0U;
    GE_CHK_BOOL_EXEC_WARN(TensorUtils::GetRealDimCnt(*tensor_desc, dim_cnt) == GRAPH_SUCCESS, continue,
                          "Get dim_cnt failed");

    ccAICPUTensor tmp{};
    tmp.format = static_cast<tagOpTensorFormat>(tensor_desc->GetFormat());
    tmp.dim_cnt = static_cast<int32_t>(dim_cnt);
    tmp.data_type = static_cast<tagOpDataType>(tensor_desc->GetDataType());

    for (int32_t j = 0; j < 4; j++) {  // 4 dims
      const int64_t tensor_dim = tensor_desc->GetShape().GetDim(static_cast<size_t>(j));
      if (tensor_dim > INT32_MAX) {
        GELOGW("Op[%s], input tensor[%zu], dim[%d]: tensor_dim[%ld] is greater than INT32_MAX[%d]",
               op_desc->GetName().c_str(), i, j, tensor_dim, INT32_MAX);
      }
      tmp.dim[j] = (j < tmp.dim_cnt) ? static_cast<int32_t>(tensor_dim) : 1;
    }

    v_input_descs.push_back(tmp);
  }

  return v_input_descs;
}

///
/// @ingroup ge
/// @brief Get AiCpuOp Output descriptor.
/// @return std::vector<ccAICPUTensor>
///
std::vector<ccAICPUTensor> ModelUtils::GetOutputDescs(const ConstOpDescPtr &op_desc) {
  // AiCpuOp::GetOutputDescs
  std::vector<ccAICPUTensor> v_output_descs;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_output_descs);

  // init op output ccAICPUTensor struct
  const size_t output_num = op_desc->GetOutputsSize();
  for (size_t i = 0U; i < output_num; ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableOutputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
      continue;
    }

    uint32_t dim_cnt = 0U;
    GE_CHK_BOOL_EXEC_WARN(TensorUtils::GetRealDimCnt(*tensor_desc, dim_cnt) == GRAPH_SUCCESS, continue,
                          "Get dim_cnt failed");

    ccAICPUTensor tmp{};
    tmp.format = static_cast<tagOpTensorFormat>(tensor_desc->GetFormat());
    tmp.dim_cnt = static_cast<int32_t>(dim_cnt);
    tmp.data_type = static_cast<tagOpDataType>(tensor_desc->GetDataType());

    for (int32_t j = 0; j < 4; j++) {  // 4 dims
      const int64_t tensor_dim = tensor_desc->GetShape().GetDim(static_cast<size_t>(j));
      if (tensor_dim > INT32_MAX) {
        GELOGW("Op[%s], output tensor[%zu], dim[%d]: tensor_dim[%ld] is greater than INT32_MAX[%d]",
               op_desc->GetName().c_str(), i, j, tensor_dim, INT32_MAX);
      }
      tmp.dim[j] = (j < tmp.dim_cnt) ? static_cast<int32_t>(tensor_dim) : 1;
    }

    v_output_descs.push_back(tmp);
  }

  return v_output_descs;
}

///
/// @ingroup ge
/// @brief Get input address.
/// @return std::vector<void*>
///
std::vector<void *> ModelUtils::GetInputAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc) {
  GELOGD("Start GetInputAddrs: op_name[%s]", op_desc->GetName().c_str());

  auto v_input_addr = GetInputDataAddrs(model_param, op_desc);
  if (GetInputOutputDescAddrs(model_param, op_desc, op_desc->GetAllInputsDescPtr(), v_input_addr) != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Check] GetInputOutputDescAddrs failed: op_name[%s]", op_desc->GetName().c_str());
    return {};
  }
  return v_input_addr;
}

///
/// @ingroup ge
/// @brief Get input data address.
/// @return std::vector<void*>
///
std::vector<void *> ModelUtils::GetInputDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc) {
  std::vector<void *> v_input_data_addr;  // init as:buf_base + op_def_->input(i));
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_input_data_addr);
  const uint64_t session_id = model_param.session_id;

  const size_t inputs_size = op_desc->GetInputsSize();
  const std::vector<int64_t> v_input_offset = op_desc->GetInputOffset();

  const std::vector<bool> v_is_input_const = op_desc->GetIsInputConst();
  size_t non_const_index = 0U;
  std::vector<int64_t> v_memory_type;
  const bool has_mem_type_attr = AttrUtils::GetListInt(op_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, v_memory_type);
  if (has_mem_type_attr && (v_memory_type.size() != inputs_size)) {
    REPORT_INNER_ERROR("E19999", "Attr:%s, memory_type.size:%zu != input_desc.size:%zu, op:%s(%s), check invalid",
                       ATTR_NAME_INPUT_MEM_TYPE_LIST.c_str(), v_memory_type.size(), inputs_size,
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Attr:%s, memory_type.size:%zu != input_desc.size:%zu, op:%s(%s)",
           ATTR_NAME_INPUT_MEM_TYPE_LIST.c_str(), v_memory_type.size(), inputs_size,
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return v_input_data_addr;
  }

  v_input_data_addr.reserve(inputs_size);
  for (size_t i = 0U; i < op_desc->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    GE_IF_BOOL_EXEC(tensor_desc == nullptr, GELOGD("Op: %s, Index: %zu, has no input", op_desc->GetName().c_str(), i);
                    continue;)
    int64_t tensor_size = 0;
    GE_CHK_STATUS_EXEC(TensorUtils::GetSize(*tensor_desc, tensor_size), return {});
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {
      // Add weights address to input
      int64_t data_offset = 0;
      GE_CHK_STATUS(TensorUtils::GetDataOffset(*tensor_desc, data_offset));
      int64_t weight_size = 0;
      // The reason why GetTensorSizeInBytes is used here is that the weight is allocated based on the size of
      // TensorData in function AdjustConstWeightSize. and the size is zero when the tensor is empty.
      GE_CHK_STATUS(TensorUtils::GetTensorSizeInBytes(*tensor_desc, weight_size));
      GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.weight_size, data_offset, weight_size), return {});
      void *const weight_addr = ValueToPtr(model_param.weight_base + static_cast<uint64_t>(data_offset));
      v_input_data_addr.push_back(weight_addr);
      GELOGI("[IMAS]GetInputDataAddrs graph_%u type[C] name[%s] input[%zu] memaddr[%p]", model_param.graph_id,
             op_desc->GetName().c_str(), i, weight_addr);
      non_const_index++;
      continue;
    }

    GE_IF_BOOL_EXEC(non_const_index >= v_input_offset.size(), break);

    const int64_t input_offset = v_input_offset[non_const_index];
    non_const_index++;
    int64_t inner_offset = 0;
    (void)AttrUtils::GetInt(op_desc->MutableInputDesc(static_cast<uint32_t>(i)), ATTR_NAME_INNER_OFFSET, inner_offset);
    GE_IF_BOOL_EXEC((model_param.var_size != 0U)
                    && (VarManager::Instance(session_id)->IsVarAddr(input_offset - inner_offset)),
                    void *variable_addr = nullptr;
                    GE_CHK_STATUS_EXEC(GetVarAddr(model_param, op_desc, input_offset - inner_offset,
                                                  tensor_size + inner_offset, variable_addr), return {});
                    variable_addr = ValueToPtr(PtrToValue(variable_addr) + static_cast<uint64_t>(inner_offset));
                    v_input_data_addr.push_back(variable_addr);
                    GELOGI("[IMAS]GetInputDataAddrs graph_%u type[V] name[%s] input[%lu] memaddr[%p]",
                           model_param.graph_id, op_desc->GetName().c_str(), i, variable_addr);
                    continue);

    int64_t mem_type = -1;
    const bool tensor_has_mem_type = AttrUtils::GetInt(tensor_desc, ATTR_NAME_TENSOR_MEM_TYPE, mem_type);
    // feature maps
    void *mem_addr = nullptr;
    if (has_mem_type_attr && (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_L1))) {  // fusion
      mem_addr = ValueToPtr(static_cast<uint64_t>(input_offset));
      v_input_data_addr.push_back(mem_addr);
    } else if (has_mem_type_attr && (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_HOST))) {
      const auto &host_mem_info = model_param.memory_infos.at(RT_MEMORY_HOST); // Init by InitRuntimeParams.
      void *const host_mem_addr = host_mem_info.GetMemory(v_input_offset[i], tensor_size);
      v_input_data_addr.push_back(host_mem_addr);
      GELOGI("[IMAS]GetInputDataAddrs graph_%u type[H] name[%s] input[%zu] memaddr[%p]", model_param.graph_id,
             op_desc->GetName().c_str(), i, host_mem_addr);
      continue;
    } else if (has_mem_type_attr && (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_TS))) {
      // The input size and peer output size may be not consecutive, therefore, the tensor_size is not been checked.
      GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, input_offset, 0), return {});
      mem_addr = model_param.ts_mem_mall->Acquire(input_offset, static_cast<uint64_t>(tensor_size));
      v_input_data_addr.push_back(mem_addr);
    } else if (tensor_has_mem_type && (mem_type == static_cast<int64_t>(RT_MEMORY_P2P_DDR))) {
      const auto &p2p_mem_info = model_param.memory_infos.at(RT_MEMORY_P2P_DDR); // Init by InitRuntimeParams.
      void *const p2p_mem_addr = p2p_mem_info.GetMemory(v_input_offset[i], tensor_size);
      v_input_data_addr.push_back(p2p_mem_addr);
      GELOGI("[IMAS]GetInputDataAddrs graph_%u type[P] name[%s] input[%zu] memaddr[%p]", model_param.graph_id,
             op_desc->GetName().c_str(), i, p2p_mem_addr);
      continue;
    } else {
      // The input size and peer output size may be not consecutive, therefore, the tensor_size is not been checked.
      GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, input_offset, 0), return {});
      mem_addr = ValueToPtr(model_param.mem_base + static_cast<uint64_t>(input_offset));
      v_input_data_addr.push_back(mem_addr);
    }
    GELOGI("[IMAS]GetInputDataAddrs graph_%u type[F] name[%s] input[%zu] memaddr[%p]", model_param.graph_id,
           op_desc->GetName().c_str(), i, mem_addr);
  }

  return v_input_data_addr;
}

///
/// @ingroup ge
/// @brief Get variable address.
/// @return Status
///
Status ModelUtils::GetVarAddr(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc, const int64_t offset,
                              const int64_t tensor_size, void *&var_addr) {
  Status ret = SUCCESS;
  const rtMemType_t mem_type = VarManager::Instance(model_param.session_id)->GetVarMemType(offset);
  switch (mem_type) {
    case RT_MEMORY_RDMA_HBM: {
      if (offset < 0) {
        REPORT_INNER_ERROR("E19999", "Param offset:%ld < 0, check invalid", offset);
        GELOGE(PARAM_INVALID, "[Check][Param] Param offset:%ld cannot be negative", offset);
        ret = PARAM_INVALID;
        break;
      }
      var_addr = ValueToPtr(static_cast<uint64_t>(offset));
      break;
    }
    case RT_MEMORY_HBM: {
      if (model_param.logic_var_base > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        GELOGE(FAILED, "logic_var_base[%lu] is greater than int64_max[%ld] ", model_param.logic_var_base,
               std::numeric_limits<int64_t>::max());
        ret = FAILED;
        break;
      }

      if (!ValidateMemRange(op_desc, model_param.var_size, offset - static_cast<int64_t>(model_param.logic_var_base),
                            tensor_size)) {
        ret = PARAM_INVALID;
        break;
      }
      var_addr = ValueToPtr((model_param.var_base + static_cast<uint64_t>(offset)) - model_param.logic_var_base);
      break;
    }
    default: {
      REPORT_INNER_ERROR("E19999", "Get mem_type:%d for offset:%ld is unsupported, check invalid", mem_type, offset);
      GELOGE(PARAM_INVALID, "[Check][Param] Get mem_type:%d for offset:%ld is unsupported, check invalid",
             mem_type, offset);
      ret = PARAM_INVALID;
      break;
    }
  }
  GE_CHECK_NOTNULL(var_addr);
  return ret;
}

///
/// @ingroup ge
/// @brief Get output address.
/// @return std::vector<void*>
///
std::vector<void *> ModelUtils::GetOutputAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc) {
  GELOGD("Start GetOutputAddrs: op_name[%s]", op_desc->GetName().c_str());

  auto v_output_addr = GetOutputDataAddrs(model_param, op_desc);
  if (GetInputOutputDescAddrs(model_param, op_desc, op_desc->GetAllOutputsDescPtr(), v_output_addr) != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Check] GetInputOutputDescAddrs failed: op_name[%s]", op_desc->GetName().c_str());
    return {};
  }
  return v_output_addr;
}

///
/// @ingroup ge
/// @brief Get output data address.
/// @return Status
///
std::vector<void *> ModelUtils::GetOutputDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc) {
  std::vector<void *> v_output_data_addr;  // init as:buf_base + op_def_->output(i)
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_output_data_addr);
  const uint64_t session_id = model_param.session_id;

  const size_t outputs_size = op_desc->GetOutputsSize();
  const std::vector<int64_t> v_output_offset = op_desc->GetOutputOffset();
  GE_IF_BOOL_EXEC(v_output_offset.size() != outputs_size,
                  GELOGW("Output param invalid: output_offset=%zu, outputs=%zu.", v_output_offset.size(), outputs_size);
                  return v_output_data_addr);
  std::vector<int64_t> v_memory_type;
  const bool has_mem_type_attr = AttrUtils::GetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, v_memory_type);
  if (has_mem_type_attr && (v_memory_type.size() != outputs_size)) {
    REPORT_INNER_ERROR("E19999", "Attr:%s, memory_type.size:%zu != output_desc.size:%zu, op:%s(%s), check invalid",
                       ATTR_NAME_OUTPUT_MEM_TYPE_LIST.c_str(), v_memory_type.size(), outputs_size,
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Attr:%s, memory_type.size:%zu != output_desc.size:%zu, op:%s(%s)",
           ATTR_NAME_OUTPUT_MEM_TYPE_LIST.c_str(), v_memory_type.size(), outputs_size,
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return v_output_data_addr;
  }

  v_output_data_addr.reserve(outputs_size);
  for (size_t i = 0U; i < outputs_size; ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableOutputDesc(static_cast<uint32_t>(i));
    // skip some addr
    if (tensor_desc == nullptr) {
      GELOGW("Op: %s, Index: %zu, Tensor Desc is null", op_desc->GetName().c_str(), i);
      continue;
    }
    int32_t calc_type = 0;
    (void)AttrUtils::GetInt(tensor_desc, ATTR_NAME_MEMORY_SIZE_CALC_TYPE, calc_type);
    if (calc_type == static_cast<int32_t>(MemorySizeCalcType::ALWAYS_EMPTY)) {
      GELOGD("%s is an optional output, the address don't need to be saved.", tensor_desc->GetName().c_str());
      continue;
    }
    // var addr
    int64_t inner_offset = 0;
    (void)AttrUtils::GetInt(op_desc->MutableOutputDesc(static_cast<uint32_t>(i)), ATTR_NAME_INNER_OFFSET, inner_offset);
    int64_t tensor_size = 0;
    GE_CHK_STATUS_EXEC(TensorUtils::GetSize(*tensor_desc, tensor_size), return {});
    GE_IF_BOOL_EXEC((model_param.var_size != 0U)
                    && (VarManager::Instance(session_id)->IsVarAddr(v_output_offset[i] - inner_offset)),
                    void *variable_addr = nullptr;
                    GE_CHK_STATUS_EXEC(GetVarAddr(model_param, op_desc, v_output_offset[i] - inner_offset,
                                                  tensor_size + inner_offset, variable_addr), return {});
                    variable_addr = ValueToPtr(PtrToValue(variable_addr) + static_cast<uint64_t>(inner_offset));
                    v_output_data_addr.push_back(variable_addr);
                    GELOGI("[IMAS]graph_%u type[V] name[%s] output[%zu] memaddr[%p]",
                           model_param.graph_id, op_desc->GetName().c_str(), i, variable_addr);
                    continue);

    int64_t memory_type;
    const bool tensor_has_mem_type = AttrUtils::GetInt(tensor_desc, ATTR_NAME_TENSOR_MEM_TYPE, memory_type);
    // feature maps addr
    void *mem_addr = nullptr;
    if (tensor_has_mem_type) {
      // p2p addr
      if (memory_type == static_cast<int64_t>(RT_MEMORY_P2P_DDR)) {
        const auto &p2p_mem_info = model_param.memory_infos.at(RT_MEMORY_P2P_DDR);  // Init by InitRuntimeParams.
        void *const p2p_mem_addr = p2p_mem_info.GetMemory(v_output_offset[i], tensor_size);
        v_output_data_addr.push_back(p2p_mem_addr);
        GELOGI("[IMAS]graph_%u type[P] name[%s] output[%zu] memaddr[%p]", model_param.graph_id,
               op_desc->GetName().c_str(), i, p2p_mem_addr);
        continue;
      }
    }
    if (has_mem_type_attr) {
      // l1 addr
      if (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_L1)) {
        mem_addr = ValueToPtr(static_cast<uint64_t>(v_output_offset[i]));
        v_output_data_addr.push_back(mem_addr);
        // host addr
      } else if (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_HOST)) {
        const auto &host_mem_info = model_param.memory_infos.at(RT_MEMORY_HOST);  // Init by InitRuntimeParams.
        void *const host_mem_addr = host_mem_info.GetMemory(v_output_offset[i], tensor_size);
        v_output_data_addr.push_back(host_mem_addr);
        GELOGI("[IMAS]graph_%u type[H] name[%s] output[%zu] memaddr[%p]", model_param.graph_id,
               op_desc->GetName().c_str(), i, host_mem_addr);
        continue;
        // ts addr
      } else if (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_TS)) {
        GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, v_output_offset[i], tensor_size), return {});
        mem_addr = model_param.ts_mem_mall->Acquire(v_output_offset[i], static_cast<uint64_t>(tensor_size));
        v_output_data_addr.push_back(mem_addr);
        // default is hbm
      } else {
        GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, v_output_offset[i], tensor_size), return {});
        mem_addr = ValueToPtr(model_param.mem_base + static_cast<uint64_t>(v_output_offset[i]));
        v_output_data_addr.push_back(mem_addr);
      }
      // hbm addr
    } else {
      GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, v_output_offset[i], tensor_size), return {});
      mem_addr = ValueToPtr(model_param.mem_base + static_cast<uint64_t>(v_output_offset[i]));
      v_output_data_addr.push_back(mem_addr);
    }
    GELOGI("[IMAS]graph_%u type[F] name[%s] output[%zu] memaddr[%p]", model_param.graph_id, op_desc->GetName().c_str(),
           i, mem_addr);
  }
  return v_output_data_addr;
}

static Status FillSinkTensorDesc(RuntimeTensorDesc &sink_tensor_desc, const GeTensorDescPtr &tensor_desc,
                                 const uint64_t data_addr) {
  sink_tensor_desc.data_addr = data_addr;
  sink_tensor_desc.dtype = static_cast<int64_t>(tensor_desc->GetDataType());
  sink_tensor_desc.format = static_cast<int64_t>(tensor_desc->GetFormat());
  const auto shape = tensor_desc->GetShape();
  const int64_t dim_num = static_cast<int64_t>(shape.GetDimNum());
  sink_tensor_desc.shape[0] = dim_num;
  if (dim_num > kMaxDimSize) {
    GELOGE(PARAM_INVALID, "shape dim size[%ld] out of range[%zu]", dim_num, kMaxDimSize);
    return FAILED;
  }
  for (int64_t i = 0; i < dim_num; i++) {
    sink_tensor_desc.shape[i + 1] = shape.GetDim(static_cast<size_t>(i));
  }
  const auto ori_shape = tensor_desc->GetOriginShape();
  const int64_t ori_dim_num = static_cast<int64_t>(ori_shape.GetDimNum());
  sink_tensor_desc.original_shape[0] = ori_dim_num;
  if (ori_dim_num > kMaxDimSize) {
    GELOGE(PARAM_INVALID, "original shape dim size[%ld] out of range[%zu]", ori_dim_num, kMaxDimSize);
    return FAILED;
  }
  for (int64_t i = 0; i < ori_dim_num; i++) {
    sink_tensor_desc.original_shape[i + 1] = ori_shape.GetDim(static_cast<size_t>(i));
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get tensor description address, and copy data address to tensor description memory.
/// @return Status
///
Status ModelUtils::GetInputOutputDescAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc,
                                           const OpDesc::Vistor<GeTensorDescPtr> &tensor_desc_visitor,
                                           std::vector<void *> &v_addrs) {
  std::vector<int64_t> v_data_mem_type;
  const bool has_mem_type_attr = AttrUtils::GetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, v_data_mem_type);
  size_t tensor_cnt = 0U;
  for (const auto &tensor_desc : tensor_desc_visitor) {
    int32_t calc_type = 0;
    const bool ret = AttrUtils::GetInt(tensor_desc, ATTR_NAME_MEMORY_SIZE_CALC_TYPE, calc_type);
    if (ret && (calc_type == static_cast<int32_t>(MemorySizeCalcType::ALWAYS_EMPTY))) {
      GELOGD("%s is an optional output, don't need to get tensor desc addr.", tensor_desc->GetName().c_str());
      continue;
    }
    int64_t mem_offset;
    const bool has_offset_attr = AttrUtils::GetInt(tensor_desc, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, mem_offset);
    if (!has_offset_attr) {
      tensor_cnt++;
      continue;
    }

    const size_t size = sizeof(struct RuntimeTensorDesc);
    GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, mem_offset, static_cast<int64_t>(size)),
                    return FAILED);
    void *mem_addr = nullptr;
    if (has_mem_type_attr && (v_data_mem_type[tensor_cnt] == static_cast<int64_t>(RT_MEMORY_TS))) {
      mem_addr = model_param.ts_mem_mall->Acquire(mem_offset, size);
    } else {
      mem_addr = ValueToPtr(model_param.mem_base + static_cast<uint64_t>(mem_offset));
    }
    RuntimeTensorDesc sink_tensor_desc;
    GE_CHK_STATUS_RET_NOLOG(FillSinkTensorDesc(sink_tensor_desc, tensor_desc, PtrToValue(v_addrs[tensor_cnt])));
    const rtError_t rt_ret = rtMemcpy(mem_addr, size, &sink_tensor_desc, size, RT_MEMCPY_HOST_TO_DEVICE);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, size:%zu, ret:0x%X", size, rt_ret);
      GELOGE(RT_FAILED, "[Call][RtMemcpy] copy data_addr failed, size:%zu, ret:0x%X", size, rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
    if (tensor_cnt >= v_addrs.size()) {
      GELOGE(FAILED, "[Check] update tensor desc addr failed, tensor_cnt:%zu, size:%zu", tensor_cnt, v_addrs.size());
      return FAILED;
    }
    v_addrs[tensor_cnt] = mem_addr;
    GELOGD("Calc op[%s] tenser[%zu] desc addr[%p] ok", op_desc->GetName().c_str(), tensor_cnt, mem_addr);
    tensor_cnt++;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get workspace data address.
/// @return std::vector<void*>
///
std::vector<void *> ModelUtils::GetWorkspaceDataAddrs(const RuntimeParam &model_param, const ConstOpDescPtr &op_desc) {
  std::vector<void *> v_workspace_data_addr;
  GE_CHECK_NOTNULL_EXEC(op_desc, return v_workspace_data_addr);

  const std::vector<int64_t> v_workspace_offset = op_desc->GetWorkspace();
  const std::vector<int64_t> v_workspace_bytes = op_desc->GetWorkspaceBytes();
  if (v_workspace_offset.size() != v_workspace_bytes.size()) {
    GELOGW("v_workspace_offset.size()[%zu] != v_workspace_bytes.size()[%zu]", v_workspace_offset.size(),
           v_workspace_bytes.size());
    return v_workspace_data_addr;
  }

  std::vector<bool> workspace_reuse_flag;
  const bool has_workspace_reuse = AttrUtils::GetListBool(op_desc, "workspace_reuse_flag", workspace_reuse_flag);
  std::vector<int64_t> v_memory_type;
  std::vector<int64_t> workspace_memory_type;
  const bool has_mem_type_attr = AttrUtils::GetListInt(op_desc, TVM_ATTR_NAME_WORKSPACE_TYPE, v_memory_type);
  const bool has_mem_type_workspace = AttrUtils::GetListInt(op_desc, ATTR_NAME_WORKSPACE_TYPE_LIST,
                                                            workspace_memory_type);

  std::vector<int32_t> workspace_no_reuse_scope;
  const bool has_workspace_no_reuse_scope = AttrUtils::GetListInt(op_desc, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE,
                                                                  workspace_no_reuse_scope);

  v_workspace_data_addr.reserve(v_workspace_bytes.size());
  for (size_t i = 0U; i < v_workspace_bytes.size(); ++i) {
    // Temporary solution, the aicpu workspace of multiple images cannot be shared.
    const bool aicpu_work_space = (has_workspace_reuse && (i < workspace_reuse_flag.size()) &&
        (!workspace_reuse_flag[i]) && (!model_param.is_single_op));
    if (aicpu_work_space) {
      void *const mem_addr = model_param.aicpu_mem_mall->Acquire(v_workspace_offset[i],
                                                                 static_cast<uint64_t>(v_workspace_bytes[i]));
      v_workspace_data_addr.push_back(mem_addr);
      GELOGI("[IMAS]graph_%u type[F] name[%s] aicpu workspace[%zu]  offset[%ld] bytes[%ld] memaddr[%p]",
             model_param.graph_id, op_desc->GetName().c_str(), i, v_workspace_offset[i],
             v_workspace_bytes[i], mem_addr);
      continue;
    }

    if (has_mem_type_workspace && (workspace_memory_type[i] == static_cast<int64_t>(RT_MEMORY_P2P_DDR))) {
      const int64_t p2p_workspace_offset = v_workspace_offset[i];
      const int64_t p2p_workspace_bytes = v_workspace_bytes[i];
      const auto &p2p_mem_info = model_param.memory_infos.at(RT_MEMORY_P2P_DDR); // Init by InitRuntimeParams.
      void *const p2p_mem_addr = p2p_mem_info.GetMemory(p2p_workspace_offset, p2p_workspace_bytes);
      v_workspace_data_addr.push_back(p2p_mem_addr);
      GELOGI("[IMAS] graph_%u type[P] name[%s] p2p workspace[%zu]  offset[%ld] bytes[%ld] memaddr[%p]",
             model_param.graph_id, op_desc->GetName().c_str(), i, p2p_workspace_offset, p2p_workspace_bytes,
             p2p_mem_addr);
      continue;
    }

    if (has_mem_type_attr && (v_memory_type[i] == static_cast<int64_t>(RT_MEMORY_L1))) {
      v_workspace_data_addr.push_back(ValueToPtr(static_cast<uint64_t>(v_workspace_offset[i])));
      GELOGI("[IMAS]graph_%u type[L1] name[%s], mem_addr[workspace index %zu]:0x%lx",
             model_param.graph_id, op_desc->GetName().c_str(), i, v_workspace_offset[i]);
    } else if (v_workspace_bytes[i] == 0) {
      v_workspace_data_addr.push_back(nullptr);
      GELOGI("[IMAS]graph_%u type[F] name[%s] workspace[%zu] offset[%ld] bytes[%ld] Null addr",
             model_param.graph_id, op_desc->GetName().c_str(), i, v_workspace_offset[i], v_workspace_bytes[i]);
    } else {
      void *mem_addr = nullptr;
      const bool session_scope_memory = (has_workspace_no_reuse_scope) && (i < workspace_no_reuse_scope.size());
      if (session_scope_memory) {
        const auto &memory_info = model_param.memory_infos.at(kSessionScopeMemoryMask | RT_MEMORY_HBM);
        mem_addr = memory_info.GetMemory(v_workspace_offset[i], v_workspace_bytes[i]);
      } else {
        GE_IF_BOOL_EXEC(!ValidateMemRange(op_desc, model_param.mem_size, v_workspace_offset[i], v_workspace_bytes[i]),
                        return {});
        mem_addr = ValueToPtr(model_param.mem_base + static_cast<uint64_t>(v_workspace_offset[i]));
      }
      v_workspace_data_addr.push_back(mem_addr);
      GELOGI("[IMAS]graph_%u type[F] name[%s] workspace[%zu] offset[%ld] bytes[%ld] memaddr[%p]",
             model_param.graph_id, op_desc->GetName().c_str(), i, v_workspace_offset[i], v_workspace_bytes[i],
             mem_addr);
    }
  }

  return v_workspace_data_addr;
}

///
/// @ingroup ge
/// @brief Get runtime memory address.
/// @return Status
///
Status ModelUtils::GetRtAddress(const RuntimeParam &param, const uintptr_t logic_addr, uint8_t *&mem_addr) {
  void *runtime_base_addr = nullptr;
  uint64_t max_logic_offset = 0U;
  if ((param.logic_mem_base <= logic_addr) && (logic_addr < (param.logic_mem_base + param.mem_size))) {
    runtime_base_addr = ValueToPtr(param.mem_base - param.logic_mem_base);
    max_logic_offset = param.logic_mem_base + param.mem_size;
    GELOGI("The logic addr:0x%lx is data address, base:0x%lx, size:%lu", logic_addr, param.logic_mem_base,
           param.mem_size);
  } else if ((param.logic_weight_base <= logic_addr) && (logic_addr < (param.logic_weight_base + param.weight_size))) {
    runtime_base_addr = ValueToPtr(param.weight_base - param.logic_weight_base);
    max_logic_offset = param.logic_weight_base + param.weight_size;
    GELOGI("The logic addr:0x%lx is weight address, base:0x%lx, size:%lu", logic_addr, param.logic_weight_base,
           param.weight_size);
  } else if ((param.logic_var_base <= logic_addr) && (logic_addr < (param.logic_var_base + param.var_size))) {
    runtime_base_addr = ValueToPtr(param.var_base - param.logic_var_base);
    max_logic_offset = param.logic_var_base + param.var_size;
    GELOGI("The logic addr:0x%lx is variable address, base:0x%lx, size:%lu", logic_addr, param.logic_var_base,
           param.var_size);
  } else if ((param.host_logic_mem_base <= logic_addr) &&
             (logic_addr < (param.host_logic_mem_base + param.host_mem_size))) {
    runtime_base_addr = ValueToPtr(param.host_mem_base - param.host_logic_mem_base);
    max_logic_offset = param.host_logic_mem_base + param.host_mem_size;
    GELOGI("The logic addr:0x%lx is host address, base:0x%lx, size:%lu", logic_addr, param.host_logic_mem_base,
           param.host_mem_size);
  } else if (logic_addr != 0U) {
    mem_addr = nullptr;
    REPORT_INNER_ERROR("E19999", "Check param logic addr:0x%lx abnormal", logic_addr);
    GELOGE(PARAM_INVALID, "[Check][Param] The logic addr:0x%lx is abnormal", logic_addr);
    return PARAM_INVALID;
  } else {
    GELOGW("The logic addr is:0x%lx, base:0x%lx, size:%lu", logic_addr, param.logic_var_base, param.var_size);
  }

  mem_addr = PtrAdd<uint8_t>(static_cast<uint8_t *>(runtime_base_addr), static_cast<size_t>(max_logic_offset),
                             static_cast<size_t>(logic_addr));
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Set device.
/// @return Status
///
Status ModelUtils::SetDevice(const uint32_t device_id, const int64_t die_id) {
  if ((device_id != kInvalidDeviceId) && (die_id != std::numeric_limits<int64_t>::max())) {
    rtError_t rt_ret = rtSetDeviceV2(static_cast<int32_t>(device_id), RT_DEVICE_MODE_MULTI_DIE);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtSetDevice failed, device_id:%u, ret:0x%X", device_id, rt_ret);
      GELOGE(RT_FAILED, "[Call][rtSetDevice] failed, device_id:%u, ret:0x%X", device_id, rt_ret);
      return RT_FAILED;
    }
    rt_ret = rtSetDie(static_cast<int32_t>(die_id));
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtSetDieId failed, die_id:%ld, ret:0x%X", die_id, rt_ret);
      GELOGE(RT_FAILED, "[Call][RtSetDevice] rtSetDieId, die_id:%ld, ret:0x%X", die_id, rt_ret);
      return RT_FAILED;
    }
  } else if (device_id != kInvalidDeviceId) {
    GE_CHK_STATUS_RET(MdsUtils::SetDevice(static_cast<int32_t>(device_id)));
  } else {
    REPORT_CALL_ERROR("E19999", "Call SetDevice failed, device_id:%u, die_id:%ld", device_id, die_id);
    GELOGE(RT_FAILED, "[Call][SetDevice] failed, device_id:%u, die_id:%ld", device_id, die_id);
    return RT_FAILED;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Reset device.
/// @return Status
///
Status ModelUtils::ResetDevice(const uint32_t device_id) {
  const rtError_t rt_ret = rtDeviceReset(static_cast<int32_t>(device_id));
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtSetDevice failed, device_id:%u, ret:0x%X", device_id, rt_ret);
    GELOGE(RT_FAILED, "[Call][RtSetDevice] failed, device_id:%u, ret:0x%X", device_id, rt_ret);
    return RT_FAILED;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get deploy number.
/// @return size_t
///
size_t ModelUtils::GetDeployNum(const GeRootModelPtr &ge_root_model, std::vector<size_t> &deploy_id) {
  const auto &root_graph = ge_root_model->GetRootGraph();
  std::vector<NamedAttrs> deploy_info;
  (void)AttrUtils::GetListNamedAttrs(root_graph, ATTR_NAME_DEPLOY_INFO, deploy_info);

  const auto device_id_fission_from = GetContext().DeviceId();
  for (size_t i = 0U; i < deploy_info.size(); ++i) {
    const auto thread_instance = deploy_info[i];
    std::string device_type = kSingleMode;
    int64_t device_id_fissioned = std::numeric_limits<int64_t>::max();
    if ((!AttrUtils::GetInt(thread_instance, ATTR_NAME_DEPLOY_DEVICE_ID, device_id_fissioned)) ||
        (device_id_fissioned == std::numeric_limits<int64_t>::max())) {
      GELOGW("[GetDeployNum] graph %s has invalid deploy attr %s", root_graph->GetName().c_str(),
             ATTR_NAME_DEPLOY_INFO.c_str());
      return deploy_id.size();
    }
    if (AttrUtils::GetStr(thread_instance, ATTR_NAME_DEPLOY_DEVICE_TYPE, device_type) &&
        (device_type == kMultiMode)) {
      deploy_id.push_back(i);
    }
    if (device_type == kSingleMode) {
      if (device_id_fissioned != static_cast<int64_t>(device_id_fission_from)) {
        GELOGW("[GetDeployNum] skip load for thread instance[%zu]", i);
        continue;
      }
      deploy_id.push_back(i);
    }
  }
  GELOGI("[GetDeployNum] get deploy info instance number is %zu", deploy_id.size());
  return deploy_id.size();
}

Status ModelUtils::CalculateFollowStream(const GeModelPtr &ge_model, uint64_t &hccl_fellow_stream_num) {
  const auto &model_def = ge_model->GetModelTaskDefPtr();
  GE_CHECK_NOTNULL(model_def);
  const Graph &graph = ge_model->GetGraph();
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHECK_NOTNULL(compute_graph);

  std::map<uint32_t, OpDescPtr> op_list;
  for (const auto &node : compute_graph->GetDirectNode()) {
    OpDescPtr op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    (void)op_list.emplace(op_desc->GetId(), op_desc);
  }

  std::multimap<int64_t, uint64_t> main_follow_num;
  for (int32_t i = 0; i < model_def->task_size(); i++) {
    const domi::TaskDef &task = model_def->task(i);
    if (static_cast<rtModelTaskType_t>(task.type()) == RT_MODEL_TASK_HCCL) {
      auto const &hccl_def = task.kernel_hccl();
      const OpDescPtr hccl_op_desc = op_list.at(hccl_def.op_index());
      int64_t main_stream_id = hccl_op_desc->GetStreamId();
      int64_t follow_stream_num = 0;
      if (!AttrUtils::GetInt(hccl_op_desc, kUsedStreamNum, follow_stream_num)) {
        GELOGW("Get used stream num failed, op is %s", hccl_op_desc->GetName().c_str());
      }
      (void)main_follow_num.emplace(main_stream_id, follow_stream_num);
    }
  }
  hccl_fellow_stream_num = CalFollowStreamSum(main_follow_num);
  return SUCCESS;
}

uint64_t ModelUtils::CalFollowStreamSum(const std::multimap<int64_t, uint64_t> &hccl_stream_map) {
  std::map<int64_t, uint64_t> max_follow_stream_map;
  for (const auto &it : hccl_stream_map) {
    const std::map<int64_t, uint64_t>::const_iterator max_it = max_follow_stream_map.find(it.first);
    if (max_it == max_follow_stream_map.cend()) {
      (void)max_follow_stream_map.emplace(it.first, it.second);
      continue;
    }
    if (it.second > max_it->second) {
      max_follow_stream_map.at(max_it->first) = it.second;
    }
  }
  uint64_t need_follow_stream_num = 0U;
  for (const auto &follow_it : max_follow_stream_map) {
    need_follow_stream_num = need_follow_stream_num + follow_it.second;
  }
  GELOGD("Need follow num is %lu", need_follow_stream_num);
  return need_follow_stream_num;
}
}  // namespace ge
