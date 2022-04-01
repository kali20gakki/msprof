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

#ifndef GE_GRAPH_PASSES_REPLACE_WITH_EMPTY_CONST_PASS_H_
#define GE_GRAPH_PASSES_REPLACE_WITH_EMPTY_CONST_PASS_H_

#include "graph/passes/potential_folding_pass.h"

namespace ge {
class ReplaceWithEmptyConstPass : public PotentialFoldingPass {
 public:
  explicit ReplaceWithEmptyConstPass(bool need_fold = true) : need_fold_(need_fold) {}
  bool NeedIgnorePass(const NodePtr &node) override;
  bool NeedFold() const override;
  Status ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) override;
  std::string GetPassName() const override;
 private:
  Status GetOutputsOfCurrNode(const NodePtr &node_to_replace, std::vector<GeTensorPtr> &outputs) const;
  // indicate pass provide folding func or not, default : need_fold
  bool need_fold_ = true;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_REPLACE_WITH_EMPTY_CONST_PASS_H_
