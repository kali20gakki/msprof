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

#include "conv_strided_slice_info.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
namespace fe {

Status ConvStridedSliceInfo::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                      const vector<ge::NodePtr> &fusion_nodes,
                                                      OpCalcInfo &op_calc_info, size_t &input_size,
                                                      const bool &is_head_fusion) {
  // 1. get axis from strided node
  int32_t axis = -1;
  (void)ge::AttrUtils::GetInt(fusion_node->GetOpDesc(), "axis", axis);

  // 2. Del axis cut info from StridedRead split info
  if (axis >= 0) {
    std::vector<int64_t> axis_cut = {axis};
    std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
    op_calc_info.DelAxisSplitMapBaseAxis(axis_cut);
  }
  return SUCCESS;
}
}  // namespace fe
