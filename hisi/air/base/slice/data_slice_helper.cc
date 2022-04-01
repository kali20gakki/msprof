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
#include "slice/data_slice_helper.h"
#include "slice/data_slice_factory.h"
#include "graph/operator_factory_impl.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/debug/ge_log.h"
namespace ge {
Status DataSliceHelper::SetInputSlice(ge::OpDescPtr &op, const AxisTypeInfo &slice_info, DataSliceType &input_slice)
{
  if (input_slice.size() == slice_info.GetRelateInputs().size()) {
    for (size_t tensor_slice_idx = 0; tensor_slice_idx < input_slice.size(); tensor_slice_idx++) {
      int64_t tensor_idx = slice_info.GetRelateInputs()[tensor_slice_idx].first;
      size_t input_size = op->GetAllInputsSize();
      if (tensor_idx >= static_cast<int64_t>(input_size)) {
        GELOGE(FAILED, "[DataSlice][Status] node %s cannot find cut tensor index.", op->GetName().c_str());
        return FAILED;
      }
      GeTensorDesc tensor_desc = op->GetInputDesc(slice_info.GetRelateInputs()[tensor_slice_idx].first);
      (void)AttrUtils::SetListListInt(tensor_desc, ge::ATTR_NAME_DATA_SLICE, input_slice[tensor_slice_idx]);
      op->UpdateInputDesc(slice_info.GetRelateInputs()[tensor_slice_idx].first, tensor_desc);
    }
  }
  return SUCCESS;
}

// infer input slice by output slice
Status DataSliceHelper::InferAxisSlice(ge::OpDescPtr &op, const AxisTypeInfo &slice_info)
{
  DataSliceType output_slice;
  for (const auto &tensor_slice : slice_info.GetRelateOutputs()) {
    GeTensorDesc tensor_desc = op->GetOutputDesc(tensor_slice.first);
    std::vector<std::vector<int64_t>> infer_range_vec_res;
    (void)AttrUtils::GetListListInt(tensor_desc, ge::ATTR_NAME_DATA_SLICE, infer_range_vec_res);
    output_slice.emplace_back(infer_range_vec_res);
  }
  // call register to get special op infer func
  auto node_slice_infer_ptr = OperatorFactoryImpl::GetInferAxisSliceFunc(op->GetType());
  if (node_slice_infer_ptr != nullptr) {
    GELOGD("[DataSlice][Status] special node %s start infer axis slice", op->GetName().c_str());
    DataSliceType input_slice;
    Operator op_proxy = OpDescUtils::CreateOperatorFromOpDesc(op);
    const graphStatus ret = static_cast<graphStatus>(node_slice_infer_ptr(op_proxy,
        slice_info, output_slice, input_slice));
    if (ret != GRAPH_SUCCESS) {
      GELOGE(FAILED, "[DataSlice][Status]special node %s infer axis slice failed", op->GetName().c_str());
      return FAILED;
    }
    op_proxy.BreakConnect();
    if (SetInputSlice(op, slice_info, input_slice) != SUCCESS) {
      GELOGE(FAILED, "[DataSlice][Status]special node %s set axis slice failed", op->GetName().c_str());
      return FAILED;
    }
    return SUCCESS;
  }
  // call data slice factory to get axis infer func
  auto data_slice_infer_ptr = DataSliceFactory::GetInstance()->GetClassByAxisType(slice_info.GetAxisType());
  if (data_slice_infer_ptr != nullptr) {
    DataSliceType input_slice;
    Operator op_proxy = OpDescUtils::CreateOperatorFromOpDesc(op);
    if (data_slice_infer_ptr->InferAxisSlice(op_proxy, slice_info, output_slice, input_slice) != SUCCESS) {
      GELOGE(FAILED, "[DataSlice][Check] node: %s InferAxisSlice failed", op->GetName().c_str());
      return FAILED;
    }
    op_proxy.BreakConnect();
    if (SetInputSlice(op, slice_info, input_slice) != SUCCESS) {
      GELOGE(FAILED, "[DataSlice][Status]special node %s set axis slice failed", op->GetName().c_str());
      return FAILED;
    }
    return SUCCESS;
  }
  return FAILED;
}

// get op axis slice info
Status DataSliceHelper::GetSliceInfo(ge::OpDescPtr &op, std::vector<AxisTypeInfo> &axis_type_vec)
{
  // call register to get axis slice info
  auto axis_slice_info_ptr = OperatorFactoryImpl::GetInferAxisTypeInfoFunc(op->GetType());
  if (axis_slice_info_ptr == nullptr) {
    GELOGW("[DataSlice][Check] node: %s has no axis slice func.", op->GetName().c_str());
    return FAILED;
  }
  GELOGD("[DataSlice][Status] node %s get axis type info.", op->GetName().c_str());
  Operator op_proxy = OpDescUtils::CreateOperatorFromOpDesc(op);
  const graphStatus ret = static_cast<graphStatus>(axis_slice_info_ptr(op_proxy, axis_type_vec));
  if (ret != GRAPH_SUCCESS) {
    GELOGE(FAILED, "[DataSlice][Status]special node %s get axis slice failed", op->GetName().c_str());
    return FAILED;
  }
  op_proxy.BreakConnect();
  return SUCCESS;
}
}
