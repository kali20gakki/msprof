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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_CONV_DEQUANTS16_SLICE_INFO_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_CONV_DEQUANTS16_SLICE_INFO_H

#include <vector>
#include "ub_pass_slice_info_base.h"
#include "common/fe_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"

namespace fe {
class ConvDequantS16SliceInfo : public UbPassSliceInfoBase {
 protected:
  Status ModifySliceInfoByPattern(const ge::NodePtr &fusion_node, const vector<ge::NodePtr> &fusion_nodes,
                                  OpCalcInfo &op_calc_info, size_t &input_size, const bool &is_head_fusion) override;
};
}
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_CONV_DEQUANTS16_SLICE_INFO_H
