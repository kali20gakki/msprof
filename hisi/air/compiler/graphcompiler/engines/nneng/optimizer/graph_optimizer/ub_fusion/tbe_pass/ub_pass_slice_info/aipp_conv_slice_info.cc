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

#include "aipp_conv_slice_info.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
#include "common/op_info_common.h"
#include "external/graph/types.h"
namespace fe {
Status AippConvSliceInfo::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                   const vector<ge::NodePtr> &fusion_nodes,
                                                   OpCalcInfo &op_calc_info, size_t &input_size,
                                                   const bool &is_head_fusion) {
  ge::GeTensorDesc input_desc = fusion_node->GetOpDesc()->GetInputDesc(0);
  ge::Format input_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_desc.GetFormat()));
  if (input_format != ge::FORMAT_NCHW && input_format != ge::FORMAT_NHWC) {
    FE_LOGD("Node[%s] format is not NCHW or NHWC, not modify slice info.", fusion_node->GetName().c_str());
    return SUCCESS;
  }

  // delete H axis slice info
  int32_t axis_h = GetAxisIndexByFormat(input_format, "H");
  if (axis_h >= 0) {
    std::vector<int64_t> axis_cut = {axis_h};
    std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
    op_calc_info.DelAxisSplitMapBaseAxis(axis_cut);
  }
  // delete W axis slice info
  int32_t axis_w = GetAxisIndexByFormat(input_format, "W");
  if (axis_w >= 0) {
    std::vector<int64_t> axis_cut = {axis_w};
    std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
    op_calc_info.DelAxisSplitMapBaseAxis(axis_cut);
  }
  return SUCCESS;
}
}  // namespace fe
