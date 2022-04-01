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

#include "graph/passes/replace_with_empty_const_pass.h"
#include <string>
#include "common/plugin/ge_util.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"

namespace {
const std::string kPassName = "ReplaceWithEmptyConstPass";
const std::unordered_set<std::string> kControlFlowOps = {
    ge::SWITCH,
    ge::REFSWITCH,
    ge::MERGE,
    ge::REFMERGE,
    ge::ENTER,
    ge::REFENTER,
    ge::NEXTITERATION,
    ge::REFNEXTITERATION,
    ge::EXIT,
    ge::REFEXIT,
    ge::LOOPCOND
};
// tmp solution, when IR support is_stateful interface, here change to inferface
// resource op always show up on pair, once only one resouce op has empty output tensor,
// we should consider remove them together.
// Currently, we ignore resource op, let them run with empty tensor.
const std::unordered_set<std::string> kResourceOps = {
    ge::STACK,
    ge::STACKPOP,
    ge::STACKPUSH
};
}
namespace ge {
bool ReplaceWithEmptyConstPass::NeedIgnorePass(const NodePtr &node) {
  const std::set<std::string> constant_like_task_ops = {CONSTANT, CONSTANTOP, DATA, FILECONSTANT};
  auto node_type = NodeUtils::GetNodeType(node);
  if (constant_like_task_ops.count(node_type) > 0) {
    GELOGI("Node %s is const. Ignore current pass.", node->GetName().c_str());
    return true;
  }
  if ((kControlFlowOps.count(node_type) != 0) ||
      (kResourceOps.count(node_type) != 0)) {
    GELOGI("Node %s type %s is control flow op or resouce op. Ignore current pass.",
           node->GetName().c_str(), node_type.c_str());
    return true;
  }
  // Node like no op, it has no output
  if (node->GetOpDesc()->GetAllOutputsDescPtr().empty()) {
    GELOGI("Node %s has no output desc. Ignore current pass.", node->GetName().c_str());
    return true;
  }
  // if node is inserted by ge, like empty identity inserted by folding pass, need ignore current pass
  // to avoid optimize again and again
  bool is_inserted_by_ge = false;
  (void)AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_IS_INSERTED_BY_GE, is_inserted_by_ge);
  if (is_inserted_by_ge) {
    return true;
  }
  return false;
}

bool ReplaceWithEmptyConstPass::NeedFold() const {
  return need_fold_;
}

Status ReplaceWithEmptyConstPass::ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) {
  auto op_desc = node->GetOpDesc();
  // If any of outputs of current node is not empty, ignore pass
  if (!AreAllOutputsEmptyShape(op_desc)) {
    GELOGI("Node %s outputs are not all empty tensor, not change.", node->GetName().c_str());
    return NOT_CHANGED;
  }

  GELOGI("Node %s has empty tensor output. It will be replaced by empty const.", node->GetName().c_str());
  return GetOutputsOfCurrNode(node, outputs);
}

Status ReplaceWithEmptyConstPass::GetOutputsOfCurrNode(const NodePtr &node_to_replace,
                                                       std::vector<GeTensorPtr> &outputs) const {
  for (const auto &out_anchor : node_to_replace->GetAllOutDataAnchors()) {
    GE_CHECK_NOTNULL(node_to_replace->GetOpDesc());
    auto out_desc = node_to_replace->GetOpDesc()->GetOutputDesc(out_anchor->GetIdx());
    GeTensorPtr empty_tensor = MakeShared<ge::GeTensor>(out_desc);
    GE_CHECK_NOTNULL(empty_tensor);
    outputs.emplace_back(empty_tensor);
  }
  return SUCCESS;
}

string ReplaceWithEmptyConstPass::GetPassName() const {
  return kPassName;
}
}  // namespace ge
