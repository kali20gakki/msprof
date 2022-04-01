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

#include "host_kernels/rank_kernel.h"

#include <memory>
#include <vector>

#include "external/graph/types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/debug/ge_log.h"
#include "inc/kernel_factory.h"
#include "framework/omg/omg_inner_types.h"
#include "framework/common/types.h"
#include "graph/def_types.h"

namespace ge {
namespace {
const size_t kRankInputSize = 1U;
const uint32_t kRankDataInputIndex = 0U;
REGISTER_KERNEL(RANK, RankKernel);
}  // namespace

Status RankKernel::Compute(const NodePtr &node, std::vector<GeTensorPtr> &v_output) const {
  if (node == nullptr) {
    GELOGE(FAILED, "parameter is null.");
    return FAILED;
  }
  const OpDescPtr op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const size_t input_node_size = op_desc->GetInputsSize();
  if (input_node_size != kRankInputSize) {
    GELOGW("input node size must be %zu", kRankInputSize);
    return NOT_CHANGED;
  }

  const auto &input_shape = op_desc->MutableInputDesc(kRankDataInputIndex);
  GE_CHECK_NOTNULL(input_shape);
  if (input_shape->GetShape().GetDims() == UNKNOWN_RANK) {
    return NOT_CHANGED;
  }
  const auto ndims = input_shape->GetShape().GetDimNum();
  GeTensorDesc tensor_desc(op_desc->GetOutputDesc(0U));
  const GeTensorPtr output_ptr = MakeShared<ge::GeTensor>(tensor_desc, PtrToPtr<const size_t, const uint8_t>(&ndims),
                                                          GetSizeByDataType(DT_INT32));
  if (output_ptr == nullptr) {
    GELOGE(MEMALLOC_FAILED, "make_shared ge::GeTensor failed");
    return MEMALLOC_FAILED;
  }
  v_output.push_back(output_ptr);
  return SUCCESS;
}
}  // namespace ge