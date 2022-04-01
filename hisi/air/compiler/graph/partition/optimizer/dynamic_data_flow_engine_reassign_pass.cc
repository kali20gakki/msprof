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

#include "graph/partition/optimizer/dynamic_data_flow_engine_reassign_pass.h"
#include "framework/common/util.h"
#include "framework/common/types.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
Status DynamicDataFlowReAssign::Run(const ComputeGraphPtr &graph,
                                    NodeEngineMap &node_atomic_engine_map,
                                    NodeEngineMap &node_composite_engine_map) {
  GE_CHECK_NOTNULL(graph);
  (void)node_composite_engine_map;
  static const char_t *const kGeLocalEngineName = "DNN_VM_GE_LOCAL";
  static const char_t *const kGeLocalOpKernelLibName = "DNN_VM_GE_LOCAL_OP_STORE";
  static const std::unordered_set<std::string> kDataFlowOps = {STACK, STACKPUSH, STACKPOP, STACKCLOSE};
  for (const NodePtr &node : graph->GetAllNodes()) {
    const auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if (kDataFlowOps.count(node->GetType()) == 0UL) {
      continue;
    }
    bool is_unknown = false;
    const bool ret = AttrUtils::GetBool(op_desc, ATTR_NAME_IS_UNKNOWN_SHAPE, is_unknown);
    if (ret && is_unknown) {
      op_desc->SetOpEngineName(kGeLocalEngineName);
      op_desc->SetOpKernelLibName(kGeLocalOpKernelLibName);
      node_atomic_engine_map[node] = kGeLocalEngineName;
      GELOGD("ReAssigning DNNEngine %s to node %s, op type %s", kGeLocalEngineName, node->GetName().c_str(),
             node->GetType().c_str());
    }
  }
  return SUCCESS;
}
}  // namespace ge
