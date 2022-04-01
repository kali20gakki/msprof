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

#include "hybrid/model/infer/tensor_desc_holder.h"
#include "hybrid/model/node_item.h"

namespace ge {
namespace hybrid {

bool TensorDescHolder::operator==(const TensorDescHolder &left) const {
  return (this->index_ == left.index_) && (this->node_item_ == left.node_item_);
}

GeTensorDesc& TensorDescHolder::Input() const {
  if (index_ == INT32_MAX) {
    return *tensor_desc_;
  }
  return *(node_item_->MutableInputDesc(index_).get());
}

GeTensorDesc& TensorDescHolder::Output() const {
  if (index_ == INT32_MAX) {
    return *tensor_desc_;
  }
  return *(node_item_->MutableOutputDesc(index_).get());
}
} // namespace hybrid
} // namespace ge
