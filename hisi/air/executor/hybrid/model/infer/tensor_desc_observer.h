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
#ifndef GE_HYBRID_MODEL_INFER_TENSOR_DESC_OBSERVER_H
#define GE_HYBRID_MODEL_INFER_TENSOR_DESC_OBSERVER_H

#include "ge/ge_api_error_codes.h"
#include "hybrid/model/infer/tensor_desc_holder.h"

namespace ge {
namespace hybrid {
struct ChangeObserver {
  ChangeObserver() = default;
  virtual ~ChangeObserver() = default;
  virtual void Changed() = 0;

protected:
  ChangeObserver(const ChangeObserver &change) = delete;
  ChangeObserver &operator=(const ChangeObserver &change) = delete;
};

struct TensorDescObserver {
  explicit TensorDescObserver(const TensorDescHolder &holder);
  TensorDescObserver(GeTensorDesc &tensor_desc, ChangeObserver &change_server)
      : tensor_desc_holder_(tensor_desc), change_server_(change_server) {}
  Status OnChanged(const GeTensorDesc &source);

private:
  TensorDescHolder tensor_desc_holder_;
  ChangeObserver &change_server_;
};
} // namespace hybrid
} // namespace ge
#endif // GE_HYBRID_MODEL_INFER_TENSOR_DESC_OBSERVER_H
