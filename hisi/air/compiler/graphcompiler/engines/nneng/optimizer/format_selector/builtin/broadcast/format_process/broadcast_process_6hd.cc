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

#include "format_selector/builtin/broadcast/format_process/broadcast_process_6hd.h"

namespace fe {
bool BroadcastProcess6HD::CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) {
  auto input_format = input_arg.input_format_;
  auto input_dtype = input_arg.input_dtype_;
  auto input_shape = input_arg.input_shape_;
  // shape should be 5D
  if (CheckOriginShapeDimNum(input_shape, DIMENSION_NUM_FIVE)) {
    int64_t dim_value = 0;
    GetDimValue(C_AXIS_NAME, input_format, input_shape, dim_value);
    // axis C should be C0 aligned
    if (!CheckAxisIsAligned(input_dtype, dim_value, -1)) {
      FE_LOGD("Axis C [%ld] is not 16 aligned, input_formats=[%d], inputDtype=[%d], inputShape=[%s].", dim_value,
              input_format, input_dtype, GetShapeDims(input_shape).c_str());
      return false;
    }
  } else {
    FE_LOGD("The dim_num of the input_shape=[%s] is < %d.", GetShapeDims(input_shape).c_str(), DIMENSION_NUM_FIVE);
    return false;
  }
  return true;
}

bool BroadcastProcess6HD::CheckAllNonScalarInputs(const FormatProccessArgs &args) {
  auto input_formats_b = args.origin_info_ptr->input_formats;
  auto input_shapes = args.origin_info_ptr->input_shapes;
  // each shape should be 5D
  for (size_t i = 0; i < input_shapes.size(); i++) {
    if (!CheckOriginShapeDimNum(input_shapes[i], DIMENSION_NUM_FIVE)) {
      FE_LOGD("The dim_num of the input_shape[%zu] value[%s] is < %d.", i, GetShapeDims(input_shapes[i]).c_str(),
              DIMENSION_NUM_FIVE);
      return false;
    }
  }

  // axis c should not need broadcast, if need, not support 6HD
  if (CheckAxisNeedBroadcast(C_AXIS_NAME, input_formats_b, input_shapes)) {
    FE_LOGD("Axis C needs to broadcast.");
    return false;
  }
  return true;
}

REGISTER_FORMAT_PROCESS(BroadcastProcess6HD, OP_PATTERN_BROADCAST, FORMAT_NDC1HWC0, ge::FORMAT_NDC1HWC0);
}  // namespace fe
