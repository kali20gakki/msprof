/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_NODE_OPTIMIZE_CHECKER_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_NODE_OPTIMIZE_CHECKER_BASE_H_

#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/common/node_optimize_utils.h"

namespace fe {
class NodeOptimizeCheckerBase {
 public:
  /**
   * Check if wee need to optimize the C aixs.
   * @param node_ptr node
   * @param dim_attr the attribute name of the aixs
   * @param is_input is input or output
   * @return true or false
   */
  bool IsDimC(const ge::NodePtr &node_ptr, const string &dim_attr, bool is_input) const;

  /**
   * Get the dim_c.
   * @param tensor_desc input or output desc
   * @param dim_c result
   * @return SUCCESS or FAILED
   */
  Status GetDimC(const ge::GeTensorDesc &tensor_desc, int &dim_c) const;

  /**
   * Get the postion of the dim_c, if the dim_c <0, update it to a positive
   * number.
   * @param tensor_desc input or output desc
   * @param pos result
   * @return SUCCESS or FAILED
   */
  Status GetPosOfDimC(const ge::GeTensorDesc &tensor_desc, int &pos) const;

  /**
   * Check if the dim C of the input is aligned by 16(float16) or 32(int8).
   * @param tensor_desc input or output desc
   * @param dim_c result
   * @return SUCCESS or FAILED
   */
  bool IsDimCOfInputAligned(const ge::GeTensorDesc &tensor_desc, const int &dim_c,
                            const ge::DataType &quant_data_type) const;

  /**
   * Check if input is data.
   * @param node_ptr node
   * @return true or false
   */
  bool IsInputNotData(const ge::NodePtr &node_ptr) const;
  const string QUANT = "AscendQuant";
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_NODE_OPTIMIZE_CHECKER_BASE_H_
