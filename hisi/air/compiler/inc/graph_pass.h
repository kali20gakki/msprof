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

#ifndef GE_INC_GRAPH_PASS_H_
#define GE_INC_GRAPH_PASS_H_

#include <string>
#include <vector>

#include "framework/common/op/attr_value_util.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "inc/pass.h"

namespace ge {
///
/// @ingroup domi_omg
/// @brief graph pass
/// @author
///
class GraphPass : public Pass<ge::ComputeGraph> {
 public:
  ///
  /// run graph pass
  /// @param [in] graph graph to be optimized
  /// @return SUCCESS optimize successfully
  /// @return NOT_CHANGED not optimized
  /// @return others optimized failed
  /// @author
  ///
  virtual Status Run(ge::ComputeGraphPtr graph) = 0;
  virtual Status ClearStatus() { return SUCCESS; };
};
}  // namespace ge

#endif  // GE_INC_GRAPH_PASS_H_
