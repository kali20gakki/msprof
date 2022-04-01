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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_TENSOR_COMPUTE_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_TENSOR_COMPUTE_UTIL_H_

#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/ge_tensor.h"
#include "graph/types.h"

namespace fe {
class TensorComputeUtil {
 public:
  /**
   * verify the data type, format and shape of this tensor
   * @param tensor_desc Tensor object
   * @return Status
   */
  static Status VerifyTensor(ge::GeTensorDesc &tensor_desc);

  /**
   * calculate the tensor element count by multiply all the dim of shape
   * @param tensor_desc Tensor object
   * @param element_cnt output value
   * @return Status
   */
  static Status GetElementCountByMultiply(const ge::GeTensorDesc &tensor_desc, int64_t &element_cnt);

  /**
   * calculate the tensor element count by multiply all the dim of shape
   * @param shape shape object
   * @param element_cnt output value
   * @return Status
   */
  static Status GetElementCountByMultiply(const ge::GeShape &shape, int64_t &element_cnt);

  /**
   *
   * @param element_cnt
   * @param data_type
   * @param tensor_size
   * @return
   */
  static Status GetTensorSizeByDataType(int64_t &element_cnt, ge::DataType &data_type, int64_t &tensor_size,
                                        int32_t &output_real_calc_flag);

 /**
  * Get the size of data type
  * @param data_type data type
  * @param data_type_size size of data type
  * @return Status
  */
  static Status GetDataTypeSize(ge::DataType &data_type, uint32_t &data_type_size);

  static Status CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size, int32_t &output_real_calc_flag);

 private:
  /**
   * Check whether the multiplication of a set of int64_t number a,b may exceed
   * the maximum of int64_t
   * And do the multiplication at the same time
   * @param nums a set of int64_t number
   * @param result multiplication result
   * @return Status
   */
  static Status ArrayMultiplyInt64WithVerify(const std::vector<int64_t> &nums, int64_t &result);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_TENSOR_COMPUTE_UTIL_H_
