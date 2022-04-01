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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_BASE_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_BASE_H

#include <vector>
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "graph_optimizer/fusion_common/op_slice_info.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"

namespace fe {

class UbPassSliceInfoBase;
using UbPassSliceInfoBasePtr = std::shared_ptr<UbPassSliceInfoBase>;

const std::vector<int64_t> kNc1hwc0NCutAxis = {0};
const std::vector<int64_t> kNc1hwc0HCutAxis = {2};
const std::vector<int64_t> kNc1hwc0WCutAxis = {3};
const std::vector<int64_t> kNc1hwc0CoutCutAxis = {1};

class UbPassSliceInfoBase {
 public:
  virtual Status ModifySliceInfoByPattern(const ge::NodePtr &fusion_node, const vector<ge::NodePtr> &fusion_nodes,
                                          OpCalcInfo &op_calc_info, size_t &input_size, const bool &is_head_fusion);
  virtual Status ModifySliceInfoByPattern(const ge::NodePtr &fusion_node);
  Status SetOpSliceInfoForSingleOp(const ge::NodePtr& node, OpCalcInfo& op_slice_info) const;
  Status GetOpSliceInfoForSingleOp(const ge::NodePtr& node, OpCalcInfo& op_slice_info) const;
};
}
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_BASE_H
