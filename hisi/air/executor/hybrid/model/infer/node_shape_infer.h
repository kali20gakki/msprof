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

#ifndef AIR_CXX_NODE_SHAPE_INFER_H
#define AIR_CXX_NODE_SHAPE_INFER_H

#include <vector>
#include "node_shape_propagator.h"
#include "ge/ge_api_error_codes.h"
#include "graph/types.h"
#include "graph/op_desc.h"

namespace ge {
namespace hybrid {
struct NodeShapeInfer : NodeShapePropagator {

  const std::string &NodeName() const {
    return node_name;
  }

  const std::string &NodeType() const {
    return node_type;
  }

  inline bool IsShapeInFuture() const {
    return (shape_inference_type == DEPEND_SHAPE_RANGE) || (shape_inference_type == DEPEND_COMPUTE);
  }

  Status CalcOutputTensorSizes(const bool fallback_with_range = false) const;
  Status OnNodeDone() const;

  bool IsInputShapeStatic(const int32_t index) const;

public:
  std::string node_name;
  std::string node_type;
  int32_t group = -1;
  bool is_dynamic = false;
  bool is_output_shape_static = true;
  bool is_need_force_infershape = false;
  OpDesc *op_desc;
  UnknowShapeOpType shape_inference_type = DEPEND_IN_SHAPE;

protected:
  std::vector<bool> is_input_shape_static_;
};
} // namespace hybrid
} // namespace ge
#endif 
