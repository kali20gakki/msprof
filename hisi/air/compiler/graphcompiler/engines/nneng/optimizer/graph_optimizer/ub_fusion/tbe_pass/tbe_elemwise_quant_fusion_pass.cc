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

#include "tbe_elemwise_quant_fusion_pass.h"
#include <string>
#include <vector>
#include "common/fe_log.h"

namespace fe {
const string kPatternElemwise = "eltwise";
const string kPatternQuant = "quant";
const size_t kInputSizeLimit = 2;
const string kOpTypeElt = "Eltwise";

vector<BufferFusionPattern *> TbeElemwiseQuantFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  string pattern_name1 = "TbeElemwiseQuantFusionPattern";
  BufferFusionPattern *pattern1 = new (std::nothrow) BufferFusionPattern(pattern_name1);
  FE_CHECK(pattern1 == nullptr, FE_LOGE("Fail to create new pattern[%s].", pattern_name1.c_str()), return patterns);
  FE_LOGD("Start to define %s buffer fusion pass pattern.", pattern_name1.c_str());
  // define pattern Elemwise -> AscendQuant
  pattern1
      ->AddOpDesc(kPatternElemwise, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_STATIC)
      .AddOpDesc(kPatternQuant, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_STATIC)
      .SetHead({kPatternElemwise})
      .SetOutputs(kPatternElemwise, {kPatternQuant});
  patterns.push_back(pattern1);
  FE_LOGD("End to define %s buffer fusion pass pattern.", pattern_name1.c_str());
  return patterns;
}

/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
Status TbeElemwiseQuantFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                  vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to verify fused nodes for TbeTbeElemwiseQuantFusionPass ub fusion pass.");
  vector<ge::NodePtr> elemwise_nodes = GetMatchedNodesByDescName(kPatternElemwise, mapping);
  if (elemwise_nodes.size() != 1) {
    FE_LOGE("There should be only one elemwise node, but actually is [%zu.]", elemwise_nodes.size());
    return FAILED;
  }
  ge::NodePtr elemwise_node = elemwise_nodes.at(0);
  if (elemwise_node->GetType() == kOpTypeElt) {
    FE_LOGD("The type of elemwise node[%s, %s] can not be Eltwise.", elemwise_node->GetName().c_str(),
            elemwise_node->GetType().c_str());
    return SUCCESS;
  }

  if (elemwise_node->GetOpDesc()->GetInputsSize() != kInputSizeLimit) {
    FE_LOGD("The elemwise node[%s] should have two input tensors, but actually is [%zu]",
            elemwise_node->GetName().c_str(), elemwise_node->GetOpDesc()->GetInputsSize());
    return SUCCESS;
  }
  fusion_nodes = GetMatchedNodes(mapping);
  FE_LOGD("End to verify fused nodes for TbeTbeElemwiseQuantFusionPass ub fusion pass.");
  return SUCCESS;
}
}  // namespace fe