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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_

#include <functional>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_utils.h"
#include "common/format/axis_util.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"

namespace fe {

struct CalcShapeExtraAttr {
  int64_t hidden_size;
  int64_t input_size;
  int64_t state_size;
};

/* The first parameter is axis value, second is new shape and third is
 * op implementation type. */
using GetNewShapeByAxisValueAndFormat =
    std::function<Status(ge::GeShape &, const int64_t &, vector<int64_t> &, vector<int64_t> &)>;

using GetNewShapeByAxisValueAndFormatPtr = std::shared_ptr<GetNewShapeByAxisValueAndFormat>;

struct ShapeAndFormatInfo {
  ge::GeShape old_shape;
  ge::GeShape &new_shape;
  ge::Format old_format;
  ge::Format new_format;
  ge::DataType current_data_type;
  int64_t op_impl_type;
  int64_t group_count;
  CalcShapeExtraAttr extra_attr;
  ShapeAndFormatInfo(ge::GeShape old_shape, ge::GeShape &new_shape, ge::Format old_format,
                     ge::Format new_format, ge::DataType current_data_type, int64_t op_impl_type,
                     int64_t group_count) :
          old_shape(old_shape), new_shape(new_shape), old_format(old_format), new_format(new_format),
          current_data_type(current_data_type), op_impl_type(op_impl_type), group_count(group_count),
          extra_attr({1, 1, -1}) {}
  ShapeAndFormatInfo(ge::GeShape old_shape, ge::GeShape &new_shape, ge::Format old_format,
                     ge::Format new_format, ge::DataType current_data_type, int64_t op_impl_type,
                     int64_t group_count, CalcShapeExtraAttr extra_attr) :
          old_shape(old_shape), new_shape(new_shape), old_format(old_format), new_format(new_format),
          current_data_type(current_data_type), op_impl_type(op_impl_type), group_count(group_count),
          extra_attr(extra_attr) {}
};

using ShapeAndFormat = struct ShapeAndFormatInfo;

uint32_t GetC0(ge::DataType dtype);

class ShapeTransferAccordingToFormat {
 public:
  ShapeTransferAccordingToFormat();

  ~ShapeTransferAccordingToFormat();

  ShapeTransferAccordingToFormat(const ShapeTransferAccordingToFormat &) = delete;

  ShapeTransferAccordingToFormat &operator=(const ShapeTransferAccordingToFormat &) = delete;

  static Status GetShapeAccordingToFormat(ShapeAndFormat &shape_and_format_info, int64_t *c = nullptr);

  /*----------Below is the function of getting new shape----------------------*/
  static Status GetNCHWShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                        const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetNHWCShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                        const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetNC1HWC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                           const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFzShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                      const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFzC04ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                         const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetHWCNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                        const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetCHWNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                        const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetC1HWNCoC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                             const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetNzShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                      const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetNDC1HWC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                            const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFz3DShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                        const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFz3DTransposeShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                 const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFzLstmShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                          const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetFznRNNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                          const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);

  static Status GetNewDimVec(ge::GeShape &new_shape, const int64_t &impl_type, const vector<int64_t> &axis_value,
                             const vector<int64_t> &nd_value);

  static Status GetNDRNNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                         const vector<int64_t> &axis_value, const vector<int64_t> &nd_value);
 private:
  /* map of GetAxisValueInfoByFormat, get axis value by different original
   * formats. */
  static const std::map<ge::Format, GetNewShapeByAxisValueAndFormatPtr> get_new_shape_func_map;

  static void SetRNNAttr(const ShapeAndFormat &shape_and_format_info, std::vector<int64_t> &axis_value);

  static int64_t GetAsisEnlargeValue(const int64_t& cin, const int64_t& cout, const int64_t& c0, const int64_t& group);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANSFER_SHAPE_ACCORDING_TO_FORMAT_H_
