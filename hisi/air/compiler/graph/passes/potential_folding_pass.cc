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

#include "graph/passes/potential_folding_pass.h"
#include <vector>
#include <numeric>
#include "graph/utils/constant_utils.h"

namespace ge {
namespace {
const std::string kAttrNameSourcePass = "_source_pass_of_potential_const";
}
Status PotentialFoldingPass::Run(ge::NodePtr &node) {
  GE_CHECK_NOTNULL(node);
  GE_CHECK_NOTNULL(node->GetOpDesc());
  if (NeedIgnorePass(node)) {
    GELOGI("Node %s need ignore folding pass.", node->GetName().c_str());
    return SUCCESS;
  }
  std::vector<GeTensorPtr> outputs;
  auto compute_ret = ComputePotentialWeight(node, outputs);
  if (!IsResultSuccess(compute_ret)) {
    GELOGE(compute_ret, "Compute ret is wrong. Exit potential folding.");
    return compute_ret;
  }
  // after compute, if out_shape is changed, need update peer node input_shape ,so that peer node can infershape on
  // new input_shape. Eg. where kernel compute can change output shape from unknown to known
  if (compute_ret == SUCCESS) {
    auto ret = UpdatePeerShapeIfChanged(node, outputs);
    if (ret != SUCCESS) {
      return ret;
    }
  }

  if (NeedFold()) {
    return FoldingConstByComputeRet(compute_ret, node, outputs);
  }
  // if no need_fold , just mark potential const
  return UpdatePotentialConstMark(compute_ret, node, outputs);
}

Status PotentialFoldingPass::FoldingConstByComputeRet(const Status compute_ret, NodePtr &node,
                                                      std::vector<GeTensorPtr> &outputs) {
  // if need_fold, only fold node when compute_ret is success
  if (compute_ret == NOT_CHANGED || compute_ret == UNSUPPORTED) {
    GELOGD("Node %s type %s, compute terminates and exits the constant folding.", node->GetName().c_str(),
           node->GetType().c_str());
    return SUCCESS;
  }
  if (outputs.empty()) {
    REPORT_INNER_ERROR("E19999", "After calculate for node %s(%s), output weight is empty, check invalid",
                       node->GetName().c_str(), node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] After calculate for node %s(%s), output weight is empty",
           node->GetName().c_str(), node->GetType().c_str());
    return INTERNAL_ERROR;
  }
  GELOGI("Node %s can be fold to const, now start folding.", node->GetName().c_str());
  return Folding(node, outputs);
}

Status PotentialFoldingPass::UpdatePotentialConstMark(const Status compute_ret, const NodePtr &node,
                                                      const std::vector<GeTensorPtr> &outputs) {
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  switch (compute_ret) {
    case UNSUPPORTED: {
      GELOGD("No kernel support for node %s, ignore pass.", node->GetName().c_str());
      return SUCCESS;
    }
    case NOT_CHANGED: {
      if (ConstantUtils::IsPotentialConst(op_desc) && IsCurPassSameWithSource(op_desc)) {
        // During several turns pass of infershape and constant folding, maybe this node is potential_const
        // on last turn. but on this turn, this node is not potential const anymore.
        // so remove mark of potential const.
        GELOGI("After %s compute, node %s is not potential const, unmark.", GetPassName().c_str(),
               node->GetName().c_str());
        (void) UnMarkPotentialConstWithPassName(op_desc);
        AddRePassNodesWithInOut(node);
      }
      return SUCCESS;
    }
    case SUCCESS: {
      if (outputs.empty()) {
        REPORT_INNER_ERROR("E19999", "After calculate for node %s(%s), output weight is empty, check invalid",
                           node->GetName().c_str(), node->GetType().c_str());
        GELOGE(INTERNAL_ERROR, "[Check][Param] After calculate for node %s(%s), output weight is empty",
               node->GetName().c_str(), node->GetType().c_str());
        return INTERNAL_ERROR;
      }

      // currently only all output is const, will mark potential const, so here we
      // generate output_index by sequence, cause output in vector is in order
      std::vector<int> out_indices(outputs.size());
      iota(out_indices.begin(), out_indices.end(), 0);
      if (MarkPotentialConstWithPassName(op_desc, out_indices, outputs)) {
        GELOGI("After %s compute, node %s is potential const, it will be fold later.",
               GetPassName().c_str(), node->GetName().c_str());
      }
      return SUCCESS;
    }
    default:
      return FAILED;
  }
}

bool PotentialFoldingPass::AreAllOutputsEmptyShape(const OpDescPtr &op_desc) const {
  for (const auto &output_desc_ptr : op_desc->GetAllOutputsDescPtr()) {
    if (output_desc_ptr == nullptr) {
      GELOGW("Node %s got empty output_desc_ptr.", op_desc->GetName().c_str());
      return false;
    }
    if (!IsKnownEmptyTenor(output_desc_ptr->GetShape())) {
      GELOGI("Node %s has no-empty output, can not be potential empty const.", op_desc->GetName().c_str());
      return false;
    }
  }
  return true;
}

bool PotentialFoldingPass::IsKnownEmptyTenor(const ge::GeShape &shape) const {
  bool is_known_empty_tensor = false;
  for (auto dim : shape.GetDims()) {
    if (dim < 0) {
      // current dim is unknown dim
      return false;
    } else if (dim == 0) {
      is_known_empty_tensor = true;
    }
  }
  return is_known_empty_tensor;
}

Status PotentialFoldingPass::UpdatePeerShapeIfChanged(const NodePtr &node,
    const std::vector<GeTensorPtr> &outputs) const {
  if (node->GetOpDesc()->GetOutputsSize() != outputs.size()) {
    GELOGE(INTERNAL_ERROR, "Out anchor size %zu, outputs size %zu, not match.",
           node->GetOpDesc()->GetOutputsSize(), outputs.size());
    return INTERNAL_ERROR;
  }

  for (size_t i = 0; i < outputs.size(); ++i) {
    if (outputs[i] == nullptr) {
      REPORT_INNER_ERROR("E19999", "Index:%lu in param v_weight is nullptr check invalid", i);
      GELOGE(INTERNAL_ERROR,
             "[Check][Param] Failed to constant fold on node %s type %s, the %lust node calculated is null",
             node->GetName().c_str(), node->GetType().c_str(), i);
      return INTERNAL_ERROR;
    }
    auto origin_out_shape = node->GetOpDesc()->GetOutputDesc(i).GetShape();
    auto compute_out_shape = outputs[i]->GetTensorDesc().GetShape();
    if (origin_out_shape.GetDims() == compute_out_shape.GetDims()) {
      continue;
    }
    auto peer_in_anchors = node->GetOutDataAnchor(i)->GetPeerInDataAnchors();
    for (const auto &in_anchor : peer_in_anchors) {
      auto peer_node = in_anchor->GetOwnerNode();
      GE_CHECK_NOTNULL(peer_node);
      GE_CHECK_NOTNULL(peer_node->GetOpDesc());
      peer_node->GetOpDesc()->MutableInputDesc(in_anchor->GetIdx())->SetShape(compute_out_shape);
      peer_node->GetOpDesc()->MutableInputDesc(in_anchor->GetIdx())->SetOriginShape(compute_out_shape);
      GELOGI("Compute out_shape is %s, origin out_shape is %s, update peer node %s, %d input_shape.",
             compute_out_shape.ToString().c_str(), origin_out_shape.ToString().c_str(), peer_node->GetName().c_str(),
             in_anchor->GetIdx());
    }
  }
  return SUCCESS;
}

bool PotentialFoldingPass::IsCurPassSameWithSource(const OpDescPtr &op_desc) const {
  string source_pass_name;
  (void) AttrUtils::GetStr(op_desc, kAttrNameSourcePass, source_pass_name);
  return (source_pass_name == GetPassName());
}

bool PotentialFoldingPass::MarkPotentialConstWithPassName(OpDescPtr &op_desc, const std::vector<int> out_indices,
                                                          const std::vector<GeTensorPtr> &outputs) const {
  return ConstantUtils::MarkPotentialConst(op_desc, out_indices, outputs)
          && AttrUtils::SetStr(op_desc, kAttrNameSourcePass, GetPassName());
}

bool PotentialFoldingPass::UnMarkPotentialConstWithPassName(OpDescPtr &op_desc) {
  (void) ConstantUtils::UnMarkPotentialConst(op_desc);
  (void) op_desc->DelAttr(kAttrNameSourcePass);
  return true;
}
}  // namespace ge

