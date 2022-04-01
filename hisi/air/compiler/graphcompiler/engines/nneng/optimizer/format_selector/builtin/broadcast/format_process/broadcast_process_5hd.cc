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

#include "format_selector/builtin/broadcast/format_process/broadcast_process_5hd.h"

namespace fe {
bool BroadcastProcess5HD::CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) {
  auto input_format = input_arg.input_format_;
  auto input_dtype = input_arg.input_dtype_;
  auto input_shape = input_arg.input_shape_;
  // shape should be 4D
  if (CheckOriginShapeDimNum(input_shape, DIM_DEFAULT_SIZE)) {
    int64_t dim_value = 0;
    GetDimValue(C_AXIS_NAME, input_format, input_shape, dim_value);
    // axis c should be C0 aligned
    if (!CheckAxisIsAligned(input_dtype, dim_value, -1)) {
      FE_LOGD("Axis C [%ld] is not 16 aligned, input_formats=[%d], inputDtype=[%d], inputShape=[%s].", dim_value,
              input_format, input_dtype, GetShapeDims(input_shape).c_str());
      return false;
    }
  } else {
    FE_LOGD("The dim_num of the input_shape=[%s] is < %zu.", GetShapeDims(input_shape).c_str(), DIM_DEFAULT_SIZE);
    return false;
  }
  return true;
}

bool BroadcastProcess5HD::CheckAllNonScalarInputs(const FormatProccessArgs &args) {
  auto input_formats = args.origin_info_ptr->input_formats;
  auto current_input_shapes = args.origin_info_ptr->input_shapes;

  // each shape should be 4D
  for (size_t i = 0; i < current_input_shapes.size(); i++) {
    if (!CheckOriginShapeDimNum(current_input_shapes[i], DIM_DEFAULT_SIZE)) {
      FE_LOGD("The dim_num of the input_shape[%zu] value[%s] is < %zu.",
              i, GetShapeDims(current_input_shapes[i]).c_str(),
              DIM_DEFAULT_SIZE);
      return false;
    }
  }

  // axis c should not need broadcast, if need, not support 5HD
  if (CheckAxisNeedBroadcast(C_AXIS_NAME, input_formats, current_input_shapes)) {
    FE_LOGD("Axis C needs to broadcast.");
    return false;
  }
  return true;
}

REGISTER_FORMAT_PROCESS(BroadcastProcess5HD, OP_PATTERN_BROADCAST, FORMAT_NC1HWC0, ge::FORMAT_NC1HWC0);
}  // namespace fe
