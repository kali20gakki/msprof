/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "param_calculate/tensorsize_calculator.h"

#include "common/fe_error_code.h"
#include "common/fe_type_utils.h"
#include "common/comm_log.h"
#include "common/constants_define.h"
#include "common/op_tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"

namespace fe {

Status TensorSizeCalculator::CalculateOpTensorSize(ge::OpDesc &op_desc) {
  CM_LOGD("Begin to calculate input and output tenosor size of op [%s, %s].", op_desc.GetName().c_str(),
          op_desc.GetType().c_str());
  int32_t output_real_calc_flag = 0;
  (void)CalcInputOpTensorSize(op_desc, output_real_calc_flag);

  bool ret = ge::AttrUtils::GetInt(op_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag);
  CM_LOGD("FE -- get tensor_actual_size : [%d],[%d]", output_real_calc_flag, ret);

  (void)CalcOutputOpTensorSize(op_desc, output_real_calc_flag);
  CM_LOGD("Finish the calculation of op [%s, %s] tensor size.", op_desc.GetName().c_str(), op_desc.GetType().c_str());
  (void)op_desc.DelAttr(fe::ATTR_NAME_UNKNOWN_SHAPE);

  return SUCCESS;
}
Status TensorSizeCalculator::CalcSingleTensorSize(const ge::OpDesc &op_desc,
    const ge::GeTensorDescPtr &tensor_desc_ptr, const string &direction, size_t i, bool output_real_calc_flag,
    int64_t &tensor_size) {
  ge::DataType data_type = tensor_desc_ptr->GetDataType();
  auto shape = tensor_desc_ptr->MutableShape();
  std::vector<int64_t> dims = shape.GetDims();

  if (shape.IsUnknownShape()) {
    CM_LOGD("Tensor %s [%zu] of op[%s, %s] is dynamic shape, no need to calculate size.",
            direction.c_str(), i, op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return SUCCESS;
  }

  if (OpTensorUtils::CalcTensorSize(dims, data_type, output_real_calc_flag, tensor_size) != SUCCESS) {
    std::map<std::string, std::string> error_key_map;
    error_key_map[EM_OP_NAME] = op_desc.GetName();
    error_key_map[EM_OP_TYPE] = op_desc.GetType();
    ReportErrorMessage(EM_CAL_TENSOR_SIZE_FAILED, error_key_map);
    CM_LOGW("Fail to calculate %s [%zu] tensor size of op[%s, %s].", direction.c_str(),
            i, op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TensorSizeCalculator::CalcInputOpTensorSize(ge::OpDesc &op_desc, int32_t &output_real_calc_flag) {
  size_t input_size = op_desc.GetAllInputsSize();
  for (size_t i = 0; i < input_size; i++) {
    ge::GeTensorDescPtr tensor_desc_ptr = op_desc.MutableInputDesc(i);
    if (tensor_desc_ptr == nullptr) {
      continue;
    }

    bool value_skip = false;
    (void)ge::AttrUtils::GetBool(tensor_desc_ptr, "valueSkip", value_skip);
    if (value_skip) {
      CM_LOGD("The tensor size input[%zu] is 0 for valueSkip is true.", i);
      ge::TensorUtils::SetSize(*tensor_desc_ptr, 0);
      continue;
    }

    int64_t tensor_size = -1;
    Status result = CalcSingleTensorSize(op_desc, tensor_desc_ptr, "input", i, output_real_calc_flag, tensor_size);
    if (result != SUCCESS) {
      continue;
    }
    CM_LOGD("The tensor size input[%zu] is %ld.", i, tensor_size);
    ge::TensorUtils::SetSize(*tensor_desc_ptr, tensor_size);
  }
  return SUCCESS;
}

Status TensorSizeCalculator::CalcOutputOpTensorSize(ge::OpDesc &op_desc, int32_t &output_real_calc_flag) {
  size_t output_size = op_desc.GetOutputsSize();
  for (size_t i = 0; i < output_size; i++) {
    ge::GeTensorDescPtr tensor_desc_ptr = op_desc.MutableOutputDesc(i);
    if (tensor_desc_ptr == nullptr) {
      continue;
    }
    int64_t tensor_size = -1;
    Status result = SUCCESS;
    if (ge::AttrUtils::GetInt(tensor_desc_ptr, ge::ATTR_NAME_SPECIAL_OUTPUT_SIZE, tensor_size)) {
      if (!output_real_calc_flag) {
        result = OpTensorUtils::CalibrateTensorSize(tensor_size);
      }
    } else {
      result = CalcSingleTensorSize(op_desc, tensor_desc_ptr, "output", i, output_real_calc_flag, tensor_size);
    }

    if (result != SUCCESS) {
      continue;
    }
    CM_LOGD("The tensor size output[%zu] is %ld.", i, tensor_size);
    ge::TensorUtils::SetSize(*tensor_desc_ptr, tensor_size);
  }
  return SUCCESS;
}
}  // namespace fe
