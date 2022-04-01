/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef GE_GRAPH_PASSES_POTENTIAL_FOLDING_PASS_H_
#define GE_GRAPH_PASSES_POTENTIAL_FOLDING_PASS_H_

#include <vector>
#include "graph/passes/folding_pass.h"

namespace ge {
class PotentialFoldingPass : public FoldingPass {
 public:
  Status Run(NodePtr &node) override;
  // this func supposed to be const, but low_level interface require non_const
  virtual bool NeedIgnorePass(const NodePtr &node) {
    return false;
  }
  virtual bool NeedFold() const {
    return true;
  }
  // in_param node supposed to be const, but low-level interface require non_const
  virtual Status ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) = 0;
  virtual std::string GetPassName() const = 0;

 protected:
  ///
  /// Check is node all outputs shape is empty tensor.This kind of node is potential empty const.
  /// @param op_desc    Should check not_null before call this func.
  /// @return
  ///
  bool AreAllOutputsEmptyShape(const OpDescPtr &op_desc) const;
  ///
  /// @param shape
  /// @return
  ///
  bool IsKnownEmptyTenor(const ge::GeShape &shape) const;

  ///
  /// check current pass is same with source pass(where potential const comes from)
  /// only source pass can update potential const mark on node.
  /// @param op_desc
  ///
  bool IsCurPassSameWithSource(const OpDescPtr &op_desc) const;

  ///
  /// mark current potential const comes from which pass, pass name from pass inside
  /// @param op_desc
  ///
  bool MarkPotentialConstWithPassName(OpDescPtr &op_desc, const std::vector<int> out_indices,
                                      const std::vector<GeTensorPtr> &outputs) const;
  bool UnMarkPotentialConstWithPassName(OpDescPtr &op_desc);

 private:
  ///
  /// folding const according to compute_ret
  /// @param compute_ret
  /// @param node
  /// @param outputs
  /// @return
  ///
  Status FoldingConstByComputeRet(const Status compute_ret, NodePtr &node, std::vector<GeTensorPtr> &outputs);

  ///
  /// update potential_const mark according to compute_ret
  /// @param node
  /// @param compute_ret   Result of dimension compute, if not_changed, remove potential_const mark.
  /// @param outputs     Weight of potential const. Need set with potential_const mark.
  /// @return
  ///
  Status UpdatePotentialConstMark(const Status compute_ret, const NodePtr &node,
                                  const std::vector<GeTensorPtr> &outputs);

  Status UpdatePeerShapeIfChanged(const NodePtr &node, const std::vector<GeTensorPtr> &outputs) const;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_POTENTIAL_FOLDING_PASS_H_