/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef AICPU_TENSORFLOW_UTIL_H_
#define AICPU_TENSORFLOW_UTIL_H_

#include <string>
#include "proto/tensorflow/node_def.pb.h"

namespace aicpu {
class TensorFlowUtil {
 public:
  /**
   * Add tf node attr
   * @param attr_name Attr name
   * @param attr_value Attr value
   * @param node_def Node def
   * @return true: success  false: failed
  */
  static bool AddNodeAttr(const std::string &attr_name,
                          const domi::tensorflow::AttrValue &attr_value,
                          domi::tensorflow::NodeDef *node_def);

  /**
   * Find attr from node def
   * @param node_def Node def
   * @param attr_name Attr name
   * @param attr_value Attr value
   * @return true: success  false: failed
  */
  static bool FindAttrValue(const domi::tensorflow::NodeDef *node_def,
                            const std::string attr_name,
                            domi::tensorflow::AttrValue &attr_value);
};
} // namespace aicpu
#endif // AICPU_TENSORFLOW_UTIL_H_

