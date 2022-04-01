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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/delete_no_const_folding_fusion_pass.h"
#include <string>
#include "common/fe_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace fe {
static const string OP_WEIGHTQUANT = "AscendWeightQuant";
static const string PATN_WEIGHT_QUANT = "weight_quant";

vector<FusionPattern *> DeleteNoConstFolding::DefinePatterns() {
  vector<FusionPattern *> patterns;
  /*
   * ================================pattern0===================================
   *
   *  --> AscendWeightQuant-->
   *
   * ===========================================================================
   */
  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("deleteNoConstFoldingFusion0");
  FE_CHECK(pattern0 == nullptr,
      REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* above defines ops that we need */
  pattern0->AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT}).SetOutput(PATN_WEIGHT_QUANT);
  patterns.push_back(pattern0);

  return patterns;
}

Status DeleteNoConstFolding::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  for (Mapping::iterator itr = mapping.begin(); itr != mapping.end(); ++itr) {
    FE_CHECK(itr->second.size() == 0, FE_LOGE("no node in mapping!"), return FAILED);
    auto node = itr->second[0];
    FE_CHECK_NOTNULL(node);
    if (node->GetOpDesc()->HasAttr(ge::ATTR_NO_NEED_CONSTANT_FOLDING)) {
      if (node->GetOpDesc()->DelAttr(ge::ATTR_NO_NEED_CONSTANT_FOLDING) != ge::GRAPH_SUCCESS) {
        return FAILED;
      }
      FE_LOGD("delete attr[%s] of node[%s] success.", ge::ATTR_NO_NEED_CONSTANT_FOLDING.c_str(),
              node->GetName().c_str());
    }
  }

  return SUCCESS;
}

}  // namespace fe
