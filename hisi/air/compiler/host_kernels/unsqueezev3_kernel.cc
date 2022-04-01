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

#include "host_kernels/unsqueezev3_kernel.h"

#include <memory>
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "host_kernels/kernel_utils.h"
#include "inc/kernel_factory.h"

namespace {
constexpr uint32_t kIndex = 0;
constexpr size_t kUnsqueezeInputSize = 2UL;
constexpr size_t kUnsqueezeOutputSize = 1UL;
}  // namespace

namespace ge {
Status UnsqueezeV3Kernel::Compute(const NodePtr &node_ptr) const {
  GE_CHECK_NOTNULL(node_ptr);
  if (!KernelUtils::CheckFormatSupported(node_ptr)) {
    GELOGW("CheckFormatSupported failed");
    return NOT_CHANGED;
  }

  const OpDescPtr op_desc = node_ptr->GetOpDesc();
  Status ret = KernelUtils::CheckDimensionNodeInfo(node_ptr);
  if (ret != SUCCESS) {
    GELOGW("GetDimensionNodeInfo failed");
    return ret;
  }

  return SUCCESS;
}

Status UnsqueezeV3Kernel::Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                                  std::vector<ge::GeTensorPtr> &v_output) {
  GE_CHECK_NOTNULL(op_desc_ptr);
  GELOGD("UnsqueezeV3 folding kernel in");
  const std::string op_name = op_desc_ptr->GetName();

  const bool is_size_invalid =
      ((op_desc_ptr->GetInputsSize() != kUnsqueezeInputSize) ||
       (op_desc_ptr->GetOutputsSize() != kUnsqueezeOutputSize) || (input.size() != kUnsqueezeInputSize));
  if (is_size_invalid) {
    GELOGW("Size check fail, node[%s] inputs size:%zu, outputs size:%zu, input size:%zu", op_name.c_str(),
           op_desc_ptr->GetInputsSize(), op_desc_ptr->GetOutputsSize(), input.size());
    return NOT_CHANGED;
  }

  const auto &tensor_desc = op_desc_ptr->GetOutputDesc(kIndex);
  GeTensorPtr output_ptr = MakeShared<ge::GeTensor>(tensor_desc);
  if (output_ptr == nullptr) {
    GELOGE(PARAM_INVALID, "ndoe [%s] make shared failed", op_name.c_str());
    return PARAM_INVALID;
  }

  auto input_tensor = input[kIndex];
  if (input_tensor == nullptr) {
    GELOGE(PARAM_INVALID, "node [%s] get input failed.", op_name.c_str());
    return PARAM_INVALID;
  }

  if (output_ptr->SetData(input_tensor->GetData()) != GRAPH_SUCCESS) {
    GELOGW("Compute: SetData failed");
    return NOT_CHANGED;
  }
  v_output.emplace_back(output_ptr);
  GELOGD("UnsqueezeKernel success: node[%s]", op_name.c_str());
  return SUCCESS;
}
REGISTER_KERNEL(UNSQUEEZEV3, UnsqueezeV3Kernel);
}  // namespace ge
