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

#include "format_selector/builtin/reduce/reduce_format_selector/reduce_process_nz.h"
#include "common/util/op_info_util.h"

namespace fe {
const int kIntervalValue = 16;

Status ReduceProcessNz::Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args,
                                FormatProccessResult &result) {
  OriginInfoPtr origin_info_ptr = args.origin_info_ptr;
  vector<ge::Format> input_formats = origin_info_ptr->input_formats;
  vector<ge::DataType> input_dtypes = origin_info_ptr->input_dtypes;
  vector<ge::GeShape> input_shapes = origin_info_ptr->input_shapes;
  size_t input_shapes_size = input_shapes.size();
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  if (!CheckOriginFormatAndShape(input_formats, input_shapes, MINIMUM_NZ_SHAPE_DIM_NUM)) {
    FE_LOGD("Op[name=%s,type=%s]: check origin format and shape not success.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  // 2. check the axis attribute of the op_desc
  bool is_reduce_last = CheckContainReduceAxis(op_desc, input_formats, input_shapes, LAST_AXIS_NAME);
  bool is_reduce_lastbutone = CheckContainReduceAxis(op_desc, input_formats, input_shapes, LASTBUTONE_AXIS_NAME);

  vector<ge::Format> input_support_format;
  vector<ge::Format> output_support_format;
  ge::Format support_format = args.support_format;
  // don't reduce the last axis and lastbutone axis: NZ->NZ
  if (!is_reduce_last && !is_reduce_lastbutone) {
    input_support_format.emplace_back(support_format);
    output_support_format.emplace_back(support_format);
  } else {
    // the last axis and the lastbutone axis is aligned: NZ->ND
    if (CheckNzAxisIsAligned(op_desc, input_dtypes, input_shapes, is_reduce_last, is_reduce_lastbutone)) {
      input_support_format.emplace_back(support_format);
      output_support_format.emplace_back(ge::FORMAT_ND);
    }
  }

  // 3. generate the result
  if (input_support_format.empty()) {
    return FAILED;
  }
  GenerateFormats(input_shapes_size, origin_info_ptr->output_shapes.size(), input_support_format, output_support_format,
                  result);
  return SUCCESS;
}  // namespace fe

bool ReduceProcessNz::CheckOriginFormatAndShape(const vector<ge::Format> &formats, const vector<ge::GeShape> &shapes,
                                                const size_t &dim) {
  if (!CheckOriginShapesDimNum(shapes, dim)) {
    FE_LOGD("The shape size is < %zu.", dim);
    return false;
  }
  return true;
}

bool ReduceProcessNz::CheckNzAxisIsAligned(const ge::OpDesc &op_desc, const vector<ge::DataType> &dtypes,
                                           const vector<ge::GeShape> &shapes, const bool &is_reduce_last,
                                           const bool &is_reduce_lastbutone) const {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  size_t size = shapes.size();

  for (size_t i = 0; i != size; ++i) {
    if (is_reduce_last || is_reduce_lastbutone) {
      int64_t dim_num = static_cast<int64_t>(shapes[i].GetDimNum());
      int64_t last_dim_value = shapes[i].GetDim(dim_num - 1);
      int64_t lastbutone_dim_value = shapes[i].GetDim(dim_num - 2);
      if (!CheckAxisIsC0Aligned(dtypes[i], last_dim_value) || lastbutone_dim_value % kIntervalValue != 0) {
        return false;
      }
    }
  }
  return true;
}

bool ReduceProcessNz::CheckAxisIsC0Aligned(const ge::DataType &dtype, const int64_t &dim_value) const {
  int c0 = SHAPE_NUMBER_16;
  if (dtype < vector_of_dtype_and_c0.size()) {
    c0 = vector_of_dtype_and_c0[dtype];
  }
  return dim_value % c0 == 0;
}

REGISTER_FORMAT_PROCESS(ReduceProcessNz, OP_PATTERN_REDUCE, FORMAT_FRACTAL_NZ, ge::FORMAT_FRACTAL_NZ);
}  // namespace fe