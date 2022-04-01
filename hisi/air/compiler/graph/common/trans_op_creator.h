/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef GE_GRAPH_TRANS_OP_CREATOR_H_
#define GE_GRAPH_TRANS_OP_CREATOR_H_

#include <string>

#include "external/graph/types.h"
#include "graph/op_desc.h"

namespace ge {
class TransOpCreator {
 public:
  static OpDescPtr CreateTransDataOp(const std::string &op_name, Format src_format, Format dst_format);

  static OpDescPtr CreateTransPoseDOp(const std::string &op_name, const Format src_format, const Format dst_format);

  static OpDescPtr CreateCastOp(const std::string &op_name, const DataType input_dtype, const DataType output_dtype);

  TransOpCreator() = default;
  TransOpCreator(TransOpCreator &&) = delete;
  TransOpCreator(const TransOpCreator &) = delete;
  TransOpCreator &operator=(const TransOpCreator &) = delete;
};
}  // namespace ge

#endif  // GE_GRAPH_TRANS_OP_CREATOR_H_
