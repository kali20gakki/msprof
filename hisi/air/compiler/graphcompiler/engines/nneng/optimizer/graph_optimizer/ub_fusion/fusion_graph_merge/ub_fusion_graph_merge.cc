/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#include "graph_optimizer/ub_fusion/fusion_graph_merge/ub_fusion_graph_merge.h"

namespace fe {
namespace {
static const int kBNTrainingUpdateInputFive = 5;
static const int kBNTrainingUpdateInputSix = 6;
static const int kBNTrainingUpdateOutputZero = 0;
static const int kBNTrainingUpdateOutputOne = 1;
}  // namespace

UBFusionGraphMerge::UBFusionGraphMerge(const std::string &scope_attr, const GraphCommPtr &graph_comm_ptr)
    : FusionGraphMerge(scope_attr, graph_comm_ptr) {}

UBFusionGraphMerge::~UBFusionGraphMerge() {}

Status UBFusionGraphMerge::AfterMergeFusionGraph(ge::ComputeGraph &graph) {
  // iterate all nodes in the graph to find fused BNTrainingUpdate nodes
  for (auto node : graph.GetDirectNode()) {
    if ((node->GetType() != "BNTrainingUpdate" && node->GetType() != AIPP) ||
        !node->GetOpDesc()->HasAttr(GetScopeAttr())) {
      continue;
    }
    int64_t scope_id = -1;
    (void)ge::AttrUtils::GetInt(node->GetOpDesc(), GetScopeAttr(), scope_id);
    // fused BNTrainingUpdate node's scope_id should be positive
    if (scope_id > 0) {
      FE_LOGD("Get fused node:[%s] scope_id:[%ld].", node->GetOpDesc()->GetName().c_str(), scope_id);
      if (node->GetType() == AIPP) {
        node->GetOpDesc()->SetType(CONV2D);
        continue;
      }
      // get BNTrainingUpdate node's all input anchor and output anchor name
      std::map<string, uint32_t> BNUpdateInputName = node->GetOpDesc()->GetAllInputName();
      std::map<string, uint32_t> BNUpdateOutputName = node->GetOpDesc()->GetAllOutputName();
      // set BNTrainingUpdate node's fifth and sixth input anchor name
      for (std::map<string, uint32_t>::iterator it = BNUpdateInputName.begin(); it != BNUpdateInputName.end();) {
        if (it->second == kBNTrainingUpdateInputFive || it->second == kBNTrainingUpdateInputSix) {
          std::map<string, uint32_t>::const_iterator it2 = it;
          ++it;
          BNUpdateInputName.erase(it2);
        } else {
          ++it;
        }
      }
      BNUpdateInputName["mean"] = kBNTrainingUpdateInputFive;
      BNUpdateInputName["variance"] = kBNTrainingUpdateInputSix;
      // set BNTrainingUpdate node's zero and first output anchor name
      for (std::map<string, uint32_t>::iterator it = BNUpdateOutputName.begin(); it != BNUpdateOutputName.end();) {
        if (it->second == kBNTrainingUpdateOutputZero || it->second == kBNTrainingUpdateOutputOne) {
          std::map<string, uint32_t>::const_iterator it2 = it;
          ++it;
          BNUpdateOutputName.erase(it2);
        } else {
          ++it;
        }
      }
      BNUpdateOutputName["mean"] = kBNTrainingUpdateOutputZero;
      BNUpdateOutputName["variance"] = kBNTrainingUpdateOutputOne;
      // update modified anchor name to BNTrainingUpdate node
      node->GetOpDesc()->UpdateInputName(BNUpdateInputName);
      node->GetOpDesc()->UpdateOutputName(BNUpdateOutputName);
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ge::ATTR_NAME_REFERENCE, true);
    }
  }
  return SUCCESS;
}
}
