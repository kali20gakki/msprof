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

#include "format_selector/builtin/broadcast/format_process/broadcast_format_process.h"
#include "common/op_info_common.h"
#include "graph/utils/type_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
Status BroadcastFormatProcess::Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args,
                                       FormatProccessResult &result) {
  auto support_format = args.support_format;
  auto input_formats = args.origin_info_ptr->input_formats;
  auto input_dtypes = args.origin_info_ptr->input_dtypes;
  auto input_shapes = args.origin_info_ptr->input_shapes;
  auto output_shapes = args.origin_info_ptr->output_shapes;
  auto propagat_primary_format = args.propagat_primary_format;
  auto propagat_sub_format = args.propagat_sub_format;

  auto op_name = op_desc.GetName();
  auto op_type = op_desc.GetType();

  // 1. check the origin format
  if (!CheckOriginFormat(input_formats, input_shapes)) {
    FE_LOGW("Op[name%s,optype[%s]]:  all inputs are different in format.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  // 2. check the origin shape
  size_t input_size = op_desc.GetAllInputsSize();
  size_t output_size = op_desc.GetOutputsSize();
  if (CheckOriginShape(input_shapes)) {
    FE_LOGD("Op[name=%s,type=%s]: all input_descs have the same origin shape.", op_name.c_str(), op_type.c_str());
    /* 2.1 For 6HD, we need the original shape to be 5D although the two
     * shape is same. The reason is there is no padding mechanism from less
     * than 5D to 5D. */
    size_t dim_value = 0;
    if (Check6HDShape(input_shapes, support_format, dim_value) != SUCCESS) {
      FE_LOGD("Op[name=%s,type=%s]: dim size %zu should > 5 for 6HD.", op_name.c_str(), op_type.c_str(), dim_value);
      return SUCCESS;
    }
    ge::Format same_format = (CheckScalarExists(input_shapes) ? ge::FORMAT_ND : support_format);
    InsertFormatVec(input_size, same_format, result.input_format_res);
    InsertFormatVec(output_size, same_format, result.output_format_res);
    return SUCCESS;
  }

  // 3. has scalar inputs
  if (CheckScalarExists(input_shapes)) {
    vector<ge::Format> input_support_formats;
    for (size_t i = 0; i < input_shapes.size(); ++i) {
      if (IsScalarInput(input_shapes[i])) {
        FE_LOGD("Op[name=%s,type=%s]: input %zu is scalar.", op_name.c_str(), op_type.c_str(), i);
        input_support_formats.emplace_back(ge::FORMAT_ND);
        continue;
      }

      FormatProccessInputArg input_arg(input_formats[i], input_dtypes[i], input_shapes[i], propagat_primary_format,
                                       propagat_sub_format);
      if (!CheckPartNonScalarInputs(input_arg)) {
        FE_LOGD("Op[name=%s,type=%s]: check non-scalar input [%zu] not success.", op_name.c_str(), op_type.c_str(), i);
        return FAILED;
      }
      FE_LOGD("Op[name=%s,type=%s]: input %zu is non-scalar.", op_name.c_str(), op_type.c_str(), i);
      input_support_formats.emplace_back(support_format);
    }

    result.input_format_res.emplace_back(input_support_formats);
    vector<ge::Format> output_support_formats;
    GenerateOutputFormats(output_shapes, support_format, output_support_formats);
    result.output_format_res.emplace_back(output_support_formats);
    return SUCCESS;
  }

  // 4. has no scalar inputs
  if (!CheckAllNonScalarInputs(args)) {
    FE_LOGD("Op[name=%s,type=%s]: check all non-scalar inputs not success.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  InsertFormatVec(input_size, support_format, result.input_format_res);
  vector<ge::Format> out_support_formats;
  GenerateOutputFormats(output_shapes, support_format, out_support_formats);
  result.output_format_res.emplace_back(out_support_formats);
  return SUCCESS;
}

bool IsFormatValid(ge::Format &first_idtf_format, const ge::Format &current_format, const ge::GeShape &current_shape) {
  bool is_scalar = IsScalarInput(current_shape);
  if (current_format == ge::FORMAT_ND) {
    return is_scalar;
  } else {
    if (is_scalar) {
      return true;
    } else {
      if (first_idtf_format == ge::FORMAT_RESERVED) {
        first_idtf_format = current_format;
        return true;
      } else {
        return first_idtf_format == current_format;
      }
    }
  }
}

bool BroadcastFormatProcess::CheckOriginFormat(const std::vector<ge::Format> &formats,
                                               const vector<ge::GeShape> &input_shapes) {
  // formats must be identifiable and same
  if (formats.empty()) {
    FE_LOGD("The formats is empty.");
    return false;
  }
  if (input_shapes.empty()) {
    FE_LOGD("The input shapes is empty.");
    return false;
  }
  if (input_shapes.size() != formats.size()) {
    FE_LOGD("Sizes of input shapes and format are not equal.");
    return false;
  }
  ge::Format first_format = ge::FORMAT_RESERVED;
  uint32_t index = 0;
  for (auto format : formats) {
    if (!IsFormatValid(first_format, format, input_shapes[index])) {
      FE_LOGD("The format %d is different from the first format %d.", format, first_format);
      return false;
    }
    index++;
  }
  /* If there is no (non-scalar && non-ND) tensors, the first_format will not
   * be set, we just return false. */
  return !(first_format == ge::FORMAT_RESERVED);
}

bool BroadcastFormatProcess::CheckOriginShape(const std::vector<ge::GeShape> &shapes) {
  // shape must be same
  if (shapes.empty()) {
    return false;
  }
  ge::GeShape first_shape = shapes.front();
  for (auto iter = shapes.begin(); iter != shapes.end(); iter++) {
    if (!IsSameShape(first_shape, *iter)) {
      return false;
    }
  }
  return true;
}

// If we need to support format 6HD, we need to make sure the shape
// is 5D.
Status BroadcastFormatProcess::Check6HDShape(const std::vector<ge::GeShape> &shapes, const ge::Format &supprt_format,
                                             size_t &dim_value) const {
  if (!(supprt_format == ge::FORMAT_NDC1HWC0 || supprt_format == ge::FORMAT_FRACTAL_Z_3D)) {
    return SUCCESS;
  }
  if (shapes.empty()) {
    REPORT_FE_ERROR("[GraphOpt][GenBuiltInFmt][ChkUbSizeEnHDShp] The parameter[shapes] is empty.");
    return FAILED;
  }

  /* !!!!Attention!!!!: Because now this function is invoked when every input
   * shape is the same, so here only need to judge the first input shape. */
  ge::GeShape first_shape = shapes.front();
  dim_value = first_shape.GetDimNum();
  if (dim_value < DIMENSION_NUM_FIVE) {
    return FAILED;
  }
  return SUCCESS;
}

bool BroadcastFormatProcess::CheckScalarExists(const std::vector<ge::GeShape> &shapes) const {
  for (ge::GeShape shape : shapes) {
    if (IsScalarInput(shape)) {
      return true;
    }
  }
  return false;
}

Status BroadcastFormatProcess::GetDimValue(const std::string &dim, const ge::Format &format, const ge::GeShape &shape,
                                           int64_t &dim_value) const {
  int64_t axis_n = GetAxisIndexByFormat(format, N_AXIS_NAME);
  if (axis_n == -1) {
    return FAILED;
  }
  int64_t axis_c = GetAxisIndexByFormat(format, C_AXIS_NAME);
  if (axis_c == -1) {
    return FAILED;
  }

  if (dim == N_AXIS_NAME) {
    dim_value = shape.GetDim(axis_n);
  } else if (dim == C_AXIS_NAME) {
    dim_value = shape.GetDim(axis_c);
  } else {
    REPORT_FE_ERROR("[GraphOpt][GenBuiltInFmt][GetDimVal] Node's axis[%s] is invalid.", dim.c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool BroadcastFormatProcess::CheckAxisIsAligned(const ge::DataType &dtype, const int64_t &dim_value,
                                                const int64_t &shape_number) const {
  if (shape_number == -1) {
    uint32_t c0 = GetC0(dtype);
    if (c0 == 0) {
      return false;
    }
    return dim_value % c0 == 0;
  }
  return dim_value % shape_number == 0;
}

bool BroadcastFormatProcess::CheckAxisNeedBroadcast(const std::string &dim, const std::vector<ge::Format> &formats,
                                                    const std::vector<ge::GeShape> &shapes) const {
  if (formats.empty()) {
    REPORT_FE_ERROR(
        "[GraphOpt][GenBuiltInFmt][ChkAxisNeedBrdct] The parameter[formats] is empty, no need to do broadcast.");
    return false;
  }

  if (shapes.empty()) {
    REPORT_FE_ERROR(
        "[GraphOpt][GenBuiltInFmt][ChkAxisNeedBrdct] The parameter[shapes] is empty, no need to do broadcast.");
    return false;
  }

  int64_t input_desc_num = formats.size();
  if (input_desc_num == 1) {
    FE_LOGD("The size of formats is 1, no need to do broadcast.");
    return false;
  }

  int64_t shape_num = shapes.size();
  if (input_desc_num != shape_num) {
    REPORT_FE_ERROR("[GraphOpt][GenBuiltInFmt][ChkAxisNeedBrdct] The size of shape and format are not equal.");
    return false;
  }

  int64_t dim_value = 0;
  GetDimValue(dim, formats.front(), shapes.front(), dim_value);

  for (int64_t i = 1; i < input_desc_num; ++i) {
    int64_t tmp_dim_value = 0;
    GetDimValue(dim, formats[i], shapes[i], tmp_dim_value);
    if (dim_value == SHAPE_UNKNOWN_DIM && tmp_dim_value == SHAPE_UNKNOWN_DIM) {
      FE_LOGD("Both Axis[%s] of two shape is -1, need to broadcast.", dim.c_str());
      return true;
    }
    if (dim_value != tmp_dim_value) {
      FE_LOGD("Axis[%s] is not equal(dim_value=%ld, tmp_dim_value=[%ld]), need to broadcast.", dim.c_str(), dim_value,
              tmp_dim_value);
      return true;
    }
  }
  FE_LOGD("Axis[%s] is equal, no need to broadcast.", dim.c_str());
  return false;
}

void BroadcastFormatProcess::GenerateOutputFormats(const vector<ge::GeShape> &output_shapes, const ge::Format &format,
                                                   vector<ge::Format> &output_formats) const {
  for (auto shape : output_shapes) {
    if (IsScalarInput(shape)) {
      output_formats.push_back(ge::FORMAT_ND);
    } else {
      output_formats.push_back(format);
    }
  }
}

void BroadcastFormatProcess::InsertFormatVec(const size_t &size, const ge::Format &format,
                                             vector<vector<ge::Format>> &formats) const {
  vector<ge::Format> format_vec(size, format);
  formats.emplace_back(format_vec);
}
}  // namespace fe
