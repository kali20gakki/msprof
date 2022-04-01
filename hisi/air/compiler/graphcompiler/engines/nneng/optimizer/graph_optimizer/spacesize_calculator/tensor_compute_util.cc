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

#include "graph_optimizer/spacesize_calculator/tensor_compute_util.h"

#include <cmath>
#include "common/configuration.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"

namespace fe {
Status TensorComputeUtil::VerifyTensor(ge::GeTensorDesc &tensor_desc) {
  auto format = ge::GetPrimaryFormat(tensor_desc.GetFormat());
  if (format < ge::FORMAT_NCHW || format >= ge::FORMAT_END) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][VerifyTensor] The format of this tensor is invalid.");
    return INVALID_TENSOR_FORMAT;
  }
  if (tensor_desc.GetDataType() < ge::DT_FLOAT || tensor_desc.GetDataType() == ge::DT_UNDEFINED
      || tensor_desc.GetDataType() >= ge::DT_MAX) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][VerifyTensor] The data type of this tensor is invalid.");
    return INVALID_TENSOR_DATATYPE;
  }

  std::vector<int64_t> dims = tensor_desc.MutableShape().GetDims();
  if (!dims.empty()) {
    for (auto dim : dims) {
      if (dim < 0) {
        FE_LOGW("The dim value[%ld] is invalid.", dim);
        return INVALID_DIM_VALUE;
      }
    }
  }

  return SUCCESS;
}

Status TensorComputeUtil::GetElementCountByMultiply(const ge::GeTensorDesc &tensor_desc, int64_t &element_cnt) {
  const ge::GeShape &shape = tensor_desc.GetShape();
  if (GetElementCountByMultiply(shape, element_cnt) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][GetElemCount] Fail to get element count by multiply all the dims.");
    return FAILED;
  }
  return SUCCESS;
}

Status TensorComputeUtil::GetElementCountByMultiply(const ge::GeShape &shape, int64_t &element_cnt) {
  element_cnt = 1;  // initial value
  std::vector<int64_t> dim_vec = shape.GetDims();
  if (dim_vec.empty()) {
    return SUCCESS;
  }
  if (ArrayMultiplyInt64WithVerify(dim_vec, element_cnt) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][GetElemCount] The multiplication of int64 vector is overflow.");
    return MUL_OVERFLOW_INT64;
  }
  return SUCCESS;
}

Status TensorComputeUtil::GetTensorSizeByDataType(int64_t &element_cnt, ge::DataType &data_type, int64_t &tensor_size,
                                                  int32_t &output_real_calc_flag) {
  FE_LOGD("The element count is %ld, data type is %s.", element_cnt,
          ge::TypeUtils::DataTypeToSerialString(data_type).c_str());
  if (element_cnt <= 0) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][GetTensorSize] The count of element should be nonnegative integer.");
    return FAILED;
  }
  tensor_size = ge::GetSizeInBytes(element_cnt, data_type);
  if (tensor_size < 0) {
    REPORT_FE_ERROR("GetSizeInBytes failed!");
    return FAILED;
  }
  if (!output_real_calc_flag) {
    tensor_size = (tensor_size + DATA_MEMORY_ALIGN_SIZE - 1) / DATA_MEMORY_ALIGN_SIZE;
    FE_INT64_MULCHECK(tensor_size, DATA_MEMORY_ALIGN_SIZE);
    tensor_size *= DATA_MEMORY_ALIGN_SIZE;
    tensor_size += DATA_MEMORY_ALIGN_SIZE;
  }

  return SUCCESS;
}

Status TensorComputeUtil::ArrayMultiplyInt64WithVerify(const std::vector<int64_t> &nums, int64_t &result) {
  if (nums.empty()) {
    return VECTOR_INT64_EMPTY;
  }
  result = 1;  // Initial value
  for (int64_t num : nums) {
    if (num == 0) {
      FE_LOGD("num = 0, return 1");
      result = 1;
      return SUCCESS;
    }
    FE_INT64_MULCHECK(result, num);
    result *= num;
  }
  return SUCCESS;
}

Status TensorComputeUtil::CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size,
                                         int32_t &output_real_calc_flag) {
  // verify the tensor
  if (VerifyTensor(tensor_desc) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize] Fail to verify this tensor.");
    return FAILED;
  }

  int64_t element_cnt;
  if (GetElementCountByMultiply(tensor_desc, element_cnt) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize] Fail to calculate tensor size.");
    return FAILED;
  }
  ge::DataType data_type = tensor_desc.GetDataType();
  if (GetTensorSizeByDataType(element_cnt, data_type, tensor_size, output_real_calc_flag) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize] Fail to get tensor size by element count and datatype.");
    return FAILED;
  }
  return SUCCESS;
}

}  // namespace fe
