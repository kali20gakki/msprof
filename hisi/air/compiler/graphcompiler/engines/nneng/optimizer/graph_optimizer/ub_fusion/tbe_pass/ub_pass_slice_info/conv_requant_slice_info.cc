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

#include "conv_requant_slice_info.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
namespace fe {

Status ConvRequantSliceInfo::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                      const vector<ge::NodePtr> &fusion_nodes,
                                                      OpCalcInfo &op_calc_info, size_t &input_size,
                                                      const bool &is_head_fusion) {
  // if requant node's req_scale input is vector, no need to add split info
  ge::GeTensorDesc req_scale_tensor = fusion_node->GetOpDesc()->GetInputDesc(1);
  if (req_scale_tensor.GetOriginShape().GetDims().size() <= 1) {
    FE_LOGI("req scale shape dim is less than 2.");
    return SUCCESS;
  }
  // 2. set requant(double input) input split info
  InputSplitInfo requant_input_split_info;
  if (!requant_input_split_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][MdfSliceInfo] requant_input_split_info initialize failed");
    return FAILED;
  }
  requant_input_split_info.SetIndex(input_size - 1);
  // requant(double input) can only split 0 axis
  std::vector<int64_t> axis = {0};
  requant_input_split_info.SetAxis(axis);
  // requant(double input)'s overlap must be minus one
  std::vector<int64_t> over_lap = {-1};
  requant_input_split_info.SetHeadOverLap(over_lap);
  requant_input_split_info.SetTailOverLap(over_lap);

  // 3. add requant(double input) input split info for each split map
  std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
  for (auto &axis_split_map : axis_split_maps) {
    axis_split_map.AddInputSplitInfo(requant_input_split_info);
  }
  op_calc_info.SetAxisSplitMaps(axis_split_maps);
  return SUCCESS;
}
}  // namespace fe
