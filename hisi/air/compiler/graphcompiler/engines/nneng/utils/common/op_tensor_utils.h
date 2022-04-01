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

#ifndef FUSION_ENGINE_UTILS_COMMON_OP_TENSOR_UTILS_H_
#define FUSION_ENGINE_UTILS_COMMON_OP_TENSOR_UTILS_H_

#include <vector>
#include "ops_store/op_kernel_info.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/types.h"
#include "graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {
class OpTensorUtils {
 public:
  static Status CalcTensorSize(const std::vector<int64_t> &dims, const ge::DataType &data_type,
                               int32_t output_real_calc_flag, int64_t &tensor_size);

  static Status CalcTensorSize(const ge::GeTensorDesc &tensor_desc,
                               const int32_t output_real_calc_flag, int64_t &tensor_size);

  static bool IsUnKnownShapeOp(const ge::OpDesc &op_desc, const bool &is_use_attr_check);

  static bool IsUnKnownShapeOp(const ge::OpDesc &op_desc);

  static bool IsFuzzBuildOp(const ge::OpDesc &op_desc);

  static bool IsFeSupportedDynamicOp(const ge::OpDesc &op_desc, bool is_use_attr_check);

  /**
  * Get the size of data type
  * @param data_type data type
  * @param data_type_size size of data type
  * @return Status
  */
  static Status GetDataTypeSize(const ge::DataType &data_type, uint32_t &data_type_size);

  static Status CalibrateTensorSize(int64_t &tensor_size);
 private:
  /**
   * verify the data type, format and shape of this tensor
   * @param tensor_desc Tensor object
   * @return Status
   */
  static Status VerifyTensor(const std::vector<int64_t> &dims, const ge::DataType &data_type);

  /**
   * Check whether the multiplication of a set of int64_t number a,b may exceed
   * the maximum of int64_t
   * And do the multiplication at the same time
   * @param nums a set of int64_t number
   * @param result multiplication result
   * @return Status
   */
  static Status ArrayMultiplyInt64WithVerify(const std::vector<int64_t> &dims, int64_t &result);

  static bool IsUnKnownShapeTensor(const ge::OpDesc &op_desc);
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_COMMON_OP_TENSOR_UTILS_H_
