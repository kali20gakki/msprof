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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_N_OPTIMIZER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_N_OPTIMIZER_H_

#include <map>
#include <memory>
#include <string>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/op_info_common.h"
#include "common/optimizer/graph_optimizer.h"
#include "common/optimizer/graph_optimizer_types.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/spacesize_calculator/tensor_compute_util.h"

namespace fe {
/** @brief provide two interface: 1. optimize original graph 2. optimize fused
* sub graph */
class SplitOptimizer {
 public:
  /**
   *  @ingroup fe
   *  @brief set fusion_virtual_op info for op
   *  @param [in|out] graph compute graph
   *  @return SUCCESS or FAILED
   */
  Status SetFusionVirtualOp(const ge::ComputeGraph &graph) const;
  bool ValidInput(ge::NodePtr split_node) const;
  bool InputCheck(ge::NodePtr split_node) const;
  bool OutputCheck(ge::NodePtr split_node) const;

 private:
  bool NeedSkip(const ge::NodePtr &node, const ge::OpDescPtr &op_desc) const;
  void GetRealSplitDimFromOriginalFormatToFormat(const ge::OpDescPtr &op_desc, int64_t &split_dim) const;
  static bool InvalidNodeType(const string& node_type);
  static bool InvalidNodeAttr(const ge::OpDescPtr& node_desc);
  static bool HasControlEdge(ge::NodePtr split_node);
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_N_OPTIMIZER_H_
