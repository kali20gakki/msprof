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

#include "format_selector/builtin/reduce/reduce_format_selector/reduce_format_process.h"

namespace fe {

bool ReduceFormatProcess::CheckOriginFormatAndShape(const vector<ge::Format> &formats,
                                                    const vector<ge::GeShape> &shapes, const size_t &dim) {
  if (!CheckAccuracyOriginShapesDimNum(shapes, dim)) {
    FE_LOGD("The size of the origin shapes is != %zu.", dim);
    return false;
  }

  if (!CheckOriginFormatsIdentifiable(formats)) {
    FE_LOGD("The origin formats are not identifiable.");
    return false;
  }

  return true;
}

bool ReduceFormatProcess::CheckContainReduceAxis(const ge::OpDesc &op_desc, const vector<ge::Format> &formats,
                                                 const vector<ge::GeShape> &shapes, const string &axis_name) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  size_t size = formats.size();

  if (shapes.size() != size) {
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    vector<int64_t> axis_index_vec;
    if (AxisUtil::GetOriginAxisAttribute(op_desc, shapes[i], axis_index_vec) != SUCCESS) {
      FE_LOGW("Op[%s,optype[%s]]: can not get origin axis attribute.", op_name.c_str(), op_type.c_str());
      return true;
    }

    int64_t axis_tobe_check = -1;
    if (GetOriginAxisIndexByName(formats[i], shapes[i], axis_name, axis_tobe_check) != SUCCESS) {
      FE_LOGW("Op[%s,optype[%s]]: can not get %s!", op_name.c_str(), op_type.c_str(), axis_name.c_str());
      return true;
    }

    for (const auto &axis_index : axis_index_vec) {
      if (axis_tobe_check == axis_index) {
        return true;
      }
    }
  }
  return false;
}

Status ReduceFormatProcess::GetOriginAxisIndexByName(const ge::Format &format, const ge::GeShape &shape,
                                                     const string &axis_name, int64_t &axis_index) {
  if (axis_name == N_AXIS_NAME) {
    axis_index = GetAxisIndexByFormat(format, N_AXIS_NAME);
  } else if (axis_name == C_AXIS_NAME) {
    axis_index = GetAxisIndexByFormat(format, C_AXIS_NAME);
  } else if (axis_name == LAST_AXIS_NAME) {
    axis_index = shape.GetDimNum() - 1;
  } else if (axis_name == LASTBUTONE_AXIS_NAME) {
    axis_index = shape.GetDimNum() - 2;
  }
  if (axis_index < 0) {
    return FAILED;
  }

  FE_LOGD("origin axis index by axis_name %s is: %ld.", axis_name.c_str(), axis_index);
  return SUCCESS;
}

void ReduceFormatProcess::GenerateFormats(const size_t &in_shape_size, const size_t &out_shape_size,
                                          const vector<ge::Format> &support_in_formats,
                                          const vector<ge::Format> &support_out_formats, FormatProccessResult &result)
                                          const {
  for (const auto &support_format : support_in_formats) {
    vector<ge::Format> tmp(in_shape_size, support_format);
    result.input_format_res.emplace_back(tmp);
  }

  for (const auto &support_format : support_out_formats) {
    vector<ge::Format> tmp(out_shape_size, support_format);
    result.output_format_res.emplace_back(tmp);
  }
}
}  // namespace fe
