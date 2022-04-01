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

#ifndef INC_1249A0BF64B44060BAF658B265D6D356
#define INC_1249A0BF64B44060BAF658B265D6D356

#include "stdint.h"
#include "vector"
#include "ge_running_env/fake_ns.h"
#include "graph/ge_tensor.h"

FAKE_NS_BEGIN

class GeTensorDesc;

struct TensorDescs {
  TensorDescs(int size);
  std::vector<GeTensorDesc>& Value(){
    return tensor_descs_;
  }

 private:
  std::vector<GeTensorDesc> tensor_descs_;
};

FAKE_NS_END

#endif