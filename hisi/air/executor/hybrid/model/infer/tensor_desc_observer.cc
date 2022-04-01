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
#include "hybrid/model/infer/tensor_desc_observer.h"
#include "graph/ge_tensor.h"
#include "hybrid/model/infer/shape_utils.h"
#include "common/debug/log.h"

namespace ge {
namespace hybrid {

struct DefaultChangeObserver : ChangeObserver {
  void Changed() override {};
} default_observer;

TensorDescObserver::TensorDescObserver(const TensorDescHolder &holder)
    : tensor_desc_holder_(holder), change_server_(default_observer) {}

Status TensorDescObserver::OnChanged(const GeTensorDesc &source) {
  GE_CHK_STATUS_RET_NOLOG(ShapeUtils::CopyShapeAndTensorSize(source, tensor_desc_holder_.Input()));
  change_server_.Changed();
  return SUCCESS;
}
} // namespace hybrid
} // namespace ge