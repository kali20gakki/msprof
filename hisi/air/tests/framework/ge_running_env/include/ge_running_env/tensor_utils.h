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

#ifndef AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_TENSOR_UTILS_H_
#define AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_TENSOR_UTILS_H_
#include "fake_ns.h"
#include "graph/ge_tensor.h"
#include <vector>
#include <memory>
FAKE_NS_BEGIN
inline GeTensorPtr GenerateTensor(DataType dt, const std::vector<int64_t> &shape) {
  size_t total_num = 1;
  for (auto dim : shape) {
    total_num *= dim; // 未考虑溢出
  }

  auto size = GetSizeInBytes(static_cast<int64_t>(total_num), dt);
  auto data_value = std::unique_ptr<uint8_t[]>(new uint8_t[size]());
  GeTensorDesc data_tensor_desc(GeShape(shape), FORMAT_ND, dt);
  return std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)(data_value.get()),size);
}
inline GeTensorPtr GenerateTensor(const std::vector<int64_t> &shape) {
  return GenerateTensor(DT_FLOAT, shape);
}
inline GeTensorPtr GenerateTensor(DataType dt, std::initializer_list<int64_t> shape) {
  return GenerateTensor(dt, std::vector<int64_t>(shape));
}
inline GeTensorPtr GenerateTensor(std::initializer_list<int64_t> shape) {
  return GenerateTensor(std::vector<int64_t>(shape));
}


FAKE_NS_END
#endif //AIR_CXX_TESTS_FRAMEWORK_GE_RUNNING_ENV_INCLUDE_GE_RUNNING_ENV_TENSOR_UTILS_H_
