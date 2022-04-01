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

#include "host_kernels/size_kernel.h"

#include <memory>

#include "framework/common/types.h"
#include "host_kernels/kernel_utils.h"
#include "inc/kernel_factory.h"

namespace ge {
namespace {
const size_t kSizeInputSize = 1U;
const size_t kSizeOutputSize = 1U;
REGISTER_KERNEL(SIZE, SizeKernel);
}  // namespace

Status SizeKernel::Compute(const NodePtr &node, std::vector<GeTensorPtr> &v_output) const {
  GELOGD("SizeKernel in");
  if (node == nullptr) {
    GELOGE(FAILED, "parameter is null.");
    return FAILED;
  }
  const OpDescPtr op_desc = node->GetOpDesc();
  if (op_desc == nullptr) {
    GELOGE(PARAM_INVALID, "node:%s opdesc is null", node->GetName().c_str());
    return PARAM_INVALID;
  }

  const bool size_check = ((op_desc->GetInputsSize() != kSizeInputSize) ||
      (op_desc->GetOutputsSize() != kSizeOutputSize));
  if (size_check) {
    GELOGW("SizeKernel input size check fail, GetInputsSize:%zu", op_desc->GetInputsSize());
    return NOT_CHANGED;
  }

  if (KernelUtils::IsUnknownShape(op_desc->GetInputDesc(0U).GetShape())) {
    GELOGW("Input shape is unknown, ignore size kernel.");
    return NOT_CHANGED;
  }

  int64_t size = 1;
  // Calculate the number of elements of the sensor
  for (const int64_t dim : op_desc->GetInputDesc(0U).GetShape().GetDims()) {
    if (!CheckInt64MulOverflow(size, dim)) {
      GELOGE(INTERNAL_ERROR, "int64 overflow!");
      return INTERNAL_ERROR;
    }
    size *= dim;
  }

  const std::vector<int64_t> data{size};
  const Status ret = KernelUtils::ConstructTensorDescWithData(op_desc->GetOutputDesc(0U), data, v_output, true);
  if (ret != SUCCESS) {
    GELOGE(ret, "Size kernel construct tensor desc fail.");
    return ret;
  }

  GELOGD("Size kernel success");
  return SUCCESS;
}
}  // namespace ge
