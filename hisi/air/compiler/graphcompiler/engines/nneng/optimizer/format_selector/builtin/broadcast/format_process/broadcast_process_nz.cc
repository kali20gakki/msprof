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

#include "format_selector/builtin/broadcast/format_process/broadcast_process_nz.h"

namespace fe {
bool BroadcastProcessNz::CheckOriginFormat(const std::vector<ge::Format> &formats,
                                           const std::vector<ge::GeShape> &input_shapes) {
  return true;
}

bool BroadcastProcessNz::CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) {
  auto input_dtype = input_arg.input_dtype_;
  auto input_shape = input_arg.input_shape_;

  // shape dim should be larger than 2D
  if (CheckOriginShapeDimNum(input_shape, MINIMUM_NZ_SHAPE_DIM_NUM)) {
    // the last axis should be c0 aligned
    // the penultimate axis should be 16 aliged
    bool last_dim_c0_aligned =
        CheckAxisIsAligned(input_dtype, input_shape.GetDim(input_shape.GetDimNum() - LAST_DIM), -1);
    bool penultimate_dim_16_aligned =
        CheckAxisIsAligned(input_dtype, input_shape.GetDim(input_shape.GetDimNum() - PENULTIMATE_DIM), SHAPE_NUMBER_16);
    if (!last_dim_c0_aligned || !penultimate_dim_16_aligned) {
      FE_LOGD("The last two axises are not aligned, input_dtype=[%d], inputShape=[%s].", input_dtype,
              GetShapeDims(input_shape).c_str());
      return false;
    }
  } else {
    FE_LOGD("The dim_num of the input_shape=[%s] is < %d.", GetShapeDims(input_shape).c_str(),
            MINIMUM_NZ_SHAPE_DIM_NUM);
    return false;
  }
  return true;
}

bool BroadcastProcessNz::CheckAllNonScalarInputs(const FormatProccessArgs &args) {
  auto input_shapes = args.origin_info_ptr->input_shapes;
  // each shape should be >=2
  for (size_t i = 0; i < input_shapes.size(); i++) {
    if (!CheckOriginShapeDimNum(input_shapes[i], MINIMUM_NZ_SHAPE_DIM_NUM)) {
      FE_LOGD("The dim_num of the input_shape[%zu] value[%s] is < %d.", i, GetShapeDims(input_shapes[i]).c_str(),
              MINIMUM_NZ_SHAPE_DIM_NUM);
      return false;
    }
  }

  // the last two axis should not need broadcast
  if (CheckAxisNeedBroadcast(LAST_DIM, input_shapes)) {
    FE_LOGD("The last axis needs to broadcast.");
    return false;
  }
  if (CheckAxisNeedBroadcast(PENULTIMATE_DIM, input_shapes)) {
    FE_LOGD("The penultimate axis needs to broadcast.");
    return false;
  }
  return true;
}

bool BroadcastProcessNz::CheckAxisNeedBroadcast(const size_t &index, const std::vector<ge::GeShape> &shapes) const {
  if (shapes.empty()) {
    return false;
  }
  int64_t dim_value = shapes[0].GetDim(shapes[0].GetDimNum() - index);
  for (size_t i = 1; i < shapes.size(); i++) {
    int64_t tmp_dim_value = shapes[i].GetDim(shapes[i].GetDimNum() - index);
    if (dim_value != tmp_dim_value) {
      return true;
    }
  }
  return false;
}

REGISTER_FORMAT_PROCESS(BroadcastProcessNz, OP_PATTERN_BROADCAST, FORMAT_FRACTAL_NZ, ge::FORMAT_FRACTAL_NZ);
}  // namespace fe
