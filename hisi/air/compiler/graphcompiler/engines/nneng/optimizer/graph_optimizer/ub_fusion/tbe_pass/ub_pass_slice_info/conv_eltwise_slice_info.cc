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

#include "conv_eltwise_slice_info.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
namespace fe {

static const int32_t OUTPUT_INDEX_ONE = 1;
static const size_t DUAL_OUTPUT = 2;

Status ConvEltwiseSliceInfo::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                      const vector<ge::NodePtr> &fusion_nodes,
                                                      OpCalcInfo &op_calc_info, size_t &input_size,
                                                      const bool &is_head_fusion) {

  // 2. set elemwise(double input) input split info
  InputSplitInfo elemwise_input_split_info;
  if (!elemwise_input_split_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][MdfSliceInfo] elemwise_input_split_info initialize failed");
    return FAILED;
  }
  elemwise_input_split_info.SetIndex(input_size - 1);
  FE_LOGI("input size is %zu", input_size);
  // elemwise(double input) can only split 0 axis
  std::vector<int64_t> axis = {0};
  elemwise_input_split_info.SetAxis(axis);
  // elemwise(double input)'s overlap must be minus one
  std::vector<int64_t> over_lap = {-1};
  elemwise_input_split_info.SetHeadOverLap(over_lap);
  elemwise_input_split_info.SetTailOverLap(over_lap);

  // 3. add elemwise(double input) input split info for each split map
  std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
  bool has_placeholder_input = false;
  for (auto input_node : fusion_node->GetInDataNodes()) {
    if (input_node == nullptr) {
      continue;
    }
    if (input_node->GetType() == "PlaceHolder") {
      has_placeholder_input = true;
      break;
    }
  }
  bool has_scalar_input = false;
  if (fusion_node->GetInDataNodes().size() == 2) {
    auto other_input_index = is_head_fusion ? 1 : 0;
    auto other_inputdesc = fusion_node->GetOpDesc()->GetInputDesc(other_input_index);
    if (other_inputdesc.GetOriginShape().GetDims().size() == 0) {
      has_scalar_input = true;
    }
  }
  if (fusion_node->GetInDataNodes().size() == 1 || has_placeholder_input || has_scalar_input) {
    FE_LOGD("Node[%s] has single input or placeholder input, not need to modify slice info.",
            fusion_node->GetName().c_str());
  } else {
    for (auto &axis_split_map : axis_split_maps) {
      FE_LOGI("Add input split info of node[%s]", fusion_node->GetName().c_str());
      axis_split_map.AddInputSplitInfo(elemwise_input_split_info);
    }
    op_calc_info.SetAxisSplitMaps(axis_split_maps);
  }
  // need to judge mult output
  if (fusion_node->GetOutDataNodes().size() == DUAL_OUTPUT) {
    bool mult_output = false;
    auto first_node = fusion_node->GetOutDataNodes().at(0);
    auto second_node = fusion_node->GetOutDataNodes().at(1);
    bool find_first_node = std::find(fusion_nodes.begin(), fusion_nodes.end(), first_node) != fusion_nodes.end();
    bool find_second_node = std::find(fusion_nodes.begin(), fusion_nodes.end(), second_node) != fusion_nodes.end();
    mult_output = (find_first_node && !find_second_node) || (!find_first_node && find_second_node);
    if (mult_output) {
      SetOutputSliceInfoForEltwise(fusion_node, op_calc_info);
    }
  }
  return SUCCESS;
}

Status ConvEltwiseSliceInfo::SetOutputSliceInfoForEltwise(const ge::NodePtr &fusion_node,
                                                          OpCalcInfo &op_calc_info) const {
  // 3. add elemwise(double output) output split info for each split map
  std::vector<AxisSplitMap> axis_split_maps = op_calc_info.GetAxisSplitMapVec();
  for (auto &axis_split_map : axis_split_maps) {
    // 2. set elemwise(double output) output split info
    OutputSplitInfo elemwise_output_split_info;
    if (!elemwise_output_split_info.Initialize()) {
      REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][SetOutSliceInfo] elemwise_output_split_info initialize failed");
      return FAILED;
    }
    elemwise_output_split_info.SetIndex(OUTPUT_INDEX_ONE);
    std::vector<OutputSplitInfo> output_split_infos = axis_split_map.GetOutputSplitInfoVec();
    if (!output_split_infos.empty()) {
      std::vector<int64_t> output_axis = output_split_infos.at(0).GetAxis();
      elemwise_output_split_info.SetAxis(output_axis);
      auto tmp = elemwise_output_split_info.GetAxis();
      FE_LOGI("Add output split info of node[%s]", fusion_node->GetName().c_str());
      axis_split_map.AddOutputSplitInfo(elemwise_output_split_info);
    }
  }
  op_calc_info.SetAxisSplitMaps(axis_split_maps);
  return SUCCESS;
}
}  // namespace fe
