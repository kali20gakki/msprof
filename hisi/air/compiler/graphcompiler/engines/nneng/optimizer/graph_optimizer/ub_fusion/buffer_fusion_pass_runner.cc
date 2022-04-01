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

#include "graph_optimizer/ub_fusion/buffer_fusion_pass_runner.h"
#include <queue>
#include "common/fusion_statistic/fusion_statistic_writer.h"
#include "common/unknown_shape_util.h"
#include "common/op_info_common.h"
#include "common/util/platform_info.h"
#include "common/configuration.h"
#include "common/scope_allocator.h"
#include "register/graph_optimizer/fusion_common/graph_pass_util.h"

namespace fe {
namespace {
static const char *STREAM_LABEL = "_stream_label";
}

BufferFusionPassRunner::BufferFusionPassRunner(const string &name,
                                               BufferFusionPassBase *(*create_fn)(),
                                               std::shared_ptr<FusionCycleDetector> &cycle_detector)
    : cycle_detector_(cycle_detector) {
  SetName(name);
  buffer_fusion_pass_base_ptr_ = std::unique_ptr<BufferFusionPassBase>(create_fn());
  buffer_fusion_pass_base_ptr_->SetName(name);
  cube_op_type_ = {"BasicLSTMCellV2", "MatMul", "MatMulV2", "BatchMatMul", "GEMM", "ROIAlign",
               "Pooling", "FullyConnection", "Conv2DBackpropFilterD",
               "Conv2DBackpropFilter", "Conv2DBackpropInputD", "Conv2DBackpropInput",
               "Deconvolution", "Conv2DTransposeD", "Conv2D", "DepthwiseConv2D",
               "DepthwiseConv2DBackpropFilterD", "DepthwiseConv2DBackpropInputD",
               "ROIPooling", "BasicLSTMCellWeightGrad", "LRN", "PSROIPooling", "Conv3D",
               "Conv3DBackpropInputD", "Conv3DTransposeD", "Conv3DBackpropFilterD"
  };
}

BufferFusionPassRunner::~BufferFusionPassRunner() {
  for (auto pattern : patterns_) {
    if (pattern != nullptr) {
      delete (pattern);
      pattern = nullptr;
    }
  }
}

size_t GetMappingSize(const BufferFusionMapping &mapping) {
  size_t mapping_size = 0;
  for (const auto &ele : mapping) {
    mapping_size += ele.second.size();
  }
  return mapping_size;
}

size_t GetDistinctNodeSize(const BufferFusionMapping &mapping) {
  std::set<ge::NodePtr> distinct_nodes;
  for (const auto &ele : mapping) {
    for (const auto &node : ele.second) {
      distinct_nodes.emplace(node);
    }
  }
  return distinct_nodes.size();
}

void PrintMapping(const BufferFusionMapping &mapping, string stage = "") {
  if (CheckLogLevel(FE, DLOG_DEBUG) == 1) {
    FE_LOGD("Print mapping at stage [%s]:", stage.c_str());

    for (auto &ele : mapping) {
      for (auto &node : ele.second) {
        FE_LOGD("[%s] : [%s]", ele.first->desc_name.c_str(),
                node->GetName().c_str());
      }
    }
  }
}

string AssmbleDescNames(const vector<BufferFusionOpDesc *> &curr_queue_descs) {
  string node_name;
  for (auto &desc : curr_queue_descs) {
    node_name += desc->desc_name;
    node_name += ", ";
  }
  return node_name;
}

/*
 * @brief: get pattern and match it from graph
 * @param [in] graph: original graph
 * @return bool: the result deal with pattern matching
 */
Status BufferFusionPassRunner::Run(ge::ComputeGraph &graph) {
  // 1. get pattern info
  patterns_ = buffer_fusion_pass_base_ptr_->DefinePatterns();
  FE_CHECK_NOTNULL(cycle_detector_);

  // 2. for all patterns
  for (BufferFusionPattern *pattern : patterns_) {
    if (pattern == nullptr) {
      continue;
    }
    string pattern_name = pattern->GetName();
    if (pattern->GetErrorCnt()) {
      REPORT_FE_ERROR("[SubGraphOpt][UB][Run] [%s] pattern has error config, error count is [%ld], and it's invalid.",
                      pattern_name.c_str(), pattern->GetErrorCnt());
      continue;
    }
    // 3. compare pattern op and graph op(include compare op type and TBE type)
    if (!RunOnePattern(graph, *pattern)) {
      FE_LOGW("Run Pass[%s]Pattern[%s] failed.", GetName().c_str(), pattern->GetName().c_str());
      continue;
    }
  }
  return SUCCESS;
}

/*
 * @brief: check if is Valid op for UB fusion
 * @param [in] node: graph node
 * @return bool: check result
 */
bool BufferFusionPassRunner::NeedIgnoreOp(ge::NodePtr node) {
  FE_CHECK((node == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][UbFusion][NeedIgnOp] null node in judging ValidOp"), return false);
  auto node_type = node->GetType();
  if (node_type == OP_TYPE_PLACE_HOLDER || node_type == OP_TYPE_END) {
    return true;
  }
  // TBE core, fused pattern and hasn't fused op can not be ignore
  if (!IsTbeOp(node)) {
    FE_LOGD("node [%s] is not tbe op, and will be skipped ub fusion.", node->GetName().c_str());
    return true;
  }

  if (!NodeType(node)) {
    FE_LOGD("Fusion pattern of node [%s] is not supported, which cannot be fused with any other ops.",
        node->GetName().c_str());
    return true;
  }

  int64_t scope_id = 0;
  if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id)) {
    FE_LOGD("This node[%s] has been fused.", node->GetName().c_str());
    return true;
  }

  return false;
}

/*
 * @brief: get a node's type presented by a enum type
 * @param [in] node: graph node
 * @return OPTYPE: type of the node
 */
bool BufferFusionPassRunner::NodeType(ge::NodePtr node) {
  FE_CHECK((node == nullptr), FE_LOGD("null node in judging NodeType"), return false);
  if (node->GetType() == TRANSDATA) {
    return true;
  }
  string type = "";
  if (GetOpAttrType(node, type)) {
    if (std::find(OP_PATTERN_VEC.begin(), OP_PATTERN_VEC.end(), type) != OP_PATTERN_VEC.end()) {
      return true;
    } else {
      FE_LOGD("Node[%s]: can not find op pattern [%s] in OP_PATTERN_VEC.", node->GetName().c_str(), type.c_str());
      return false;
    }
  }
  return false;
}

/*
 * @brief: get the optype of a node
 * @param [in] node: graph node
 * @param [out] op_type: type represent by string
 * @return bool: get op type ok or not
 */
bool BufferFusionPassRunner::GetOpAttrType(ge::NodePtr node, string &op_type, bool use_op_type) const {
  FE_CHECK((node == nullptr), REPORT_FE_ERROR("[SubGraphOpt][UbFusion][GetOpAttrType] node is nullptr."), return false);
  string name = node->GetName();
  auto key_str = name + "_pattern";

  if (use_op_type) {
    op_type = node->GetType();
    return true;
  }
  if (!ge::AttrUtils::GetStr(node->GetOpDesc(), key_str, op_type)) {
    FE_LOGD("node[%s] failed to get pattern [%s].", name.c_str(), key_str.c_str());
    return false;
  }

  if (op_type.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbFusion][GetOpAttrType] optype is empty for node name [%s].", name.c_str());
    return false;
  }
  return true;
}

bool BufferFusionPassRunner::IsOpTypeAny(const std::vector<string> &types) const {
  return find(types.begin(), types.end(), TBE_PATTERN_OP_TYPE_ANY) != types.end();
}

bool BufferFusionPassRunner::IsOutputNode(const std::vector<string> &types) const {
  return find(types.begin(), types.end(), TBE_PATTERN_OUTPUT_NODE) != types.end();
}

/*
 * @brief: check whether graph node is matched with pattern desc
 * @param [in] node: graph node
 * @param [in] op_desc: candidated pattern desc
 * @return bool: check result
 */
bool BufferFusionPassRunner::IsOpTypeExist(const ge::NodePtr node, const BufferFusionOpDesc *op_desc) const {
  string op_type;
  string name = node->GetName();
  const std::vector<string> types = op_desc->types;
  bool res = GetOpAttrType(node, op_type, op_desc->not_pattern);
  FE_LOGD("op type(pattern) of %s is %s.", op_type.c_str(),
          name.c_str());
  if (!res) {
    if (IsOutputNode(types)) {
      FE_LOGD("Node:[%s] is output node.", node->GetName().c_str());
      return true;
    } else {
      FE_LOGD("Node:[%s] is not output node.", node->GetName().c_str());
      return false;
    }
  }

  if (find(types.begin(), types.end(), op_type) != types.end()) {
    return true;
  } else {
    // return true while the desc type is "OpTypeAny"
    if (IsOpTypeAny(types)) {
      FE_LOGD("Node:%s, Type:%s, Match Op Pattern ANY", name.c_str(), op_type.c_str());
      return true;
    }
    if (IsOutputNode(types)) {
      FE_LOGD("Node:%s, Type:%s, Match Op Pattern OUTNODE", name.c_str(), op_type.c_str());
      return true;
    }
    return false;
  }
}

/*
 * @brief: check whether node output size is same with candidate desc output
 * size
 * @param [in] node: graph node
 * @param [in] op_desc: candidated pattern desc
 * @return bool: check result
 */
bool BufferFusionPassRunner::SkipDiffSizeDesc(ge::NodePtr node, const BufferFusionOpDesc *op_desc,
                                              const string &pattern_name) const {
  FE_CHECK(node == nullptr, REPORT_FE_ERROR("[SubGraphOpt][UbFusion][SkipDiffSizeDesc] node is null."), return false);
  FE_CHECK(op_desc == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][UbFusion][SkipDiffSizeDesc] opDesc is null."), return false);

  // single output node match single desc, and binary node match binary desc
  if (node->GetOutDataNodes().size() == 1 && op_desc->out_branch_type == TBE_OUTPUT_BRANCH_MULTI) {
    FE_LOGD("Node[%s]: the size of out_data_nodes is 1, but the out_brand_type is TBE_OUTPUT_BRANCH_MULTI, skip.",
            node->GetName().c_str());
    return true;
  }

  if (node->GetOutDataNodes().size() > 1 && op_desc->out_branch_type == TBE_OUTPUT_BRANCH_SINGLE) {
    // support common_rules2, outputs from conv and quant, conv is head node
    if (pattern_name == "TbeCommonRules2FusionPass") {
      string op_type;
      if (GetOpAttrType(node, op_type, op_desc->not_pattern) &&
          (op_type == "Convolution" || op_type == "DepthwiseConvolution")) {
        return false;
      }
    }

    FE_LOGD("Skip node[%s]: the size of out data nodes is > 1, but desc %s's out type is TBE_OUTPUT_BRANCH_SINGLE.",
            node->GetName().c_str(), op_desc->desc_name.c_str());
    return true;
  }

  return false;
}

bool BufferFusionPassRunner::SkipDiffShapeTypeDesc(ge::NodePtr node, const BufferFusionOpDesc *op_desc) const {
  if (node == nullptr || op_desc == nullptr) {
    return true;
  }
  bool is_unknown_shape_op = IsFeSupportedDynamicOp(*(node->GetOpDesc()), true);
  if (op_desc->shape_type_rule == ONLY_SUPPORT_STATIC && is_unknown_shape_op) {
    FE_LOGD("Node[%s, %s] whose shape is dynamic shall be skipped for the buffer desc only supports static shape.",
            node->GetName().c_str(), node->GetType().c_str());
    return true;
  }

  if (op_desc->shape_type_rule == ONLY_SUPPORT_DYNAMIC && !is_unknown_shape_op) {
    FE_LOGD("Node[%s, %s] whose shape is static shall be skipped for the buffer desc only supports dynamic shape.",
            node->GetName().c_str(), node->GetType().c_str());
    return true;
  }
  return false;
}

/*
 * @brief: get current loop fusiton match status
 * @param [in] is_parallel: graph node is multi branch or single branch
 * @param [in] op_descs: candidated pattern desc
 * @param [in] usage: record whether desc has beed matched
 * @return bool: all current loop descs have beed matched or not
 */
bool BufferFusionPassRunner::GetCurrMatchStatus(const BufferFusionOpDesc *op_desc, size_t out_node_size,
                                                const std::vector<BufferFusionOpDesc *> &op_descs,
                                                std::map<BufferFusionOpDesc *, bool> &usage) const {
  bool is_parallel = !op_desc->ignore_output_num && out_node_size > 1;
  bool match_status = false;

  // check match status
  if (is_parallel) {
    match_status = true;
    for (const auto &desc : op_descs) {
      if (usage.find(desc) != usage.end()) {
        if (!usage[desc]) {
          match_status = false;
          break;
        }
      }
    }
  } else {
    match_status = false;
    for (const auto &desc : op_descs) {
      if (usage.find(desc) != usage.end()) {
        if (usage[desc]) {
          match_status = true;
          break;
        }
      }
    }
  }

  return match_status;
}

/*
 * @brief: get pattern fusiton match status
 * @param [in] pattern: fusion pattern info
 * @return bool: the pattern has beed matched or not
 */
bool BufferFusionPassRunner::GetPatternMatchStatus(BufferFusionPattern &pattern) const {
  std::map<int64_t, bool> group_status;
  // find same group desc match status
  for (auto desc : pattern.GetOpDescs()) {
    if (desc->types[0] == TBE_PATTERN_INPUT_NODE) {
      continue;
    }
    if (desc->group_id == TBE_PATTERN_GROUPID_INVALID) {
      continue;
    }
    if (group_status.find(desc->group_id) == group_status.end()) {
      group_status[desc->group_id] = false;
    }
    if (desc->repeate_curr >= desc->repeate_min) {
      group_status[desc->group_id] = true;
    }
  }
  // find all pattern descs matched status
  bool status = true;
  for (auto desc : pattern.GetOpDescs()) {
    if (desc->types[0] == TBE_PATTERN_INPUT_NODE) {
      continue;
    }
    if (desc->group_id != TBE_PATTERN_GROUPID_INVALID) {
      if (!group_status[desc->group_id]) {
        FE_LOGD("group[%ld] not match", desc->group_id);
        status = false;
        break;
      }
    } else if (desc->repeate_curr < desc->repeate_min) {
      FE_LOGD("pattern %s not match info: desc name=[%s] curr_match cnt=[%ld], min_match cnt=[%ld]",
          pattern.GetName().c_str(), desc->desc_name.c_str(), desc->repeate_curr, desc->repeate_min);
      status = false;
      break;
    }
  }

  return status;
}

/*
 * @brief: get fusiton pattern head desc matched
 * @param [in] node: graph node
 * @param [in] head_descs: candidated head desc list
 * @return BufferFusionOpDesc*: head desc ptr
 */
BufferFusionOpDesc *BufferFusionPassRunner::GetMatchedHeadDesc(ge::NodePtr node, const string &pattern_name,
                                                               std::vector<BufferFusionOpDesc *> head_descs) const {
  for (auto desc : head_descs) {
    if (SkipDiffSizeDesc(node, desc, pattern_name) || SkipDiffShapeTypeDesc(node, desc)) {
      continue;
    }
    if (IsOpTypeExist(node, desc)) {
      FE_LOGD("Node [%s], desc[%s] from graph has matched to head desc from fusion pattern %s.",
              node->GetName().c_str(), desc->desc_name.c_str(), pattern_name.c_str());
      return desc;
    }
  }
  return nullptr;
}

/*
 * @brief: get current loop desc matched
 * @param [in] node: graph node
 * @param [in] head_descs: valid head desc
 * @param [in] usage: record whether desc has beed matched
 * @return BufferFusionOpDesc*: matched desc ptr
 */
BufferFusionOpDesc *BufferFusionPassRunner::GetMatchedNormalDesc(
    const ge::NodePtr &node, const std::vector<BufferFusionOpDesc *> &descs,
    std::map<BufferFusionOpDesc *, bool> &usage, MatchInfo &m_info) {
  /* If node cannot match any concrete pattern, we use the
   * lower priority patterns such as "OutputData" or "OpTypeAny" */
  BufferFusionOpDesc *lower_prior_desc = nullptr;
  std::string node_name = node->GetName();

  for (auto out_desc : descs) {
    FE_LOGD("Check whether desc %s and node %s are matched. Usage: %d",
            out_desc->desc_name.c_str(), node->GetName().c_str(), usage[out_desc]);
    if (SkipNodeForNormalDesc(out_desc, node, m_info)) {
      continue;
    }

    if (!usage[out_desc] && IsOpTypeExist(node, out_desc)) {
      if (IsOpTypeAny(out_desc->types) || IsOutputNode(out_desc->types)) {
        lower_prior_desc = out_desc;
        continue;
      }
      FE_LOGD("Match node [name:%s, desc:%s]", node_name.c_str(), out_desc->desc_name.c_str());
      return out_desc;
    }
  }

  if (lower_prior_desc != nullptr) {
    FE_LOGD("Match node [name:%s, desc:%s]", node_name.c_str(), lower_prior_desc->desc_name.c_str());
  }
  return lower_prior_desc;
}

bool BufferFusionPassRunner::TempCycleDetection(const ge::NodePtr &out_node, MatchInfo &m_info,
                                                std::pair<ge::NodePtr, MatchInfo> &m_info_pair,
                                                std::vector<std::pair<ge::NodePtr, MatchInfo>> &m_info_stack) {
  ge::NodePtr node_lead_to_cycle;
  if (CheckLoopForward(m_info.m_st.m_rs.mapping, out_node, node_lead_to_cycle)) {
    FE_LOGD("Temp cycle has been detected when matching node %s in pattern %s. Cycle related node[%s]",
            out_node->GetName().c_str(), m_info.pattern->GetName().c_str(), node_lead_to_cycle->GetName().c_str());
    if (m_info.black_list.count(out_node) == 0) {
      /* Do not need to back track to out_node again,
       * because its father node is already in the back-tracking
       * list. */
      for (const auto &node: m_info.m_st.m_rs.nodes_lead_to_cycle) {
        if (node == node_lead_to_cycle) {
          FE_LOGD("%s has already been checked and added into the stack.", node->GetName().c_str());
          return false;
        }
      }

      /* Store current matched info. */
      m_info_pair.first = out_node;
      PrintMapping(m_info_pair.second.m_st.m_rs.mapping, "Temp Cycle Detected");
      m_info_stack.emplace_back(std::move(m_info_pair));
      m_info.m_st.m_rs.nodes_lead_to_cycle.emplace_back(node_lead_to_cycle);
      m_info.m_st.m_rs.cycle_pos.emplace_back(m_info_stack.size() - 1);
      FE_LOGD("Push this mapping into stack. cycle pos %zu.", (m_info_stack.size() - 1));
      return false;
    } else {
      FE_LOGD("Cannot match %s because it's in black list.", out_node->GetName().c_str());
      return true;
    }
  }
  return false;
}

Status SaveMatchResult(bool not_any_and_output_op, BufferFusionOpDesc *out_desc,
                       const ge::NodePtr &out_node, std::map<BufferFusionOpDesc *, bool> &usage_flags,
                       MatchInfo& m_info) {
  auto out_node_name = out_node->GetName();
  if (not_any_and_output_op) {
    m_info.m_st.queue_nodes.push_back(out_node);
    m_info.m_st.queue_descs.push_back(out_desc);
  }

  /* If the desc is TBE_PATTERN_OUTPUT_NODE or TBE_PATTERN_OP_TYPE_ANY,
   * the op_desc should also be inserted into matched output nodes.
   * Because the TBE_PATTERN_OUTPUT_NODE and TBE_PATTERN_OP_TYPE_ANY will
   * always be matched and if previous path is valid. When the father node
   * is multi-output, for example:
   *              convolution
   *                   |
   *                   |
   *              element-wise (normal matched op desc)
   *                  /\
   *                 /  \
   *                /    \
   *      output-node   quant(which maybe optional)
   *
   * First, we match output-node and than in function GetCurrMatchStatus
   * the output-node will be considered as matched. Then
   * we recover the matching queue with op_desc element-wise.
   *
   * Then if the output-node is not recorded in the matched_output_nodes,
   * we will still match the output-node again.
   *
   * Finally, the matching and recovering will be done infinitely.
   *
   * So, here we should put every matched node into matched_output_nodes.
   * */
  auto it = m_info.matched_output_nodes.find(out_desc->desc_name);
  if (it != m_info.matched_output_nodes.end()) {
    (it->second)[out_desc->repeate_curr].push_back(out_node_name);
  } else {
    std::map<int32_t, std::vector<std::string>> temp;
    temp.insert(std::pair<int32_t, std::vector<std::string>>(out_desc->repeate_curr, {out_node_name}));
    m_info.matched_output_nodes.insert(
        std::pair<std::string, std::map<int32_t, std::vector<std::string>>>(out_desc->desc_name, temp));
  }

  // add fusioned node to mapping
  m_info.m_st.m_rs.mapping[out_desc].push_back(out_node);
  // repeat desc need to plus while has been matched
  if (CheckInt64AddOverflow(out_desc->repeate_curr, 1) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][UbFusion][MtcFollowNd] repeateCurr++ overflow. (out_desc:%s)",
                    out_desc->desc_name.c_str());
    return FAILED;
  }
  out_desc->repeate_curr++;
  usage_flags[out_desc] = true;
  return SUCCESS;
}

void BufferFusionPassRunner::MatchFollowingNodes(
    ge::NodePtr node, std::vector<BufferFusionOpDesc *> &out_descs,
    std::map<BufferFusionOpDesc *, bool> &usage_flags, MatchInfo& m_info,
    std::vector<std::pair<ge::NodePtr, MatchInfo>> &m_info_stack) {
  auto out_nodes = node->GetOutDataNodes();
  FE_LOGD("Match successors for node %s, successor size %zu.", node->GetName().c_str(), out_nodes.size());
  for (auto desc : out_descs) {
    usage_flags[desc] = false;
  }

  bool is_head_dynamic_impl = IsOpDynamicImpl(node->GetOpDesc());\
  /* Store the matched info before matching following nodes of node. */
  m_info.UpdateRepTimes();
  MatchInfo m_info_back = m_info;
  std::pair<ge::NodePtr, MatchInfo> m_info_pair = std::make_pair(nullptr, std::move(m_info_back));

  for (auto &out_node : out_nodes) {
    std::string out_node_name = out_node->GetName();
    bool is_out_dynamic_impl = IsOpDynamicImpl(out_node->GetOpDesc());
    if (is_head_dynamic_impl != is_out_dynamic_impl) {
      FE_LOGD("Op impl type is different between node[%s] and node[%s].",
              node->GetName().c_str(), out_node_name.c_str());
      continue;
    }
    BufferFusionOpDesc *out_desc =
        GetMatchedNormalDesc(out_node, out_descs, usage_flags, m_info);
    if (out_desc != nullptr) {
      bool not_any_op = !IsOpTypeAny(out_desc->types);
      bool not_out_op = !IsOutputNode(out_desc->types);
      if (NeedIgnoreOp(out_node) && not_any_op && not_out_op) {
        FE_LOGD("outDesc node [%s] is ignored, out_desc:%s", out_node_name.c_str(), out_desc->desc_name.c_str());
        continue;
      }
      if (not_out_op) {
        /* Op with pattern TBE_PATTERN_OUTPUT_NODE will not be fused,
         * so we do not check cycles related to those output op. */
        if (TempCycleDetection(out_node, m_info, m_info_pair, m_info_stack)) {
          continue;
        }
        m_info.m_st.m_rs.matched_nodes[out_node] = out_desc;
      }

      bool not_any_and_output_op = not_any_op && not_out_op;
      if (SaveMatchResult(not_any_and_output_op, out_desc, out_node,
                          usage_flags, m_info) != SUCCESS) {
        return;
      }

      if (m_info.m_st.queue_descs.front()->out_branch_type != TBE_OUTPUT_BRANCH_MULTI) {
        break;
      }
    } else {
      FE_LOGD("Output node [%s] ARE NOT MATCHED to any desc from fusion pattern.", out_node->GetName().c_str());
    }
  }
}

void BufferFusionPassRunner::GetExistingFusionScopes(ge::ComputeGraph &graph,
                                                     std::map<int64_t, vector<ge::NodePtr>> &fusion_scopes) const {
  for (auto &node : graph.GetDirectNode()) {
    if (ScopeAllocator::HasScopeAttr(node->GetOpDesc())) {
      int64_t scope_id = 0;
      if (!ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id)) {
        continue;
      }
      fusion_scopes[scope_id].push_back(node);
    }
  }
}

bool BufferFusionPassRunner::IsOptionalOutput(const BufferFusionOpDesc *desc) const {
  if (desc->out_branch_type > static_cast<int>(desc->outputs.size())) {
    FE_LOGW("%s outputs size is less than out_branch_type required, consider it as optional output.",
        desc->desc_name.c_str());
    return true;
  } else if (desc->out_branch_type == TBE_OUTPUT_BRANCH_SINGLE && desc->outputs.size() > 1) {
    for (auto out_desc : desc->outputs) {
      if (!IsOpTypeAny(out_desc->types) && !IsOutputNode(out_desc->types) && out_desc->repeate_min > 0) {
        continue;
      } else if (!IsOptionalOutput(out_desc)) {
        continue;
      }
      return true;
    }
    return false;
  } else {
    for (auto out_desc : desc->outputs) {
      if (!IsOpTypeAny(out_desc->types) && !IsOutputNode(out_desc->types) && out_desc->repeate_min > 0) {
        return false;
      } else if (!IsOptionalOutput(out_desc)) {
        return false;
      }
    }
    return true;
  }
}

bool BufferFusionPassRunner::CheckLoopForward(const BufferFusionMapping &mapping,
                                              const ge::NodePtr &target_node,
                                              ge::NodePtr &node_lead_to_cycle) {
  std::vector<ge::NodePtr> all_fuse_nodes;
  for (const auto &it : mapping) {
    if (IsOpTypeAny(it.first->types) || IsOutputNode(it.first->types)) {
      continue;
    }
    for (const auto &node : it.second) {
      all_fuse_nodes.push_back(node);
    }
  }

  for (auto it = mapping.begin(); it != mapping.end(); it++) {
    for (const auto& node : it->second) {
      for (const auto& n : node->GetOutAllNodes()) {
        if (n == target_node) {
          continue;
        }
        if (find(all_fuse_nodes.begin(), all_fuse_nodes.end(), n) != all_fuse_nodes.end()) {
          continue;
        }
        if (cycle_detector_->IsConnected(n, target_node)) {
          node_lead_to_cycle = n;
          FE_LOGD("target node %s is a sub node of %s, a loop will be generated.",
                  target_node->GetName().c_str(), n->GetName().c_str());
          return true;
        }
      }
    }
  }
  return false;
}

bool BufferFusionPassRunner::CompareMappings(const MatchResult &curr_m_rs,
                                             MatchResult &longest_m_rs,
                                             size_t &longest_num) const {
  size_t curr_num = GetMappingSize(curr_m_rs.mapping);
  if (curr_num > longest_num) {
    longest_m_rs = curr_m_rs;
    longest_num = curr_num;
    FE_LOGD("Set current mapping as the longest mapping(size %zu). Mapping element size is %zu.",
            longest_num, longest_m_rs.mapping.size());
    PrintMapping(curr_m_rs.mapping, "Found Longer Mapping");
    return true;
  } else if (curr_num == longest_num) {
    // When mapping size is the same, we compare the nodes size.
    // The mapping contains more distinct nodes will be considered
    // as larger one.
    size_t curr_distinct_num = GetDistinctNodeSize(curr_m_rs.mapping);
    size_t longest_distinct_num = GetDistinctNodeSize(longest_m_rs.mapping);

    FE_LOGD("Current and longest distinct nodes number are %zu and %zu.",
            curr_distinct_num, longest_distinct_num);
    if (curr_distinct_num > longest_distinct_num) {
      longest_m_rs = curr_m_rs;
      longest_num = curr_num;
      FE_LOGD("Set current mapping as the longest mapping(size %zu). Mapping element size is %zu.",
              longest_num, longest_m_rs.mapping.size());
      PrintMapping(curr_m_rs.mapping, "Found Longer Node Size Mapping");
      return true;
    }
  }

  FE_LOGD("Current mapping (size %zu) is <= longest mapping(size %zu). Mapping element size is %zu.",
          curr_num, longest_num, longest_m_rs.mapping.size());
  PrintMapping(curr_m_rs.mapping, "Found Shorter Mapping");
  return false;
}

void BufferFusionPassRunner::RecoverMappingAndQueue(
    MatchInfo &m_info, bool match_error,
    size_t &longest_num) const {
  if (match_error) {
    m_info.m_st.queue_descs.clear();
    m_info.m_st.queue_nodes.clear();
  }

  if (m_info.m_st.queue_descs.empty() && m_info.m_st.queue_nodes.empty()) {
    FE_LOGD("Queue descs is empty, compare matching result.");
    bool longest_updated = false;
    if (GetPatternMatchStatus(*m_info.pattern) && CheckAttrMatch(m_info.m_st.m_rs.mapping)) {
      longest_updated = CompareMappings(m_info.m_st.m_rs, m_info.longest_m_rs, longest_num);
    }
    for (auto desc : m_info.pattern->GetOpDescs()) {
      if (m_info.m_st.m_rs.mapping.find(desc) != m_info.m_st.m_rs.mapping.end()) {
        desc->repeate_curr = 0;
      }
    }
    if (!m_info.saved_m_st.empty()) {
      /* Keep matching from last save queue descs. */
      m_info.m_st = m_info.saved_m_st.back();
      m_info.saved_m_st.pop_back();
      FE_LOGD("Recover to saved descs {%s}. Updated %d.",
              AssmbleDescNames(m_info.m_st.queue_descs).c_str(), longest_updated);
    } else {
      /* Matching ends. */
      m_info.m_st.m_rs = m_info.longest_m_rs;
    }
    for (auto desc : m_info.pattern->GetOpDescs()) {
      if (m_info.m_st.m_rs.mapping.find(desc) != m_info.m_st.m_rs.mapping.end()) {
        desc->repeate_curr = m_info.m_st.m_rs.mapping.find(desc)->second.size();
      }
    }
  }
}

bool BufferFusionPassRunner::SkipNodeForNormalDesc(
    BufferFusionOpDesc *out_desc, ge::NodePtr node, MatchInfo &m_info) const {
  string node_name = node->GetName();

  auto it = m_info.matched_output_nodes.find(out_desc->desc_name);
  if (it != m_info.matched_output_nodes.end()) {
    if (find((it->second)[out_desc->repeate_curr].begin(), (it->second)[out_desc->repeate_curr].end(), node_name) !=
        (it->second)[out_desc->repeate_curr].end()) {
      FE_LOGD("skip matched node %s for opdesc %s.", node_name.c_str(), out_desc->desc_name.c_str());
      return true;
    }
  }
  // check the same size branch firstly, if not, check the diff size branch
  if (!out_desc->ignore_output_num && SkipDiffSizeDesc(node, out_desc, m_info.pattern->GetName())) {
    if (!IsOpTypeAny(out_desc->types) && !IsOutputNode(out_desc->types) && !IsOptionalOutput(out_desc)) {
      FE_LOGD("[node %s, desc %s]'s output number is not matched.", node->GetName().c_str(),
              out_desc->desc_name.c_str());
      return true;
    }
  }
  bool check_status = out_desc != m_info.head_desc && !out_desc->ignore_input_num &&
                      node->GetInDataNodes().size() != out_desc->inputs.size() && !IsOutputNode(out_desc->types);
  if (check_status) {
    FE_LOGD("node size not same with desc, node name=[%s], input cnt=[%zu], desc inputsize=[%zu]",
        node_name.c_str(), node->GetInDataNodes().size(), out_desc->inputs.size());
    return true;
  }

  if (SkipDiffShapeTypeDesc(node, out_desc)) {
    return true;
  }

  return false;
}

bool BufferFusionPassRunner::SkipNodeBeforeMatch(const ge::NodePtr &node, size_t curr_node_num, size_t curr_desc_num,
                                                 BufferFusionOpDesc *op_desc, const BufferFusionOpDesc *head_desc,
                                                 bool get_output_result, const string &pattern_name) const {
  if (!curr_node_num) {
    FE_LOGD("current node %s has no output node. skip it.", node->GetName().c_str());
    return true;
  }

  // One of the conditions for matching the longest structure is that the number of output nodes is no more than 10
  if (curr_node_num > 10) {
    FE_LOGD("output nodes[%zu] of current node %s is greater than 10. skip it.", curr_node_num,
            node->GetName().c_str());
    return true;
  }

  if (!get_output_result) {
    FE_LOGD("fail to get output desc for %s. skip it.", op_desc->desc_name.c_str());
    return true;
  }
  if ((op_desc == head_desc || !op_desc->ignore_output_num) && curr_node_num > 1 &&
      (curr_node_num != curr_desc_num || op_desc->out_branch_type != TBE_OUTPUT_BRANCH_MULTI)) {
    string op_type = "";
    if (pattern_name == "TbeCommonRules2FusionPass") {
      if (GetOpAttrType(node, op_type, op_desc->not_pattern) &&
          (op_type == "Convolution" || op_type == "DepthwiseConvolution")) {
        FE_LOGD("pattern_name [%s], conv or depthwiseconv is head node", pattern_name.c_str());
        return false;
      }
    }

    FE_LOGI("Not match info: out relation [%ld], outnode size [%zu], outdesc size [%zu]. Node %s, desc %s.",
        op_desc->out_branch_type, curr_node_num, curr_desc_num,
        node->GetName().c_str(), op_desc->desc_name.c_str());
    return true;
  }
  return false;
}

void BufferFusionPassRunner::SaveQueueBeforeMatch(std::vector<BufferFusionOpDesc *> &out_descs, ge::NodePtr node,
                                                  const BufferFusionOpDesc *op_desc, MatchInfo &m_info) const {
  BufferFusionOpDesc *first_desc = nullptr;
  if (!out_descs.empty()) {
    first_desc = out_descs.front();
  }

  auto curr_nodes = node->GetOutDataNodes();
  if (first_desc && !first_desc->multi_output_skip_status.empty() &&
      first_desc->repeate_max > first_desc->repeate_curr &&
      first_desc->multi_output_skip_status[first_desc->repeate_curr] == SkipStatus::AVAILABLE &&
      curr_nodes.size() == 1 && curr_nodes.at(0)->GetOutDataNodes().size() > 1) {
    first_desc->multi_output_skip_status[first_desc->repeate_curr] = SkipStatus::SKIPPED;
    FE_LOGD("Try skipping node %s from repeated opdesc %s first.", curr_nodes.at(0)->GetName().c_str(),
            first_desc->desc_name.c_str());
    out_descs.erase(out_descs.cbegin(), out_descs.cbegin() + 1);
    m_info.saved_m_st.emplace_back(m_info.m_st);

    PrintMapping(m_info.m_st.m_rs.mapping, "SaveQueueBeforeMatch");
    FE_LOGD("Save queue for skip case");
  }

  /* When current op_desc's output type is TBE_OUTPUT_BRANCH_MULTI
   * or the op_desc does not care about the output,
   * we need to back track the current node.
   * For example:
   *              A
   *             / \
   *            B   C
   *                 \
   *                  D
   * When matching the pattern above,
   * After matching head A, first we search the left successor B
   * and then we back track from A to C because there may be a longer
   * mapping. So before matching B, we do the following operations
   * to save A into the saved_queue_descs.
   * Then if B is matched, we mark B as visited and recover the queue
   * with A.
   */
  if (!op_desc->ignore_output_num && op_desc->out_branch_type == TBE_OUTPUT_BRANCH_MULTI) {
    m_info.saved_m_st.emplace_back(m_info.m_st);
    m_info.saved_count++;

    FE_LOGD("save queue for multiple output branch.");
  }

  if (op_desc->ignore_output_num && curr_nodes.size() > 1) {
    m_info.saved_m_st.emplace_back(m_info.m_st);
    m_info.saved_count++;

    FE_LOGD("save queue for optional output.");
  }
}

bool GetOutputs(BufferFusionOpDesc *op_desc, std::vector<BufferFusionOpDesc *> &outputs,
                bool ignore_repeat = false) {
  if (op_desc == nullptr) {
    FE_LOGW("[GetOutputs][Check] op_desc is null.");
    return false;
  }

  // add curr desc can be reused while repeat_curr < repeate_max
  if ((!ignore_repeat) && (op_desc->repeate_curr < op_desc->repeate_max)) {
    outputs.push_back(op_desc);
  }

  // check candidate desc
  for (auto desc : op_desc->outputs) {
    if (desc == nullptr) {
      continue;
    }

    if (desc->repeate_curr >= desc->repeate_max) {
      continue;
    }
    // add out desc
    outputs.push_back(desc);

    // add sub out_descs while repeate_min == 0
    if (desc->repeate_min == 0) {
      std::vector<BufferFusionOpDesc *> sub_output;
      if (GetOutputs(desc, sub_output, true)) {
        for (const auto &sub_desc : sub_output) {
          outputs.push_back(sub_desc);
        }
      }
    }
  }

  return true;
}

void BufferFusionPassRunner::BreathFirstMatch(MatchInfo &m_info,
                                              std::vector<std::pair<ge::NodePtr, MatchInfo>> &m_info_stack) {
  size_t longest_num = GetMappingSize(m_info.longest_m_rs.mapping);

  while (!m_info.m_st.queue_descs.empty() && !m_info.m_st.queue_nodes.empty()) {
    ge::NodePtr node = m_info.m_st.queue_nodes.front();
    BufferFusionOpDesc *op_desc = m_info.m_st.queue_descs.front();
    auto out_nodes = node->GetOutDataNodes();
    std::vector<BufferFusionOpDesc *> out_descs;
    bool res = GetOutputs(op_desc, out_descs);

    FE_LOGD("Out descs for %s are: {%s}.", op_desc->desc_name.c_str(),
            AssmbleDescNames(out_descs).c_str());

    if (SkipNodeBeforeMatch(node, out_nodes.size(), out_descs.size(), op_desc,
                            m_info.head_desc, res, m_info.pattern->GetName())) {
      RecoverMappingAndQueue(m_info, true, longest_num);
      continue;
    }
    if (out_descs.empty() && m_info.m_st.queue_descs.size() > 1 && m_info.m_st.queue_nodes.size() > 1) {
      m_info.m_st.queue_nodes.erase(m_info.m_st.queue_nodes.cbegin());
      m_info.m_st.queue_descs.erase(m_info.m_st.queue_descs.cbegin());
      continue;
    }

    m_info.saved_count = 0;
    SaveQueueBeforeMatch(out_descs, node, op_desc, m_info);
    std::map<BufferFusionOpDesc *, bool> usage_flags;
    // match head node's following nodes
    MatchFollowingNodes(node, out_descs, usage_flags, m_info, m_info_stack);
    // check whether match is ok
    bool match_status =
        GetCurrMatchStatus(op_desc, out_nodes.size(), out_descs, usage_flags);
    FE_LOGD("matched status for output of [desc %s, node %s] is %d.", op_desc->desc_name.c_str(),
            node->GetName().c_str(), match_status);
    if (match_status) {
      m_info.m_st.queue_nodes.erase(m_info.m_st.queue_nodes.cbegin());
      m_info.m_st.queue_descs.erase(m_info.m_st.queue_descs.cbegin());
      RecoverMappingAndQueue(m_info, false, longest_num);
    } else {
      /* If none of the output nodes is matched, we just pop
       * the last saved queue because the queue is meaningless. */
      for (uint32_t i = 0; i < m_info.saved_count; i++) {
        auto save_op_desc = m_info.saved_m_st.back().queue_descs;
        m_info.saved_m_st.pop_back();
        FE_LOGD("remove last queue {%s} for failed match.",
                AssmbleDescNames(save_op_desc).c_str());
      }

      RecoverMappingAndQueue(m_info, true, longest_num);
    }
  }
  FE_LOGD("After breath first matching, longest mapping is :");
  PrintMapping(m_info.longest_m_rs.mapping, "After One Time Matching");
}

bool BufferFusionPassRunner::FinalCycleDetection(const MatchResult &longest_match_result) const {
  std::vector<std::vector<ge::NodePtr>> all_fusion_nodes;
  std::vector<ge::NodePtr> fusion_nodes;
  Status ret = buffer_fusion_pass_base_ptr_->GetFusionNodes(longest_match_result.mapping, fusion_nodes);
  if (ret != SUCCESS || fusion_nodes.empty()) {
    return true;
  }

  ge::ComputeGraphPtr graph;
  graph = fusion_nodes[0]->GetOwnerComputeGraph();
  if (graph == nullptr) {
    FE_LOGW("[BufferFusion][Match]Owner graph of node %s is nullptr.",
            fusion_nodes[0]->GetName().c_str());
    return false;
  }
  all_fusion_nodes.emplace_back(fusion_nodes);
  return cycle_detector_->CycleDetection(*graph, all_fusion_nodes);
}

void BufferFusionPassRunner::CheckMatchingResult(BufferFusionMapping &longest_mapping,
                                                 MatchInfo &m_info, bool &is_matched) const {
  // check pattern status
  bool pattern_status = GetPatternMatchStatus(*m_info.pattern);
  if (!pattern_status) {
    FE_LOGD("Skip head node %s because some op_desc are not matched.", m_info.head_node->GetName().c_str());
    return;
  }
  if (!CheckAttrMatch(m_info.longest_m_rs.mapping)) {
    return;
  }

  longest_mapping = m_info.longest_m_rs.mapping;
  is_matched = true;
}

Status BufferFusionPassRunner::MatchFusionPattern(MatchInfo &m_info, BufferFusionMapping &longest_mapping) {
  /* m_info_stack is for real cycle detection. Because currently
   * we match the pattern from top to bottom, if one node is
   * marked as a key node() then all successors will not be pushed
   * into this stack. So the stack only contains at most one
   * element. */
  std::vector<std::pair<ge::NodePtr, MatchInfo>> m_info_stack;
  size_t recursive_times = 0;
  bool is_matched = false;
  do {
    /* The first time matching we just use the matched info from parameter. */
    if (recursive_times != 0) {
      uint32_t last_bt_pos = m_info.longest_m_rs.cycle_pos.back();
      auto m_info_pair = m_info_stack.at(last_bt_pos);
      auto cycle_related_node = m_info_pair.first;
      m_info = m_info_pair.second;
      /* Each time there is only one key_node, because in a connected
       * fusion graph, if one node is marked as cycle-related node, all
       * its successors will be ignore. And out matching strategy make sure
       * that we match from head node and each node is fusion graph is connected
       * to head node. */
      FE_LOGD("Try to recover to cycle-related node %s.", cycle_related_node->GetName().c_str());
      m_info.black_list.emplace(cycle_related_node);
      if (m_info.RestoreRepTimes() != SUCCESS) {
        continue;
      }
    }

    recursive_times++;
    BreathFirstMatch(m_info, m_info_stack);
    FE_LOGD("After breath first match, m_info_stack is %zu.", m_info_stack.size());
    if (!m_info.longest_m_rs.cycle_pos.empty() && FinalCycleDetection(m_info.longest_m_rs)) {
      /* When cycle is found, we need to back track to the
       * state that some one of fusion nodes is not matched.
       * Because in function MatchFollowingNodes, we have marked
       * nodes which may lead to cycle, we just revert the matching
       * result to */
      FE_LOGD("An actual cycle or error is found, need to track back to last node. Temp cycle vec: %s.",
              StringUtils::IntegerVecToString(m_info.longest_m_rs.cycle_pos).c_str());
      continue;
    } else {
      /* No cycle is found and we got the longgest match. */
      FE_LOGD("No final cycle is found and we have got the longer match for pattern %s. Temp cycle vec: %s.",
              m_info.pattern->GetName().c_str(),
              StringUtils::IntegerVecToString(m_info.longest_m_rs.cycle_pos).c_str());

      CheckMatchingResult(longest_mapping, m_info, is_matched);
      break;
    }
  } while (!m_info.longest_m_rs.cycle_pos.empty() && recursive_times < kMaxRecursiveTimes);

  if (!is_matched) {
    FE_LOGD("Cannot matched pattern %s with head %s.",
            m_info.pattern->GetName().c_str(), m_info.head_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status BufferFusionPassRunner::MatchFromHead(const ge::NodePtr &head_node, BufferFusionPattern &pattern,
                                             BufferFusionMapping &mapping) {
  // get matched head desc
  BufferFusionOpDesc *head_desc = GetMatchedHeadDesc(head_node, pattern.GetName(), pattern.GetHead());
  MatchInfo m_info(head_node, head_desc, mapping, &pattern);

  if (head_desc != nullptr) {
    m_info.m_st.m_rs.mapping[head_desc].push_back(head_node);
    m_info.m_st.m_rs.matched_nodes[head_node] = head_desc;
    head_desc->repeate_curr++;
    m_info.m_st.queue_nodes.push_back(head_node);
    m_info.m_st.queue_descs.push_back(head_desc);
    m_info.longest_m_rs = m_info.m_st.m_rs;
  } else {
    FE_LOGD("Node [%s] from graph has not been matched to any head desc from fusion pattern %s.",
            head_node->GetName().c_str(), pattern.GetName().c_str());
    return FAILED;
  }

  // match fusion pattern from head node
  return MatchFusionPattern(m_info, mapping);
}

static bool ComparePriority(const ge::NodePtr &left_node, const ge::NodePtr &right_node) {
  // nullptr has been checked in TopologicalSortingForFusionNodes()
  auto left_desc = left_node->GetOpDesc();
  auto right_desc = right_node->GetOpDesc();
  return left_desc->GetId() < right_desc->GetId();
}

static Status TopologicalSortingForFusionNodes(vector<ge::NodePtr> &fusion_nodes,
                                               vector<ge::NodePtr> &sorted_fusion_nodes) {
  for (auto &fusion_node : fusion_nodes) {
    // check opDesc to ensure that there exists no nullptr when sorting
    FE_CHECK_NOTNULL(fusion_node);
    FE_CHECK_NOTNULL(fusion_node->GetOpDesc());
    sorted_fusion_nodes.push_back(fusion_node);
  }
  std::sort(sorted_fusion_nodes.begin(), sorted_fusion_nodes.end(), ComparePriority);
  return SUCCESS;
}

static void SetOpSliceInfoForFusionOp(vector<ge::NodePtr> &fusion_nodes, OpCalcInfo &op_calc_info) {
  string op_calc_info_str;
  SetFusionOpSliceInfoToJson(op_calc_info, op_calc_info_str);
  for (auto &fusion_node : fusion_nodes) {
    (void)ge::AttrUtils::SetStr(fusion_node->GetOpDesc(), FUSION_OP_SLICE_INFO, op_calc_info_str);
  }
}

static bool UpdateOpSliceInfoForSpecificOp(const UbPassSliceInfoManagerPtr &ub_slice_info_manager_ptr,
                                           ge::NodePtr &fusion_node,
                                           const std::string &op_pattern) {
  if (ub_slice_info_manager_ptr->CheckOpPatternSliceInfoUpdate(op_pattern)) {
    // refine op_slice_info for some specified op_pattern
    auto iter = op_pattern_to_matched_map.find(op_pattern);
    UbMatchedType matched_pattern =
        ((iter == op_pattern_to_matched_map.end()) ? UbMatchedType::MATCHED_RESERVED : iter->second);
    size_t tmp_val = 0;
    auto slice_info_base_ptr =
         ub_slice_info_manager_ptr->SwitchSliceInfoPtrByPattern(matched_pattern, fusion_node, tmp_val);
    if (slice_info_base_ptr == nullptr || slice_info_base_ptr->ModifySliceInfoByPattern(fusion_node) != SUCCESS) {
      return false;
    }
  }
  return true;
}

static bool Stratege1(BufferFusionPassBasePtr &buffer_fusion_pass_base_ptr,
                      vector<ge::NodePtr> &sorted_fusion_nodes, OpCalcInfo &op_calc_info) {
  if (buffer_fusion_pass_base_ptr != nullptr  &&
      buffer_fusion_pass_base_ptr->CalcFusionOpSliceInfo(sorted_fusion_nodes, op_calc_info) != SUCCESS) {
    vector<AxisSplitMap> empty_map;
    op_calc_info.SetAxisSplitMaps(empty_map);
  }
  // set sliceinfo for fusionOp
  if (op_calc_info.GetAxisSplitMaps().size() != 0) {
    SetOpSliceInfoForFusionOp(sorted_fusion_nodes, op_calc_info);
    FE_LOGD("Succeed to calculate op_slice info by Stratege1.");
    return true;
  } else {
    FE_LOGD("Failed to calculate op_slice info by Stratege1.");
    return false;
  }
}

static bool Stratege2(const UbPassSliceInfoManagerPtr &ub_slice_info_manager_ptr,
                      vector<ge::NodePtr> &sorted_fusion_nodes,
                      OpCalcInfo &op_calc_info) {
  for (auto &fusion_node : sorted_fusion_nodes) {
    if (fusion_node == nullptr) {
      return false;
    }
    std::string op_pattern;
    (void)ge::AttrUtils::GetStr(fusion_node->GetOpDesc(), fusion_node->GetName() + "_pattern", op_pattern);
    bool condition = (ub_slice_info_manager_ptr->CheckOpPatternSupport(op_pattern) &&
                      UpdateOpSliceInfoForSpecificOp(ub_slice_info_manager_ptr, fusion_node, op_pattern));
    if (!condition) {
      FE_LOGD("Not support op_pattern [%s], Stratege2 run failed.", op_pattern.c_str());
      return false;
    }
  }
  if (ub_slice_info_manager_ptr->CalcSliceInfoForFusionOp(sorted_fusion_nodes, op_calc_info) != SUCCESS) {
    vector<AxisSplitMap> empty_map;
    op_calc_info.SetAxisSplitMaps(empty_map);
  }
  // set sliceinfo for fusionOp
  if (op_calc_info.GetAxisSplitMaps().size() != 0) {
    SetOpSliceInfoForFusionOp(sorted_fusion_nodes, op_calc_info);
    FE_LOGD("Succeed to calculate op_slice info by Stratege2.");
    return true;
  } else {
    FE_LOGD("Failed to calculate op_slice info by Stratege2.");
    return false;
  }
}

static void Stratege3(const UbPassSliceInfoManagerPtr &ub_slice_info_manager_ptr, vector<ge::NodePtr> &fusion_nodes) {
  ub_slice_info_manager_ptr->SetSliceInfoForFusionNodes(fusion_nodes);
}

void BufferFusionPassRunner::CalcSliceInfoForFusionOp(vector<ge::NodePtr> &fusion_nodes) {
  auto first_node = fusion_nodes.at(0);
  if (first_node == nullptr) {
    return;
  }

  string slice_info_str;
  (void)ge::AttrUtils::GetStr(first_node->GetOpDesc(), FUSION_OP_SLICE_INFO, slice_info_str);
  if (!slice_info_str.empty()) {
    FE_LOGD("FusionOp's slice info has been set, no need to calculate again.");
    return;
  }

  OpCalcInfo op_calc_info;
  if (!op_calc_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UB][CalcSliceInfo] op_calc_info initialize failed");
    return;
  }

  bool enable_stratege1 = false;
  bool enable_stratege2 = false;
  bool enable_stratege3 = true;
  // adopt TopologicalSorting first to ensure that input/output indexes for fusionOp can be calculated correctlly
  vector<ge::NodePtr> sorted_fusion_nodes;
  enable_stratege1 = (TopologicalSortingForFusionNodes(fusion_nodes, sorted_fusion_nodes) == SUCCESS);
  // CalcFusionOpSliceInfo() may be implemented in fusion passes to calculate slice info,
  // mainlly for passes defined in cann
  if (enable_stratege1) {
    enable_stratege2 = (!Stratege1(buffer_fusion_pass_base_ptr_, sorted_fusion_nodes, op_calc_info));
  }
  // if stratege1 runs failed, adopt default fusionOpSliceInfo calculation process, mainlly for passes defined in fe
  FE_MAKE_SHARED(ub_slice_info_manager_ptr_ = std::make_shared<UbPassSliceInfoManager>(), return);
  if (enable_stratege2) {
    enable_stratege3 = (!Stratege2(ub_slice_info_manager_ptr_, sorted_fusion_nodes, op_calc_info));
  }
  // if above strateges run failed, adopt default fusionOpSliceInfo calculation process,
  // only for passes containing conv2d node
  if (enable_stratege3) {
    Stratege3(ub_slice_info_manager_ptr_, fusion_nodes);
  }
}

/*
 * @brief: match one pattern, and do fusion for the matched node
 * @param [in] graph: graph node
 * @param [in] pattern: fusion pattern info
 * @param [in] mappings: fusion group node set
 * @return bool: match current pattern ok or not
 */
bool BufferFusionPassRunner::RunOnePattern(ge::ComputeGraph &graph, BufferFusionPattern &pattern) {
  int matched_times = 0;
  string pass_name = GetName();
  string pattern_name = pattern.GetName();
  BufferFusionMapping mapping;
  // 1. compare 1st pattern op and graph op(include compare op type and TBE type
  for (const ge::NodePtr &node_g : graph.GetDirectNode()) {
    // filter non TBE op
    if (NeedIgnoreOp(node_g)) {
      continue;
    }
    mapping.clear();

    // initial all descs repeat curr cnt
    InitRepeatCurr(pattern.GetOpDescs());

    if (MatchFromHead(node_g, pattern, mapping) != SUCCESS) {
      continue;
    }

    vector<ge::NodePtr> fusion_nodes;
    Status status = buffer_fusion_pass_base_ptr_->GetFusionNodes(mapping, fusion_nodes);
    if (status != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][UB][RunOnePtn] Pass[%s]Pattern[%s]: Failed to get fusion nodes because %u.",
                      pass_name.c_str(), pattern_name.c_str(), status);
      return false;
    }

    if (fusion_nodes.empty()) {
      continue;
    }
    auto first_node = fusion_nodes.at(0);
    if (first_node == nullptr) {
      continue;
    }

    CalcSliceInfoForFusionOp(fusion_nodes);
    FE_LOGD("Pass [%s] Pattern[%s]: CalcSliceInfoForFusionOp end.", pass_name.c_str(), pattern_name.c_str());

    // if nodes have cube and vector core type, do not need to fuse.
    ISAArchVersion isa_arch_version = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
    bool v210_fixpipe_pass = pass_name.find("FixpipeFusionPass") != string::npos;
    bool skip_fusion = (isa_arch_version != ISAArchVersion::EN_ISA_ARCH_V300 &&
                        CheckCubeVectorSplit(fusion_nodes) && !v210_fixpipe_pass) ||
                       (isa_arch_version == ISAArchVersion::EN_ISA_ARCH_V300 && v210_fixpipe_pass);
    if (skip_fusion) {
      FE_LOGD("UbFusionPass[%s]: pattern=%s, headnode=%s. With cube and vector core type, do not need to fuse",
              GetName().c_str(), pattern_name.c_str(), node_g->GetName().c_str());
      continue;
    }
    // set scope_id
    SetScopeIdAndPassName(fusion_nodes, pass_name, pattern_name);
    FE_LOGD("UbFusionPass[%s]: pattern=%s, headnode=%s.", GetName().c_str(), pattern_name.c_str(),
        node_g->GetName().c_str());
    for (auto &item : fusion_nodes) {
      FE_LOGD("node:%s.", item->GetName().c_str());
    }
    cycle_detector_->UpdateConnectionMatrix(graph, fusion_nodes);
    matched_times++;
  }

  FE_LOGD("UbFusionPass[%s]: pattern=%s, matched_times=%d", GetName().c_str(), pattern_name.c_str(), matched_times);
  return true;
}

void BufferFusionPassRunner::SetScopeIdAndPassName(const vector<ge::NodePtr> &fusion_nodes, const string &pass_name,
                                                   const string &pattern_name) const {
  FE_LOGD("Fusion nodes' size: %zu.", fusion_nodes.size());
  if (fusion_nodes.size() < 2) {
    return;
  }

  int64_t scope_id = ScopeAllocator::Instance().AllocateScopeId();
  FE_LOGD("UBPass[pass_name=%s, pattern_name=%s]: set scope_id[%ld] for fusion_nodes.", pass_name.c_str(),
          pattern_name.c_str(), scope_id);
  for (const ge::NodePtr &node : fusion_nodes) {
    if (node == nullptr) {
      continue;
    }
    string name = node->GetName();
    if (ScopeAllocator::SetScopeAttr(node->GetOpDesc(), scope_id)) {
      FE_LOGD("Node[%s]: set scope_id[%ld] success.", name.c_str(), scope_id);
    }
    if (ge::AttrUtils::SetStr(node->GetOpDesc(), PASS_NAME_ATTR, pass_name)) {
      FE_LOGD("Node[%s]: set pass_name[%s] success.", name.c_str(), pass_name.c_str());
    }
  }
}

bool BufferFusionPassRunner::CheckAttrMatch(BufferFusionMapping &mapping) const {
  // node attr _stream_label must be equal
  auto fusion_nodes = buffer_fusion_pass_base_ptr_->GetMatchedNodes(mapping);
  string stream_label;
  for (const auto& n : fusion_nodes) {
    string stream_label_tmp;
    if (!ge::AttrUtils::GetStr(n->GetOpDesc(), STREAM_LABEL, stream_label_tmp)) {
      stream_label_tmp = "null";
    } else {
      FE_LOGD("Fusion nodes have _stream_label %s.", stream_label_tmp.c_str());
    }

    if (stream_label.empty()) {
      stream_label = stream_label_tmp;
    } else if (!stream_label.empty() && stream_label != stream_label_tmp) {
      FE_LOGD("_stream_label not equal, pattern matching failed.");
      return false;
    }
  }
  return true;
}

/*
 * @brief: init all pattern desc repeate_curr to 0
 * @param [in] pattern: fusion pattern desc
 * @return void */
void BufferFusionPassRunner::InitRepeatCurr(const std::vector<BufferFusionOpDesc *> &ops) const {
  for (auto desc : ops) {
    desc->repeate_curr = 0;
    if (!desc->multi_output_skip_status.empty() &&
        desc->multi_output_skip_status[desc->repeate_min] != SkipStatus::DISABLED) {
      for (int64_t i = desc->repeate_min; i < desc->repeate_max; i++) {
        desc->multi_output_skip_status[i] = SkipStatus::AVAILABLE;
      }
    }
  }
}

bool BufferFusionPassRunner::CheckCubeVectorSplit(vector<ge::NodePtr> &fusion_nodes) {
  bool find_cube_op = false;
  bool find_vector_op = false;
  if (GetPlatformSCubeVecSplitFlag()) {
    // cube + vector fusion is enable in MixL2 mode
    bool mixl2_flag = fe::Configuration::Instance(fe::AI_CORE_NAME).GetMixL2Enable();
    for (auto &node : fusion_nodes) {
      bool ffts_node = false;
      (void)ge::AttrUtils::GetBool(node->GetOpDesc(), kTypeFFTSPlus, ffts_node);
      if (!ffts_node && mixl2_flag) {
        return false;
      }
      std::set<std::string>::const_iterator iter = cube_op_type_.find(node->GetType());
      if (iter != cube_op_type_.end()) {
        find_cube_op = true;
      } else {
        find_vector_op = true;
      }
    }

    if (find_cube_op && find_vector_op) {
      return true;
    }
  }
  return false;
}
}  // namespace fe
