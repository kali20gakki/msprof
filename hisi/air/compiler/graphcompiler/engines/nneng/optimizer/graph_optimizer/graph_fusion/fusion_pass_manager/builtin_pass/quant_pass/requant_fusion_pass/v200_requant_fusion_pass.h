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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_REQUANT_FUSION_PASS_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_REQUANT_FUSION_PASS_H

#include <vector>
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "external/graph/types.h"

namespace fe {

class V200RequantFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  ge::NodePtr GetFirstNoConstInput(const ge::NodePtr &node_ptr) const;
  bool NotRemoveLeakyRelu(ge::NodePtr node) const;
  Status CheckConcatInputNode(const ge::NodePtr &concat_node) const;
  Status SetRequantReluFlag(const ge::NodePtr &requant_node, const int8_t &offset_quant, const bool &relu_flag) const;
  Status GetBiasValue(const ge::NodePtr &dequant_node, const string &quant_mode, vector<ge::NodePtr> &cube_nodes,
                      int32_t &bias_size, bool &del_bias_flag, vector<int32_t> &bias_value) const;
  void DealWithCubeNodes(ge::ComputeGraph &graph, vector<ge::NodePtr> &cube_nodes, const bool &del_bias_flag,
                         const bool &no_bias_s9_flag) const;
  Status RefreshNodeDtype(ge::NodePtr &next_node, const ge::DataType &data_type) const;

  Status DealDequantV200(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequant_nodes,
                         vector<ge::NodePtr> &quants, float scale_quant, int8_t offset_quant,
                         vector<ge::NodePtr> &fusion_nodes) const;
  Status DealQuant(ge::ComputeGraph &graph, vector<ge::NodePtr> &quant_nodes) const;
  Status DealQuantScale(vector<ge::NodePtr> &quant_nodes) const;
  bool CheckDequantNotRequant(const vector<ge::NodePtr> &dequants) const;
  Status RequantProcess(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequants,
                        vector<ge::NodePtr> &quants, vector<ge::NodePtr> &relus,
                        float &scale_quant, int8_t &offset_quant,
                        vector<ge::NodePtr> &new_nodes) const;
  Status CheckQuantOp(const ge::NodePtr &node_ptr, vector<ge::NodePtr> &quants, int &quant_num, float &scale_base,
                      int8_t &offset_base) const;
  Status Pattern0Parse(ge::NodePtr dequant_node, vector<ge::NodePtr> &dequants,
                       vector<ge::NodePtr> &quants, vector<ge::NodePtr> &relus, float &scale_quant,
                       int8_t &offset_quant) const;
  Status HandlePattern0(ge::ComputeGraph &graph, ge::NodePtr dequant_node, vector<ge::NodePtr> &new_nodes) const;
  bool IsConcatDimC(const ge::NodePtr &node_ptr, int32_t &dim_attr_value, const uint32_t &const_index) const;
  Status CheckConcatDOpAligned(const ge::NodePtr &concat_node, const ge::DataType &data_type) const;
  Status CheckConcatOpAligned(ge::NodePtr &concat_node, ge::DataType &data_type);
  bool IsConstToAttrInput(const ge::NodePtr &concat_node, const ge::NodePtr &const_node) const;
  Status CheckOpInputAligned(ge::NodePtr &concat_node, ge::DataType &data_type);
  Status CheckConcatOpInput(const ge::NodePtr &concat_node, vector<ge::NodePtr> &dequants,
                            vector<ge::NodePtr> &relus) const;
  Status CheckConcatOpInput(const ge::NodePtr &concat_node, const ge::NodePtr &relu_node, vector<ge::NodePtr> &dequants,
                            vector<ge::NodePtr> &relus) const;
  Status Pattern1Parse(ge::NodePtr quant_node, vector<ge::NodePtr> &dequants,
                       vector<ge::NodePtr> &quants, vector<ge::NodePtr> &relus, float &scale_quant,
                       int8_t &offset_quant);
  Status HandlePattern1(ge::ComputeGraph &graph, ge::NodePtr quant_node, vector<ge::NodePtr> &new_nodes);
  bool JudgeCondition(const bool &has_relu, const int &relu_num, const int &direct_out_node_num,
                      const int &quant_num, const int &relu_del) const;

};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_REQUANT_FUSION_PASS_H
