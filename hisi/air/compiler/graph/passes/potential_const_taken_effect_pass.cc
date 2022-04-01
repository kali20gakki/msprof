/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include <vector>
#include "graph/passes/potential_const_taken_effect_pass.h"

#include "framework/common/debug/log.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/constant_utils.h"

namespace ge {
Status PotentialConstTakenEffectPass::Run(ge::NodePtr &node) {
  (void)node;
  return SUCCESS;
}

Status PotentialConstTakenEffectPass::OnFinishGraph(ComputeGraphPtr &root_graph,
                                                    std::vector<NodePtr> &node_to_be_repass) {
  for (auto &node : root_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if (!ConstantUtils::IsPotentialConst(op_desc)) {
      continue;
    }

    if (node->GetOwnerComputeGraph() == nullptr) {
      // if cur node parent node has been deleted, no need to handle node on this deleted graph.
      GELOGD("Node %s owner graph is null. Perhapse its parent node has been deleted.", node->GetName().c_str());
      continue;
    }

    vector<GeTensorPtr> potential_weights;
    bool has_weight = AttrUtils::MutableListTensor(op_desc, ATTR_NAME_POTENTIAL_WEIGHT, potential_weights);
    if (!has_weight || potential_weights.empty()) {
      GELOGW("Potential const node %s(%s) has no weight, can not fold. May cause fail later.", node->GetName().c_str(),
             node->GetType().c_str());
      if (!ConstantUtils::UnMarkPotentialConst(op_desc)) {
        // not a complete potential const, maybe attrs is removed by mistake, here just rm attrs;
        (void)op_desc->DelAttr(ATTR_NAME_POTENTIAL_CONST);
        (void)op_desc->DelAttr(ATTR_NAME_POTENTIAL_WEIGHT_INDICES);
        (void)op_desc->DelAttr(ATTR_NAME_POTENTIAL_WEIGHT);
      }
      continue;
    }
    GELOGI("Start fold potential const node : %s(%s).", node->GetName().c_str(), node->GetType().c_str());
    auto ret = Folding(node, potential_weights);
    if (ret != SUCCESS) {
      // folding fail we need continue to handle other node on this graph. so do not return
      GELOGW("Failed to fold node %s to const. May cause failure later.", node->GetName().c_str());
      continue;
    }
  }
  std::unordered_set<NodePtr> current_repass_node;
  for (auto &repass_node : GetNodesNeedRePass()) {
    if (current_repass_node.insert(repass_node).second) {
      GELOGI("Node %s will be repass next run.", repass_node->GetName().c_str());
      node_to_be_repass.emplace_back(repass_node);
    }
  }
  return SUCCESS;
}
}  // namespace ge
