/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include "utils/tensor_descs.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"

USING_FAKE_NS;

TensorDescs::TensorDescs(int size) {
  for (int i = 0; i < size; i++) {
    GeTensorDesc tensor_desc_0(GeShape(), FORMAT_NCHW, DT_INT32);
    AttrUtils::SetInt(tensor_desc_0, ATTR_NAME_PLACEMENT, 1);
    tensor_descs_.emplace_back(tensor_desc_0);
  }
}

