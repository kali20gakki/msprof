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

#include "common/op_tensor_utils.h"

#include <cmath>
#include "common/comm_error_codes.h"
#include "common/comm_log.h"
#include "common/common_utils.h"
#include "common/constants_define.h"
#include "common/string_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/ge_context.h"

namespace fe {
// The align size of data memory
static const uint32_t DATA_MEMORY_ALIGN_SIZE = 32;

Status OpTensorUtils::VerifyTensor(const std::vector<int64_t> &dims, const ge::DataType &data_type) {
  if (data_type < ge::DT_FLOAT || data_type == ge::DT_UNDEFINED || data_type >= ge::DT_MAX) {
    REPORT_CM_ERROR("[GraphOpt][OptWholeGph][VerfyTensor] The data type of this tensor is invalid.");
    return TENSOR_DATATYPE_INVALID;
  }

  if (!dims.empty()) {
    for (const int64_t &dim : dims) {
      if (dim < 0) {
        REPORT_CM_ERROR("[GraphOpt][OptWholeGph][VerfyTensor] The dim value[%ld] is invalid.", dim);
        return DIM_VALUE_INVALID;
      }
    }
  }

  return SUCCESS;
}

Status OpTensorUtils::GetDataTypeSize(const ge::DataType &data_type, uint32_t &data_type_size) {
  int res = ge::GetSizeByDataType(data_type);
  if (res == -1) {
    return TENSOR_DATATYPE_NOT_SUPPORT;
  }
  data_type_size = static_cast<uint32_t>(res);
  return SUCCESS;
}

Status OpTensorUtils::ArrayMultiplyInt64WithVerify(const std::vector<int64_t> &dims, int64_t &result) {
  result = 1;  // Initial value
  if (dims.empty()) {
    return SUCCESS;
  }
  for (const int64_t &num : dims) {
    if (num == 0) {
      CM_LOGD("num = 0, return 1");
      result = 1;
      return SUCCESS;
    }
    CM_INT64_MULCHECK(result, num);
    result *= num;
  }
  return SUCCESS;
}
Status OpTensorUtils::CalibrateTensorSize(int64_t &tensor_size) {
  tensor_size = (tensor_size + DATA_MEMORY_ALIGN_SIZE - 1) / DATA_MEMORY_ALIGN_SIZE;
  CM_INT64_MULCHECK(tensor_size, DATA_MEMORY_ALIGN_SIZE);
  tensor_size *= DATA_MEMORY_ALIGN_SIZE;
  CM_INT64_ADDCHECK(tensor_size, DATA_MEMORY_ALIGN_SIZE);
  tensor_size += DATA_MEMORY_ALIGN_SIZE;
  return SUCCESS;
}

Status OpTensorUtils::CalcTensorSize(const std::vector<int64_t> &dims, const ge::DataType &data_type,
                                     int32_t output_real_calc_flag, int64_t &tensor_size) {
  // verify the tensor
  if (VerifyTensor(dims, data_type) != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][OptWholeGph][CalcTensorSize] Fail to verify this tensor.");
    return FAILED;
  }

  int64_t element_cnt;
  if (ArrayMultiplyInt64WithVerify(dims, element_cnt) != SUCCESS) {
    tensor_size = INT64_MAX;
    CM_LOGW("Tensor size is larger than the upper bound of int64, using INT64_MAX as the tensor size.");
    return SUCCESS;
  }

  tensor_size = ge::GetSizeInBytes(element_cnt, data_type);
  if (tensor_size < 0) {
    REPORT_FE_ERROR("GetSizeInBytes failed!");
    return FAILED;
  }
  if (!output_real_calc_flag) {
    return CalibrateTensorSize(tensor_size);
  }

  return SUCCESS;
}

Status OpTensorUtils::CalcTensorSize(const ge::GeTensorDesc &tensor_desc,
                                     const int32_t output_real_calc_flag, int64_t &tensor_size) {
  std::vector<int64_t> dims = tensor_desc.GetShape().GetDims();
  ge::DataType data_type = tensor_desc.GetDataType();
  return CalcTensorSize(dims, data_type, output_real_calc_flag, tensor_size);
}

bool OpTensorUtils::IsUnKnownShapeTensor(const ge::OpDesc &op_desc) {
  for (auto &tenosr_desc_ptr : op_desc.GetAllInputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      CM_LOGI("Op[%s, type %s] is unknown shape op. Its input is unknown.", op_desc.GetName().c_str(),
              op_desc.GetName().c_str());
      return true;
    }
  }

  for (auto &tenosr_desc_ptr : op_desc.GetAllOutputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      CM_LOGI("Op[%s, type %s] is unknown shape op. Its output is unknown.", op_desc.GetName().c_str(),
              op_desc.GetName().c_str());
      return true;
    }
  }
  return false;
}

bool OpTensorUtils::IsUnKnownShapeOp(const ge::OpDesc &op_desc, const bool &is_use_attr_check) {
  const ge::OpDesc *tmp_op_desc = &op_desc;
  ge::OpDesc *no_const_op_desc = const_cast<ge::OpDesc *>(tmp_op_desc);
  bool is_unknow_shape = false;
  if (is_use_attr_check) {
    if (ge::AttrUtils::GetBool(op_desc, ATTR_NAME_UNKNOWN_SHAPE, is_unknow_shape)) {
      return is_unknow_shape;
    }
  }

  if (op_desc.GetAllInputsSize() != 0 || op_desc.GetOutputsSize() != 0) {
    is_unknow_shape = IsUnKnownShapeTensor(op_desc);
  }

  if (is_use_attr_check) {
    (void)ge::AttrUtils::SetBool(*no_const_op_desc, fe::ATTR_NAME_UNKNOWN_SHAPE, is_unknow_shape);
    CM_LOGD("Op[%s, type %s] Set attr unknown_shape [%d].", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
            is_unknow_shape);
  }
  return is_unknow_shape;
}

bool OpTensorUtils::IsUnKnownShapeOp(const ge::OpDesc &op_desc) {
  return IsUnKnownShapeOp(op_desc, false);
}

bool OpTensorUtils::IsFuzzBuildOp(const ge::OpDesc &op_desc) {
  std::string build_mode;
  bool support_dyn_shape = false;
  if (ge::GetContext().GetOption("ge.shape_generalized_build_mode", build_mode) && (build_mode == SHAPE_GENERALIZED) &&
    ge::AttrUtils::GetBool(op_desc, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, support_dyn_shape) && support_dyn_shape) {
    return true;
  } else {
    return false;
  }
}

bool OpTensorUtils::IsFeSupportedDynamicOp(const ge::OpDesc &op_desc, bool is_use_attr_check) {
  return (IsUnKnownShapeOp(op_desc, is_use_attr_check) || IsFuzzBuildOp(op_desc));
}

}  // namespace fe
