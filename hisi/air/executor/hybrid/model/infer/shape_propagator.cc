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

#include "hybrid/model/infer/shape_propagator.h"

#include "common/debug/log.h"

namespace ge {
namespace hybrid {

Status ShapePropagator::DoPropagate() const {
  for (auto observer : observers_) {
    GE_CHK_STATUS_RET_NOLOG(observer.OnChanged(tensor_desc_holder_.Output()));
  }
  return SUCCESS;
}

bool ShapePropagator::operator==(const ShapePropagator &left) const {
  return this->tensor_desc_holder_== left.tensor_desc_holder_;
}

void ShapePropagator::PropagateTo(const TensorDescObserver &observer) {
  observers_.push_back(observer);
}
} // namespace hybrid
} // namespace ge
