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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_RANGE_FORMAT_TRANSFER_TRANSFER_RANGE_ACCORDING_TO_FORMAT_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_RANGE_FORMAT_TRANSFER_TRANSFER_RANGE_ACCORDING_TO_FORMAT_H_

#include <functional>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_utils.h"
#include "common/format/axis_util.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {

/* The first parameter is axis value, second is new shape and third is
 * op implementation type. */
using GetNewRangeByAxisValueAndFormat =
    std::function<Status(vector<std::pair<int64_t, int64_t>> &, const int64_t &, vector<std::pair<int64_t, int64_t>> &,
                         vector<std::pair<int64_t, int64_t>> &)>;

using GetNewRangeByAxisValueAndFormatPtr = std::shared_ptr<GetNewRangeByAxisValueAndFormat>;

struct RangeAndFormatInfo {
  ge::GeShape old_shape;
  vector<std::pair<int64_t, int64_t>> old_range;
  vector<std::pair<int64_t, int64_t>> &new_range;
  ge::Format old_format;
  ge::Format new_format;
  ge::DataType current_data_type;
  int64_t op_impl_type;
  CalcShapeExtraAttr extra_attr;
  RangeAndFormatInfo(ge::GeShape old_shape, vector<std::pair<int64_t, int64_t>> old_range,
                     vector<std::pair<int64_t, int64_t>> &new_range, ge::Format old_format,
                     ge::Format new_format, ge::DataType current_data_type, int64_t op_impl_type) :
          old_shape(old_shape), old_range(old_range), new_range(new_range), old_format(old_format),
          new_format(new_format), current_data_type(current_data_type), op_impl_type(op_impl_type),
          extra_attr(CalcShapeExtraAttr()) {}
  RangeAndFormatInfo(ge::GeShape old_shape, vector<std::pair<int64_t, int64_t>> old_range,
                     vector<std::pair<int64_t, int64_t>> &new_range, ge::Format old_format,
                     ge::Format new_format, ge::DataType current_data_type, int64_t op_impl_type,
                     CalcShapeExtraAttr extra_attr) :
          old_shape(old_shape), old_range(old_range), new_range(new_range), old_format(old_format),
          new_format(new_format), current_data_type(current_data_type), op_impl_type(op_impl_type),
          extra_attr(extra_attr) {}
};

using RangeAndFormat = struct RangeAndFormatInfo;

class RangeTransferAccordingToFormat {
 public:
  RangeTransferAccordingToFormat();

  ~RangeTransferAccordingToFormat();

  RangeTransferAccordingToFormat(const RangeTransferAccordingToFormat &) = delete;

  RangeTransferAccordingToFormat &operator=(const RangeTransferAccordingToFormat &) = delete;

  static Status GetRangeAccordingToFormat(RangeAndFormat &range_and_format_info);

  /*----------Below is the function of getting new range----------------------*/
  static Status GetNCHWRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                        const vector<std::pair<int64_t, int64_t>> &range_value,
                                        const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetNHWCRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                        const vector<std::pair<int64_t, int64_t>> &range_value,
                                        const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetNC1HWC0RangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                           const vector<std::pair<int64_t, int64_t>> &range_value,
                                           const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetFzRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                      const vector<std::pair<int64_t, int64_t>> &range_value,
                                      const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetFzC04RangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                         const vector<std::pair<int64_t, int64_t>> &range_value,
                                         const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetHWCNRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                        const vector<std::pair<int64_t, int64_t>> &range_value,
                                        const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetCHWNRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                        const vector<std::pair<int64_t, int64_t>> &range_value,
                                        const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetC1HWNCoC0RangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                             const vector<std::pair<int64_t, int64_t>> &range_value,
                                             const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetNzRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                      const vector<std::pair<int64_t, int64_t>> &range_value,
                                      const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetNDC1HWC0RangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                            const vector<std::pair<int64_t, int64_t>> &range_value,
                                            const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetFz3DRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                        const vector<std::pair<int64_t, int64_t>> &range_value,
                                        const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetFz3DTransposeRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range,
                                                 const int64_t &impl_type,
                                                 const vector<std::pair<int64_t, int64_t>> &range_value,
                                                 const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetFznRNNRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                          const vector<std::pair<int64_t, int64_t>> &range_value,
                                          const vector<std::pair<int64_t, int64_t>> &nd_range_value);

  static Status GetNDRNNRangeByAxisValue(vector<std::pair<int64_t, int64_t>> &new_range, const int64_t &impl_type,
                                         const vector<std::pair<int64_t, int64_t>> &range_value,
                                         const vector<std::pair<int64_t, int64_t>> &nd_range_value);
 private:
  /* map of GetAxisValueInfoByFormat, get axis value by different original
   * formats. */
  static const std::map<ge::Format, GetNewRangeByAxisValueAndFormatPtr> get_new_range_func_map;
  static void SetRNNRangeAttr(const RangeAndFormat &range_and_format_info,
                              std::vector<std::pair<int64_t, int64_t>> &range_value);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_RANGE_FORMAT_TRANSFER_TRANSFER_RANGE_ACCORDING_TO_FORMAT_H_
