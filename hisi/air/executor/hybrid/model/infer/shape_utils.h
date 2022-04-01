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

#ifndef AIR_CXX_SHAPE_UTILS_H
#define AIR_CXX_SHAPE_UTILS_H

#include "ge/ge_api_error_codes.h"

namespace ge {
class GeTensorDesc;
namespace hybrid {
  struct ShapeUtils{
    static Status CopyShapeAndTensorSize(const GeTensorDesc &from, GeTensorDesc &to);
  };
}
} // namespace ge
#endif  // AIR_CXX_SHAPE_UTILS_H
