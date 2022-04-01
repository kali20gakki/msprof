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


#include "graph/passes/dimension_compute_pass.h"

#include <memory>
#include <vector>

#include "framework/common/debug/log.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/utils/attr_utils.h"
#include "inc/kernel.h"

namespace ge {
namespace {
const std::string kPassName = "DimensionComputePass";
}
bool DimensionComputePass::NeedIgnorePass(const NodePtr &node) {
  if (AreAllOutputsEmptyShape(node->GetOpDesc())) {
    GELOGI("Cur node %s is potential empty const, ignore pass.", node->GetName().c_str());
    return true;
  }
  return false;
}

bool DimensionComputePass::NeedFold() const {
  return need_fold_;
}

 /// Compute node dimension by host kernel
 /// @param node
 /// @param outputs  SUCCESS: compute success.
 ///                 UNSUPPORTED: not support compute, do nothing
 ///                 NOT_CHANGED: dynamic_shape or param invalid
 ///                 INTERNAL_ERROR: after compute, output weight is empty
 /// @return
Status DimensionComputePass::ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) {
  auto op_kernel = folding_pass::GetKernelByType(node);
  if (op_kernel == nullptr || folding_pass::IsNoNeedConstantFolding(node)) {
    return UNSUPPORTED;
  }

  return op_kernel->Compute(node, outputs);
}

string DimensionComputePass::GetPassName() const {
  return kPassName;
}
}  // namespace ge
