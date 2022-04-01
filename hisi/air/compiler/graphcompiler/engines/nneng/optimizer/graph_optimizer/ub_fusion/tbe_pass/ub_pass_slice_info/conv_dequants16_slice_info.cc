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

#include "conv_dequants16_slice_info.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
namespace fe {

static const size_t MIN_INPUT_NUM = 1;
static const size_t FIRST_INDEX = 1;
static const size_t SECOND_INDEX = 2;
Status ConvDequantS16SliceInfo::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                         const vector<ge::NodePtr> &fusion_nodes,
                                                         OpCalcInfo &op_calc_info, size_t &input_size,
                                                         const bool &is_head_fusion) {
  // if dequant node's deq_scale input is vector, no need to add split info
  ge::GeTensorDesc deq_scale_tensor = fusion_node->GetOpDesc()->GetInputDesc(FIRST_INDEX);
  if (deq_scale_tensor.GetOriginShape().GetDims().size() <= MIN_INPUT_NUM) {
    return SUCCESS;
  }
  // 2. set dequant(trible input) input split info
  InputSplitInfo dequant_input_split_info;
  if (!dequant_input_split_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][MdfSliceInfo] dequant_input_split_info initialize failed");
    return FAILED;
  }
  dequant_input_split_info.SetIndex(input_size - SECOND_INDEX);
  // dequant(double input) can only split 0 axis
  std::vector<int64_t> axis = {1};
  dequant_input_split_info.SetAxis(axis);
  // dequant(double input)'s overlap must be minus one
  std::vector<int64_t> over_lap = {-1};
  dequant_input_split_info.SetHeadOverLap(over_lap);
  dequant_input_split_info.SetTailOverLap(over_lap);

  // 2. set dequant(trible input) input split info
  InputSplitInfo dequant_x1_input_split_info;
  if (!dequant_x1_input_split_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][MdfSliceInfo] dequant_x1_input_split_info initialize failed");
    return FAILED;
  }
  dequant_x1_input_split_info.SetIndex(input_size - FIRST_INDEX);
  // dequant(double input) can only split 0 axis
  dequant_x1_input_split_info.SetAxis(axis);
  // dequant(double input)'s overlap must be minus one
  dequant_x1_input_split_info.SetHeadOverLap(over_lap);
  dequant_x1_input_split_info.SetTailOverLap(over_lap);

  // 3. add dequant(double input) input split info for each split map
  std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
  for (auto &axis_split_map : axis_split_maps) {
    std::vector<InputSplitInfo> input_split_infos = axis_split_map.GetInputSplitInfoVec();
    for (auto input_split_info : input_split_infos) {
      std::vector<int64_t> axes = input_split_info.GetAxis();
      if (axes == axis) {
        axis_split_map.AddInputSplitInfo(dequant_input_split_info);
        axis_split_map.AddInputSplitInfo(dequant_x1_input_split_info);
      }
    }
  }
  op_calc_info.SetAxisSplitMaps(axis_split_maps);
  return SUCCESS;
}
}  // namespace fe
