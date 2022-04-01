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

#include "conv2dtd_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
/*
 *  fusion pattern
 *           input
 *               \
 *             AscendQuant
 *                \
 *                 v
 *     weights--->Conv2DTransposeD--->AscendDequant--->output
 *                ^                    ^
 *               /                    /
 *              /                    /
 *           bias                deq_scale
 */
vector<FusionPattern *> Conv2DTDQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define Conv2DTransposeD quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("Conv2DTransposeDQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {CONV2DTD})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

void Conv2DTDQuantProcessFusionPass::SetCinCoutReverse(ge::NodePtr &nodePtr) {
  (void)ge::AttrUtils::SetBool(nodePtr->GetOpDesc(), ATTR_CIN_COUT_REVERSE, true);
}
}  // namespace fe
