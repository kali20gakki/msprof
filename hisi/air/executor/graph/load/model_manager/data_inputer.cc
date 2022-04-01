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

#include "graph/load/model_manager/data_inputer.h"

#include "framework/common/debug/log.h"

namespace ge {
/// @ingroup domi_ome
/// @brief init InputData
/// @param [in] input_data use input data to init InputData
/// @param [in] output_data use output data to init OutputData
InputDataWrapper::InputDataWrapper(const InputData &input_data, const OutputData &output_data) {
  input_ = input_data;
  output_ = output_data;
}
}  // namespace ge
