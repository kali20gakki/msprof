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

#include "ub_pass_slice_info_base.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"

namespace fe {

Status UbPassSliceInfoBase::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node,
                                                     const vector<ge::NodePtr> &fusion_nodes,
                                                     OpCalcInfo &op_calc_info, size_t &input_size,
                                                     const bool &is_head_fusion) {
  return SUCCESS;
}

Status UbPassSliceInfoBase::ModifySliceInfoByPattern(const ge::NodePtr &fusion_node) {
  return SUCCESS;
}

Status UbPassSliceInfoBase::GetOpSliceInfoForSingleOp(const ge::NodePtr &node, OpCalcInfo &op_slice_info) const {
  FE_CHECK_NOTNULL(node);
  if (op_slice_info.IsPtrNull()) {
    return FAILED;
  }
  std::string op_calc_info_str;
  if (!ge::AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_calc_info_str) || op_calc_info_str.empty()) {
    return FAILED;
  }
  GetOpSliceInfoFromJson(op_slice_info, op_calc_info_str);
  return SUCCESS;
}

Status UbPassSliceInfoBase::SetOpSliceInfoForSingleOp(const ge::NodePtr &node, OpCalcInfo &op_slice_info) const {
  FE_CHECK_NOTNULL(node);
  if (op_slice_info.IsPtrNull()) {
    return FAILED;
  }
  if (op_slice_info.GetAxisSplitMaps().size() != 0) {
    string op_calc_info_str;
    SetOpSliceInfoToJson(op_slice_info, op_calc_info_str);
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), OP_SLICE_INFO, op_calc_info_str);
  }
  return SUCCESS;
}
}  // namespace fe
