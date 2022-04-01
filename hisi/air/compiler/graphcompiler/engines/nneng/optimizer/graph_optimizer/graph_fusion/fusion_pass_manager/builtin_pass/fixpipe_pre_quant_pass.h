/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PRE_QUANT_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PRE_QUANT_PASS_H_
#include <map>
#include <string>
#include <vector>

#include "common/fe_log.h"
#include "fixpipe_common.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/utils/graph_utils.h"
#include "graph/op_desc.h"
#include "graph/range_vistor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {

class FixPipePreQuantPass : public PatternFusionBasePass {
 protected:
  std::vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, std::vector<ge::NodePtr> &new_nodes) override;

 private:
  bool ReadConfig();
  bool ReadConfig(const std::string &soc_version);
  Status Pattern0Parse(const ge::NodePtr &dequant_node, vector<ge::NodePtr> &quants,
                       std::map<ge::NodePtr, vector<ge::NodePtr>> &relus) const;
  Status ReDequantProcess(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                          vector<ge::NodePtr> &quants, std::map<ge::NodePtr, vector<ge::NodePtr>> &relus,
                          vector<ge::NodePtr> &new_nodes) const;
  Status DealDequant(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                     vector<ge::NodePtr> &quants, std::map<ge::NodePtr, vector<ge::NodePtr>> &relus,
                     vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &new_nodes) const;
  Status DealQuant(vector<ge::NodePtr> &quant_nodes) const;
  Status DealRelus(ge::NodePtr &dequant_node,
                   std::map<ge::NodePtr, vector<ge::NodePtr>> &relus) const;
  Status HandlePattern0(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                        vector<ge::NodePtr> &new_nodes) const;
  ge::NodePtr DealDequantInput(ge::OpDescPtr op_desc_tmp, ge::ComputeGraph &graph, const ge::NodePtr &dequant,
                               const ge::NodePtr &quant, vector<ge::NodePtr> &new_nodes) const;
  Status DeleteNode(ge::NodePtr todeletenode) const;
  Status UpDateInputData(ge::GeTensorPtr dequant_input, ge::DataType data_type, float &scale) const;
  Status RelinkDequantWithQuant(ge::NodePtr head_node, ge::NodePtr enhanceddequantnode, ge::NodePtr dequant_node,
                                ge::NodePtr quant_node) const;
  Status RelinkReluWithQuant(ge::NodePtr head_node, ge::NodePtr enhanceddequantnode, ge::NodePtr new_relu_node,
                             ge::NodePtr relu_node, ge::NodePtr quant_node) const;
  Status DeleteDequant(ge::NodePtr dequant_node) const;
  Status JudgeQuantReluValue(const ge::NodePtr &dequant_node, const ge::NodePtr &relu_node,
                             const ge::NodePtr &quant_node) const;
  Status FusionImpl(ge::NodePtr head_node, ge::ComputeGraph &graph,
                    std::vector<ge::NodePtr> &new_nodes) const;
  std::vector<std::string> m_units_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PRE_QUANT_PASS_H_
