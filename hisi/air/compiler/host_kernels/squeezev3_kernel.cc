/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "host_kernels/squeezev3_kernel.h"

#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "host_kernels/kernel_utils.h"
#include "inc/kernel_factory.h"

namespace {
constexpr size_t kIndex = 0UL;
constexpr size_t kSqueezeInputSize = 1UL;
constexpr size_t kSqueezeOutputSize = 1UL;
constexpr size_t kSqueezeInputSizeWithAxes = 2UL;
}  // namespace

namespace ge {
Status SqueezeV3Kernel::Compute(const NodePtr &node_ptr) const {
  if (node_ptr == nullptr) {
    GELOGE(PARAM_INVALID, "parameter is nullptr");
    return PARAM_INVALID;
  }
  if (!KernelUtils::CheckFormatSupported(node_ptr)) {
    GELOGW("CheckFormatSupported failed");
    return NOT_CHANGED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  if (op_desc->GetInputsSize() == kSqueezeInputSizeWithAxes) {
    Status ret = KernelUtils::CheckDimensionNodeInfo(node_ptr);
    if (ret != SUCCESS) {
      GELOGW("GetDimensionNodeInfo failed");
      return ret;
    }
  }
  return SUCCESS;
}

Status SqueezeV3Kernel::Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                                std::vector<ge::GeTensorPtr> &v_output) {
  GELOGD("SqueezeV3 folding kernel in.");
  GE_CHECK_NOTNULL(op_desc_ptr);
  const std::string op_name = op_desc_ptr->GetName();

  if (((input.size() != kSqueezeInputSize) && (input.size() != kSqueezeInputSizeWithAxes)) ||
      (op_desc_ptr->GetOutputsSize() != kSqueezeOutputSize)) {
    GELOGW("Unexpected SqueezeV3 node, node input size: %zu, node output size: %zu, node name: %s", input.size(),
           op_desc_ptr->GetOutputsSize(), op_name.c_str());
    return NOT_CHANGED;
  }

  const auto &output_tensor_desc = op_desc_ptr->GetOutputDesc(kIndex);
  GeTensorPtr output_ptr = MakeShared<GeTensor>(output_tensor_desc);
  if (output_ptr == nullptr) {
    GELOGE(PARAM_INVALID, "ndoe [%s] make shared failed", op_name.c_str());
    return PARAM_INVALID;
  }

  const auto ge_tensor = input[kIndex];
  if (ge_tensor == nullptr) {
    GELOGE(PARAM_INVALID, "node [%s] get input failed.", op_name.c_str());
    return PARAM_INVALID;
  }

  if (output_ptr->SetData(ge_tensor->GetData()) != GRAPH_SUCCESS) {
    GELOGW("Compute: SetData failed");
    return NOT_CHANGED;
  }
  v_output.emplace_back(output_ptr);
  GELOGD("SqueezeV3 folding kernel success.");
  return SUCCESS;
}
REGISTER_KERNEL(SQUEEZEV3, SqueezeV3Kernel);
} // namespace ge