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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_CONCAT_N_OPTIMIZER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_CONCAT_N_OPTIMIZER_H_

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
class ConcatOptimizer {
 public:
  /**
   *  @ingroup fe
   *  @brief set fusion_virtual_op info for op
   *  @param [in|out] graph compute graph
   *  @return SUCCESS or FAILED
   */
  Status SetFusionVirtualOp(const ge::ComputeGraph &graph) const;
  bool InputCheck(ge::NodePtr concat_node) const;
  bool OutputCheck(ge::NodePtr concat_node) const;
  bool IsPeerNodeCanReUsed(const ge::NodePtr &concat_node) const;
  bool IsPreNodeAttrValid(const ge::OpDescPtr &pre_node_desc, bool &fusion_virtual_op_flag,
                          const string &node_name) const;
  bool HasSameSrc(const ge::NodePtr &concat_node) const;
  void GetFirstOutAnchorNotInDeleteList(const ge::InDataAnchorPtr &input_anchor,
                                        ge::OutDataAnchorPtr &src_anchor, int current_deep) const;
  void SetPeerNodeWhetherCanReUsed(const ge::NodePtr &concat_node) const;

 private:
  bool NeedSkip(const ge::NodePtr &node, const ge::OpDescPtr &op_desc) const;
  bool CheckConcatInputAligned(const ge::OpDescPtr &op_desc_ptr) const;
  void GetRealConcatDimFromOriginalFormatToFormat(const ge::OpDescPtr &op_desc, int64_t &concat_dim) const;
  bool Check32Align(ge::NodePtr concat_node) const;
  Status CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size, int32_t &output_real_calc_flag) const;
  bool ValidInputNode(ge::NodePtr concat_node) const;
  static bool IsAiCoreOp(const ge::NodePtr &node);
  static bool HasControlEdge(ge::NodePtr concat_node);
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_CONCAT_N_OPTIMIZER_H_