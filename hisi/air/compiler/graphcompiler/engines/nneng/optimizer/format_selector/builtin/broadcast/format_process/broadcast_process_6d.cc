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

#include "format_selector/builtin/broadcast/format_process/broadcast_process_6d.h"

namespace fe {
bool BroadcastProcess6D::CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) {
  auto input_format = input_arg.input_format_;
  auto input_dtype = input_arg.input_dtype_;
  auto input_shape = input_arg.input_shape_;

  // shape should be 4D
  if (CheckOriginShapeDimNum(input_shape, DIM_DEFAULT_SIZE)) {
    int64_t dim_value_a = 0;
    // axis C should be C0 aligned
    GetDimValue(C_AXIS_NAME, input_format, input_shape, dim_value_a);
    if (!CheckAxisIsAligned(input_dtype, dim_value_a, -1)) {
      FE_LOGD("Axis C [%ld] is not C0 aligned, input_formats=[%d], inputDtype=[%d], inputShape=[%s].", dim_value_a,
              input_format, input_dtype, GetShapeDims(input_shape).c_str());
      return false;
    }

    // axis N should be NI aligned
    GetDimValue(N_AXIS_NAME, input_format, input_shape, dim_value_a);
    int64_t n = dim_value_a;
    if (!CheckAxisIsAligned(input_dtype, n, NI)) {
      FE_LOGD("Axis N [%ld] is not 16 aligned, input_format=[%d], input_dtype=[%d], inputShape=[%s].", n, input_format,
              input_dtype, GetShapeDims(input_shape).c_str());
      return false;
    }
  } else {
    FE_LOGD("The dim_num of the input_shape=[%s] is < %zu.", GetShapeDims(input_shape).c_str(), DIM_DEFAULT_SIZE);
    return false;
  }
  return true;
}

bool BroadcastProcess6D::CheckAllNonScalarInputs(const FormatProccessArgs &args) {
  auto current_input_formats = args.origin_info_ptr->input_formats;
  auto input_shapes = args.origin_info_ptr->input_shapes;
  for (size_t i = 0; i < input_shapes.size(); i++) {
    // each shape should be 4D
    if (!CheckOriginShapeDimNum(input_shapes[i], DIM_DEFAULT_SIZE)) {
      FE_LOGD("The dim_num of the input_shapes[%zu] value[%s] is < %zu.", i, GetShapeDims(input_shapes[i]).c_str(),
              DIM_DEFAULT_SIZE);
      return false;
    }
  }
  // axis C&N should not need broadcast, if need, not support 6D
  if (CheckAxisNeedBroadcast(C_AXIS_NAME, current_input_formats, input_shapes)) {
    FE_LOGD("Axis C needs to broadcast.");
    return false;
  }
  if (CheckAxisNeedBroadcast(N_AXIS_NAME, current_input_formats, input_shapes)) {
    FE_LOGD("Axis N needs to broadcast.");
    return false;
  }
  return true;
}

REGISTER_FORMAT_PROCESS(BroadcastProcess6D, OP_PATTERN_BROADCAST, FORMAT_C1HWNCoC0, ge::FORMAT_C1HWNCoC0);
}  // namespace fe
