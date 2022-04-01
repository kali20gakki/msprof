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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONV_WEIGHT_COMPRESS_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONV_WEIGHT_COMPRESS_FUSION_PASS_H_

#include <vector>
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
class ConvWeightCompressFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  /**
   * whether convert the conv node to compress conv node
   * @param node_ptr node pointer
   * @return result
   */
  bool IsCompressWeight(ge::NodePtr node_ptr) const;

  /**
   * create and fill in parameters for conv compress op desc
   * @param conv_op_desc op of conv2d/fc
   * @param conv_compress_op_desc target op
   * @return SUCCESS / FAILED
   */
  Status CreateConvCompressOpDesc(ge::OpDescPtr conv_op_desc, ge::OpDescPtr &conv_compress_op_desc) const;

  ge::NodePtr CreateHostNode(const ge::OpDescPtr &conv_op_desc, ge::ComputeGraph &graph) const;

  ge::NodePtr CreateSwitchNode(const ge::OpDescPtr &conv_op_desc, ge::ComputeGraph &graph) const;

  ge::NodePtr CreateMergeNode(const ge::OpDescPtr &conv_op_desc, ge::ComputeGraph &graph) const;

  bool CheckWeightConstFoldNode(ge::NodePtr conv_node_ptr) const;

  bool CheckConstFoldNode(ge::NodePtr node_ptr) const;

  void SetHostNodeCompressRatio(ge::NodePtr &node_ptr) const;

  Status RelinkNodeEdges(ge::NodePtr conv_node, ge::NodePtr conv_compress_node, ge::NodePtr host_node,
                         ge::NodePtr switch_node, ge::NodePtr merge_node) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONV_WEIGHT_COMPRESS_FUSION_PASS_H_
