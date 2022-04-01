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
#include "hybrid/model/infer/shape_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/ge_tensor.h"
#include "common/debug/log.h"

namespace ge {
namespace hybrid {

Status ShapeUtils::CopyShapeAndTensorSize(const GeTensorDesc &from, GeTensorDesc &to) {
  const auto &shape = from.GetShape();
  const auto &origin_shape = from.GetOriginShape();
  to.SetShape(shape);
  to.SetOriginShape(origin_shape);
  int64_t tensor_size = -1;
  (void)TensorUtils::GetSize(from, tensor_size);
  if (tensor_size <= 0) {
    const auto format = to.GetFormat();
    const DataType data_type = to.GetDataType();
    if (TensorUtils::CalcTensorMemSize(shape, format, data_type, tensor_size) != GRAPH_SUCCESS) {
      GELOGE(FAILED, "CalcTensorMemSize failed for data type: [%d]  format [%d]  shape [%s]",
             data_type, format, shape.ToString().c_str());
      REPORT_CALL_ERROR("E19999", "CalcTensorMemSize failed for data type: [%d]  format [%d]  shape [%s]",
                        data_type, format, shape.ToString().c_str());
      return FAILED;
    }
  }
  GELOGD("Update input Shape: [%s] and OriginalShape: [%s], size = %ld",
         shape.ToString().c_str(),
         origin_shape.ToString().c_str(),
         tensor_size);

  TensorUtils::SetSize(to, tensor_size);
  return SUCCESS;
}
} // namespace hybrid
} // namespace ge
