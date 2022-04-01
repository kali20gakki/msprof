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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_INPUTER_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_INPUTER_H_

#include <memory>
#include "common/blocking_queue.h"
#include "framework/common/ge_types.h"
#include "framework/common/ge_inner_error_codes.h"

namespace ge {
///
/// @ingroup domi_ome
/// @brief wrapper input data
/// @author
///
class InputDataWrapper {
 public:
  InputDataWrapper(const InputData &input_data, const OutputData &output_data);

  ~InputDataWrapper() {}

  ///
  /// @ingroup domi_ome
  /// @brief init InputData
  /// @param [in] input use input to init InputData
  /// @param [in] output data copy dest address
  /// @return SUCCESS   success
  /// @return other             init failed
  ///
  OutputData *GetOutput() { return &output_; }

  ///
  /// @ingroup domi_ome
  /// @brief return InputData
  /// @return InputData
  ///
  const InputData &GetInput() const { return input_; }

 private:
  OutputData output_;
  InputData input_;
};

///
/// @ingroup domi_ome
/// @brief manage data input
/// @author
///
class DataInputer {
 public:
  ///
  /// @ingroup domi_ome
  /// @brief constructor
  ///
  DataInputer() {}

  ///
  /// @ingroup domi_ome
  /// @brief destructor
  ///
  ~DataInputer() {}

  ///
  /// @ingroup domi_ome
  /// @brief add input data
  /// @param [int] input data
  /// @return SUCCESS add successful
  /// @return INTERNAL_ERROR  add failed
  ///
  Status Push(const std::shared_ptr<InputDataWrapper> &data) {
    return queue_.Push(data, false) ? SUCCESS : INTERNAL_ERROR;
  }

  ///
  /// @ingroup domi_ome
  /// @brief pop input data
  /// @param [out] save popped input data
  /// @return SUCCESS pop success
  /// @return INTERNAL_ERROR  pop fail
  ///
  Status Pop(std::shared_ptr<InputDataWrapper> &data) {
    return queue_.Pop(data) ? SUCCESS : INTERNAL_ERROR;
  }

  ///
  /// @ingroup domi_ome
  /// @brief stop receiving data, invoke thread at Pop
  ///
  void Stop() { queue_.Stop(); }

  uint32_t Size() { return queue_.Size(); }

 private:
  ///
  /// @ingroup domi_ome
  /// @brief save input data queue
  ///
  BlockingQueue<std::shared_ptr<InputDataWrapper>> queue_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_INPUTER_H_
