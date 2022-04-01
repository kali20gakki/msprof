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
#ifndef GE_HYBRID_MODEL_INFER_SHAPE_PROPAGATOR_H
#define GE_HYBRID_MODEL_INFER_SHAPE_PROPAGATOR_H

#include "hybrid/model/infer/tensor_desc_observer.h"

namespace ge {
namespace hybrid {
struct ShapePropagator {
  explicit ShapePropagator(const TensorDescHolder &holder) : tensor_desc_holder_(holder) {}
  Status DoPropagate() const;
  void PropagateTo(const TensorDescObserver &observer);

public:
  bool operator==(const ShapePropagator &left) const;

private:
  const TensorDescHolder tensor_desc_holder_;
  std::vector<TensorDescObserver> observers_;
};
} // namespace hybrid
} // namespace ge

#endif // GE_HYBRID_MODEL_INFER_SHAPE_PROPAGATOR_H
