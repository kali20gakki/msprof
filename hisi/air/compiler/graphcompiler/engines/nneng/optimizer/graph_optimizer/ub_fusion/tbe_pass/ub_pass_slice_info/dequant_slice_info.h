/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_DEQUANT_SLICE_INFO_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_DEQUANT_SLICE_INFO_H

#include "ub_pass_slice_info_base.h"
#include "common/fe_log.h"
#include "graph/utils/graph_utils.h"

namespace fe {
class DequantSliceInfo : public UbPassSliceInfoBase {
 public:
  Status ModifySliceInfoByPattern(const ge::NodePtr &fusion_node) override;
};
}
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_UB_PASS_SLICE_INFO_DEQUANT_SLICE_INFO_H
