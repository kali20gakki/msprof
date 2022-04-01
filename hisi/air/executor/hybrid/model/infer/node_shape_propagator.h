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
#ifndef AIR_CXX_NODE_SHAPE_PROPAGATOR_H
#define AIR_CXX_NODE_SHAPE_PROPAGATOR_H

#include "shape_propagator.h"

namespace ge {
class OpDesc;
namespace hybrid {
struct NodeShapePropagator {
  NodeShapePropagator() = default;
  void CreatePropagator(const TensorDescHolder &holder, const TensorDescObserver &observer);
  Status DoPropagate() const;

private:
  std::vector<ShapePropagator> shape_propagators_;
};

} // namespace hybrid
} // namespace ge
#endif
