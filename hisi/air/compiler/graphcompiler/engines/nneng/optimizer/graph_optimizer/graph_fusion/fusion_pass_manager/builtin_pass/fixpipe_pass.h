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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PASS_H_

#include <map>
#include <queue>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/fe_log.h"
#include "fixpipe_addinputstrategy.h"
#include "fixpipe_common.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/graph_utils.h"
#include "graph/range_vistor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {

class FixPipePass : public PatternFusionBasePass {
 protected:
  std::vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, std::vector<ge::NodePtr> &new_nodes) override;

 private:
  bool IsCubeOpType(const ge::NodePtr &node_ptr) const;
  bool JudgeCachePass(const FixPipeNodeInfo &node, stack<uint32_t> &index, uint32_t &ret_index) const;
  void ClearPasses();
  Status JudgeIsMatch(const FixPipeNodeInfo &node, stack<uint32_t> &cur_index, uint32_t &ret_index) const;
  void GenerateMatchedPassesFromStack(const stack<FixPipeNodeInfo> &cur_pass,
                                      const uint32_t &pass_index, const uint32_t &cur_index);
  bool IsInWhitelist(const FixPipeNodeInfo &node) const;
  Status GenerateMatchedPasses(FixPipeNodeInfo &conv_node, std::vector<ge::NodePtr> &new_nodes);
  bool GetNodeIndex(const FixPipeNodeInfo &node, const uint32_t &index) const;
  void GenerateMatchedPassesImpl(FixPipeNodeInfo &node, stack<FixPipeNodeInfo> &cur_pass,
                                 stack<uint32_t> &cur_index, std::vector<ge::NodePtr> &new_nodes);
  void ChangeOrInsertPass(FixPipePassInfo &tmp_pass);
  Status ModfiyMatchedPasses();
  Status FiltrNodeStrategy(const FixPipeNodeInfo &node) const;
  Status FiltrNodeStrategyForTransData(const FixPipeNodeInfo &node) const;
  Status FiltrNodeStrategyForRelu(const FixPipeNodeInfo &node) const;
  Status FiltrNodeStrategyForEltWise(const FixPipeNodeInfo &node) const;
  Status FiltrNodeStrategyForQuant(const FixPipeNodeInfo &cur_node, const FixPipeNodeInfo &prenode) const;
  bool IsConfictWithSkipConfig(std::stack<uint32_t> index, const uint32_t &ret_index) const;
  bool IsConfictWithSkipConfig(const std::vector<uint32_t> &index, const uint32_t &ret_index) const;
  bool IsConfictWithSkipConfig(const FixPipePassInfo &cur_pass, const uint32_t &ret_index) const;
  bool ReadConfig();
  bool ReadConfig(const std::string &soc_version);
  std::string GetEltWiseType(const FixPipeNodeInfo &node) const;
  bool PreCachePass(const FixPipePassInfo &cur_pass, const FixPipeNodeInfo &node) const;
  bool PreMatchAcorrdingToPass(const FixPipePassInfo &cur_pass, const FixPipeNodeInfo &node) const;
  Status RelinkOpEdges(FixPipeNodeInfo &head_node,
                       const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode);
  bool IsNodeInPass(const std::vector<FixPipeNodeInfo> &fixednodeids, const ge::NodePtr input_node) const;
  ge::NodePtr CreateFixpipeNode(const FixPipePassInfo &match_pass, const FixPipeNodeInfo &head_node,
                                ge::ComputeGraph &graph) const;
  bool NeedToCutPass(FixPipePassInfo &m_pass) const;
  Status AddInputs(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                   ge::NodePtr fixpipenode, std::vector<ge::NodePtr> &new_nodes);
  Status AddInputDesc(ge::OpDescPtr cur_new_fixpipeopdesc) const;
  Status RelinkHeadEdges(FixPipeNodeInfo &head_node, FixPipeNodeInfo &fixpipeenhancenode) const;
  Status RelinkOutputEdges(const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode) const;
  Status RelinkEltWiseEdges(const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode);
  Status RelinkEltWiseEdgesImpl(const FixPipePassInfo &match_pass, FixPipeNodeInfo &tofuzednode,
                                FixPipeNodeInfo &fixpipeenhancenode);                          
  Status RelinkOtherEltWiseEdges(ge::OutDataAnchorPtr peer_anchor, const FixPipeNodeInfo &tofuzednode,
                                 FixPipeNodeInfo &fixpipeenhancenode, ge::NodePtr inputfather_node) const;
  Status RelinkAntiEltWiseEdges(ge::NodePtr inputfather_node, const FixPipeNodeInfo &tofuzednode,
                                FixPipeNodeInfo &fixpipeenhancenode);
  Status DeleteToFusedNodeEdge(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                               std::vector<ge::NodePtr> &todeletenode) const;
  Status DeleteNode(const std::vector<ge::NodePtr> &todeletenode);
  Status Init();
  Status InitInput2();
  Status InitInput3();
  Status InitInput4();
  Status InitInput5();
  Status InitInput6();
  Status InitInput8();
  Status InitInput9();
  Status InitInputDefault();
  FixPipeAddInputPtr AddInputStrategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput2Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput3Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput4Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput5Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput6Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput7Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput8Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInput9Strategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam);
  FixPipeAddInputPtr AddInputAntiStrategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam,
                                          const std::string &first_unitname);
  FixPipeAddInputPtr AddInputSingleUnitStrategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam,
                                                const std::string &first_unitname);
  FixPipeAddInputPtr AddInputTwoUnitStrategy(const FixPipePassInfo &match_pass, FixPipeFunctionParamPtr funtcparam,
                                             const std::string &first_unitname,
                                             const std::string &second_unitname);
  Status FusionImpl(FixPipeNodeInfo &head_node, ge::ComputeGraph &graph, std::vector<ge::NodePtr> &new_nodes);
  std::vector<FixPipePassInfo> m_matchpasses_;
  std::vector<FixPipeNodePair> m_toantiquantnodes_;
  std::vector<FixPipeUnit> m_idxtonodetypes_;
  std::map<std::string, FixPipeAddInputPtr> m_opmaps_;
  std::map<std::string, uint32_t> unitmapindex_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_FIXPIPE_PASS_H_
