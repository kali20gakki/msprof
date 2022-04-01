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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_MANAGER_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_MANAGER_H

#include <vector>
#include "common/fe_log.h"
#include "ub_pass_slice_info_base.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"


namespace fe {

class UbPassSliceInfoManager {
 public:
  static Status SetSliceInfoForFusionNodes(vector<ge::NodePtr> &fusion_nodes);

  static UbPassSliceInfoBasePtr SwitchSliceInfoPtrByPattern(UbMatchedType &ub_matched_pattern, ge::NodePtr &fusion_node,
                                                            size_t &input_size);
  static bool IsHeadFusion(const ge::NodePtr &fusion_node, const vector<ge::NodePtr> &fusion_nodes);

  static bool CheckOpPatternSupport(const string &op_pattern);

  static bool CheckOpPatternSliceInfoUpdate(const string &op_pattern);

  static Status CalcSliceInfoForFusionOp(vector<ge::NodePtr> &fusion_nodes, OpCalcInfo &op_slice_info);

};

}
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_UB_PASS_SLICE_INFO_MANAGER_H
