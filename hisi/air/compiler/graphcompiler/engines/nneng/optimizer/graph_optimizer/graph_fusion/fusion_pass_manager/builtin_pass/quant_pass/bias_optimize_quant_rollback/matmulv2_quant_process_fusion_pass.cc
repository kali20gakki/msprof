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

#include "matmulv2_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
static const string MATMULV2 = "MatMulV2";

/*
 *  fusion pattern
 *           input
 *               \
 *             AscendQuant
 *                \
 *                 v
 *     weights--->MatmulV2--->AscendDequant--->output
 *                ^          ^
 *               /          /
 *              /          /
 *           bias      deq_scale
 */
vector<FusionPattern *> MatmulV2QuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define matmulv2 quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("MatmulV2QuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {MATMULV2})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status MatmulV2QuantProcessFusionPass::GetCoValue(ge::NodePtr &cube_node, int64_t &co) {
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(1)->MutableShape();
  auto filter_dims = filter_shape.GetDims();
  if (filter_dims.size() == 2) {
    co = filter_dims[1];
  }

  return SUCCESS;
}

Status MatmulV2QuantProcessFusionPass::GetQuantProcessMode(ge::NodePtr &quant_node, ge::NodePtr &cube_node,
                                                           QuantProcessMode &quant_process_mode) {
  ge::OpDescPtr op_desc_ptr = quant_node->GetOpDesc();
  auto input_shape = op_desc_ptr->MutableInputDesc(0)->MutableShape();

  // JudgeUnknownShape
  if (IsUnknownShapeValue(input_shape.GetDim(0)) || IsUnknownShapeValue(input_shape.GetDim(1))) {
    FE_LOGW("quantRollbackBiasOptimizeFusion cannot be applied for unknown shape.");
    quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
    return SUCCESS;
  }

  quant_process_mode = QuantProcessMode::BIAS_OPTIMIZE;
  return SUCCESS;
}
}  // namespace fe