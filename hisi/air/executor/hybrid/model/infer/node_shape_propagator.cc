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
 * WITHOUT
  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hybrid/model/infer/node_shape_propagator.h"
#include "hybrid/model/infer/shape_propagator.h"
#include "common/debug/ge_log.h"
#include "common/debug/log.h"

namespace ge {
namespace hybrid {

void NodeShapePropagator::CreatePropagator(const TensorDescHolder &holder, const TensorDescObserver &observer) {
  const ShapePropagator new_propagator(holder);
  for (auto& propagator: shape_propagators_) {
    if (propagator == new_propagator) {
      return propagator.PropagateTo(observer);
    }
  }
  shape_propagators_.emplace_back(holder);
  shape_propagators_.back().PropagateTo(observer);
}

Status NodeShapePropagator::DoPropagate() const {
  for (auto &cp_tensor : shape_propagators_) {
    GE_CHK_STATUS_RET_NOLOG(cp_tensor.DoPropagate());
  }
  return SUCCESS;
}
} // namespace hybrid
} // namespace ge