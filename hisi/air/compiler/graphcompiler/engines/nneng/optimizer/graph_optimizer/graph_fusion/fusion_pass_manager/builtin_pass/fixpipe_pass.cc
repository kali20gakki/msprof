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

#include "fixpipe_pass.h"
#include <memory>
#include <sstream>
#include "common/aicore_util_constants.h"
#include "common/configuration.h"
#include "common/op_info_common.h"
#include "common/string_utils.h"
#include "common/util/platform_info.h"
#include "fixpipe_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

namespace fe {
const string FIXPIPE_PASS_NAME = "FIXPIPEFUSIONPASS";
constexpr uint32_t FIXPIPE_INPUT_2_INDEX = 2;
constexpr uint32_t FIXPIPE_INPUT_3_INDEX = 3;
constexpr uint32_t FIXPIPE_INPUT_4_INDEX = 4;
constexpr uint32_t FIXPIPE_INPUT_5_INDEX = 5;
constexpr uint32_t FIXPIPE_INPUT_6_INDEX = 6;
constexpr uint32_t FIXPIPE_INPUT_7_INDEX = 7;
constexpr uint32_t FIXPIPE_INPUT_8_INDEX = 8;
constexpr uint32_t FIXPIPE_INPUT_9_INDEX = 9;
constexpr uint32_t ELTWISE_SUB_TYPE = 2;
constexpr uint32_t ELTWISE_ADD_TYPE = 1;
constexpr uint32_t INSERTWOINDEX = 2;
constexpr uint32_t NODECANTACCES = 2;
constexpr uint32_t ELTWISEOPSUBSTRINDEX = 3;

bool FixPipePass::ReadConfig() {
  std::string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  FE_LOGD("ReadConfig socversion = %s", soc_version.c_str());
  return ReadConfig(soc_version);
}

bool FixPipePass::ReadConfig(const std::string &soc_version) {
  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos) !=
      SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return false;
  }
  std::vector<CONFIGDTYPE> cubeconfigtype;
  std::map<std::string, std::vector<CONFIGDTYPE>> cubmap;
  for (auto &iter : CUBE_OP_TYPE_VEC) {
    cubmap.emplace(make_pair(iter, cubeconfigtype));
  }
  FixPipeUnit cube_ops(CUBEUNIT, cubmap);
  m_idxtonodetypes_.push_back(cube_ops);
  unitmapindex_.emplace(make_pair(CUBEUNIT, 0));
  uint32_t index = 1;
  std::map<std::string, std::map<std::string, std::vector<CONFIGDTYPE>>> fixpipe_map_;
  std::vector<std::string> unit_list;
  std::map<std::string, std::vector<std::string>> depends_units;
  ReadPlatFormConfig(soc_version, unit_list, depends_units, fixpipe_map_);
  if (unit_list.empty()) {
    return false;
  }
  for (auto &iter : unit_list) {
    FixPipeUnit unitops(iter, fixpipe_map_[iter]);
    unitmapindex_.emplace(make_pair(iter, index));
    index++;
    FE_LOGD("unit name = %s", iter.c_str());
    for (auto &depends : depends_units[iter]) {
      FE_LOGD("depends = %s", depends.c_str());
    }
    unitops.SetDependUnits(depends_units[iter]);
    m_idxtonodetypes_.push_back(unitops);
  }
  for (auto &unit : m_idxtonodetypes_) {
    const std::vector<std::string> depend_unitsname = unit.GetDependsUnits();
    if (depend_unitsname.empty()) {
      continue;
    }
    std::vector<uint32_t> depend_unitindex;
    for (auto &unitname : depend_unitsname) {
      FE_LOGD("match unit name = %s index = [%u] GetName() = %s ", unit.GetName().c_str(), unitmapindex_[unitname],
              unitname.c_str());
      depend_unitindex.push_back(unitmapindex_[unitname]);
    }
    unit.SetDependUnitsIndex(depend_unitindex);
  }
  return true;
}

std::vector<FusionPattern *> FixPipePass::DefinePatterns() {
  std::vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern("fixpipe");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][pattern][DefPtn] new an object failed."), return patterns);
  pattern->AddOpDesc(PATTERN_CONV, CUBE_OP_TYPE_VEC).SetOutput(PATTERN_CONV);
  patterns.push_back(pattern);
  return patterns;
}

bool FixPipePass::IsCubeOpType(const ge::NodePtr &node_ptr) const {
  auto iter = std::find(CUBE_OP_TYPE_VEC.begin(), CUBE_OP_TYPE_VEC.end(), node_ptr->GetType());
  if (iter == CUBE_OP_TYPE_VEC.end()) {
    FE_LOGD("IsCubeOpType is't node name = %s type = %s", node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
    return false;
  }
  FE_LOGD("IsCubeOpType is node name = %s type = %s", node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
  return true;
}

bool FixPipePass::IsConfictWithSkipConfig(const FixPipePassInfo &cur_pass, const uint32_t &ret_index) const {
  std::vector<uint32_t> cur_index;
  for (auto &index : cur_pass.m_opnodes) {
    FE_LOGD("cur_index = %u", index.GetBelongUnitIndex());
    cur_index.push_back(index.GetBelongUnitIndex());
  }
  return IsConfictWithSkipConfig(cur_index, ret_index);
}

bool FixPipePass::IsConfictWithSkipConfig(const std::vector<uint32_t> &index, const uint32_t &ret_index) const {
  std::vector<uint32_t> depend_indexs = m_idxtonodetypes_[ret_index].GetDependsUnitsIndex();
  FE_LOGD("ret_index = %u", ret_index);
  for (auto &depend_index : depend_indexs) {
    if (std::find(index.begin(), index.end(), depend_index) == index.end()) {
      FE_LOGD("depend_index isn't = %u", depend_index);
      return false;
    }
  }
  return true;
}

bool FixPipePass::IsConfictWithSkipConfig(std::stack<uint32_t> index, const uint32_t &ret_index) const {
  std::vector<uint32_t> cur_index;
  while (!index.empty()) {
    auto node = index.top();
    FE_LOGD("cur_index =  = %d", node);
    cur_index.push_back(static_cast<uint32_t>(node));
    index.pop();
  }
  return IsConfictWithSkipConfig(cur_index, ret_index);
}

bool FixPipePass::JudgeCachePass(const FixPipeNodeInfo &node, stack<uint32_t> &index, uint32_t &ret_index) const {
  uint32_t cur_index;
  FE_LOGD("JudgeCachePass start node name = %s type = %s", node.GetNode()->GetName().c_str(),
          node.GetNode()->GetType().c_str());
  if (index.empty()) {
    cur_index = 0;
  } else {
    cur_index = index.top();
  }
  FE_LOGD("udgeCachePass cur_index= %d", cur_index);
  bool find_flag = false;
  if (index.empty() && node.GetIsHeadNode() && IsCubeOpType(node.GetNode())) {
    ret_index = 0;
    FE_LOGD("JudgeCachePass is headcubenode node = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return true;
  }
  for (size_t i = cur_index + 1; i < m_idxtonodetypes_.size(); i++) {
    find_flag = GetNodeIndex(node, static_cast<uint32_t>(i));
    if (find_flag) {
      ret_index = i;
      break;
    }
  }
  if (!find_flag) {
    return false;
  }
  FE_LOGD("JudgeCachePass node can fixpipe name = %s type = %s", node.GetNode()->GetName().c_str(),
          node.GetNode()->GetType().c_str());
  return IsConfictWithSkipConfig(index, ret_index);
}

bool FixPipePass::GetNodeIndex(const FixPipeNodeInfo &node, const uint32_t &index) const {
  for (auto &nodetype : m_idxtonodetypes_[index].GetNode()) {
    std::string node_type;
    if (node.GetNode()->GetType() == ELTWISE) {
      node_type = GetEltWiseType(node);
    } else {
      node_type = node.GetNode()->GetType();
    }
    if (nodetype.first == node_type) {
      FE_LOGD("PreCachePass node is name = %s type = %s index= %u", node.GetNode()->GetName().c_str(),
              node.GetNode()->GetType().c_str(), index);
      return true;
    }
  }
  return false;
}

bool FixPipePass::PreCachePass(const FixPipePassInfo &cur_pass, const FixPipeNodeInfo &node) const {
  if (FiltrNodeStrategy(node) != SUCCESS) {
    return false;
  }
  if (!CheckIsInVector(cur_pass.m_opnodes)) {
    return false;
  }
  Status ret = FiltrNodeStrategyForQuant(node, cur_pass.m_opnodes[cur_pass.m_opnodes.size() - 1]);
  if (ret != SUCCESS) {
    FE_LOGD("PreCachePass post relu+quant node can't be fixpipe name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return false;
  }
  bool find_flag = false;
  uint32_t ret_index = 0;
  if (!cur_pass.m_opnodes.empty()) {
    for (uint32_t i = cur_pass.unit_index + 1; i < static_cast<uint32_t>(m_idxtonodetypes_.size()); i++) {
      find_flag = GetNodeIndex(node, i);
      if (find_flag) {
        ret_index = i;
        break;
      }
    }
  }
  if (!find_flag) {
    return false;
  }
  return IsConfictWithSkipConfig(cur_pass, ret_index);
}

bool FixPipePass::PreMatchAcorrdingToPass(const FixPipePassInfo &cur_pass, const FixPipeNodeInfo &node) const {
  if (!IsInWhitelist(node)) {
    FE_LOGD("node isn't IsInWhitelist name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return false;
  }
  if (!node.GetIsHeadNode() && IsCubeOpType(node.GetNode())) {
    FE_LOGD("node isnt headcube name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return false;
  }
  if (!PreCachePass(cur_pass, node)) {
    FE_LOGD("node isn't PreCachePass name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return false;
  }
  FE_LOGD("node isn' name = %s type = %s", node.GetNode()->GetName().c_str(), node.GetNode()->GetType().c_str());
  return true;
}

bool FixPipePass::NeedToCutPass(FixPipePassInfo &m_pass) const {
  if (m_pass.m_opnodes.size() == 1) {
    FE_LOGD("only has headnode");
    return true;
  }
  FixPipeNodeInfo node = m_pass.m_opnodes[m_pass.m_opnodes.size() - 1];
  if (node.GetNode()->GetOutDataNodes().empty()) {
    FE_LOGD("GetOutDataNodes.empty");
    m_pass.m_flag = 2;  // m_flag = 2 DONT NEED, 1NEED, 0 UNKOWN
    return false;
  }
  if (node.GetNode()->GetOutDataNodesSize() > 1) {
    FE_LOGD("GetOutDataNodes.> 1");
    m_pass.m_flag = 2;  // m_flag = 2 DONT NEED, 1NEED, 0 UNKOWN
    return false;
  }
  ge::Node::Vistor<ge::NodePtr> out_nodes = node.GetNode()->GetOutDataNodes();
  for (auto &out_node : out_nodes) {
    FixPipeNodeInfo grandnode(out_node);
    if (!PreMatchAcorrdingToPass(m_pass, grandnode)) {
      FE_LOGD("has a can't fixpipe outputnode name = %s type = %s", grandnode.GetNode()->GetName().c_str(),
              grandnode.GetNode()->GetType().c_str());
      m_pass.m_flag = 2;  // m_flag = 2 DONT NEED, 1NEED, 0 UNKOWN
      return false;
    }
  }
  FE_LOGD(" needto cut, passid = %d", m_pass.pass_index);
  m_pass.m_flag = 1;  // m_flag = 2 DONT NEED, 1NEED, 0 UNKOWN
  return true;
}

std::string FixPipePass::GetEltWiseType(const FixPipeNodeInfo &node) const {
  uint32_t real_eltwisetype = 0;
  if (ge::AttrUtils::GetInt(node.GetNode()->GetOpDesc(), ATTR_ELTWISEMODE, real_eltwisetype)) {
    if (real_eltwisetype == ELTWISE_ADD_TYPE) {
      return ADD;
    }
    if (real_eltwisetype == ELTWISE_SUB_TYPE) {
      return SUB;
    }
  }
  return ELTWISE;
}

bool FixPipePass::IsInWhitelist(const FixPipeNodeInfo &node) const {
  for (auto &unit : m_idxtonodetypes_) {
    for (auto &nodes : unit.GetNode()) {
      std::string node_type;
      if (node.GetNode()->GetType() == ELTWISE) {
        node_type = GetEltWiseType(node);
      } else {
        node_type = node.GetNode()->GetType();
      }
      if (nodes.first == node_type) {
        FE_LOGD("node isinwhitelist name = %s type = %s", node.GetNode()->GetName().c_str(), node_type.c_str());
        return true;
      }
    }
  }
  return false;
}

// extrafilterforhardware
Status FixPipePass::FiltrNodeStrategy(const FixPipeNodeInfo &node) const {
  if (node.GetNode()->GetType() == TRANSDATA) {
    return FiltrNodeStrategyForTransData(node);
  }
  if (node.GetNode()->GetType() == ADD || node.GetNode()->GetType() == SUB || node.GetNode()->GetType() == ELTWISE) {
    return FiltrNodeStrategyForEltWise(node);
  }
  if (node.GetNode()->GetType() == PRELU || node.GetNode()->GetType() == LEAKYRELU) {
    return FiltrNodeStrategyForRelu(node);
  }
  return SUCCESS;
}

Status FixPipePass::FiltrNodeStrategyForQuant(const FixPipeNodeInfo &cur_node, const FixPipeNodeInfo &prenode) const {
  if (cur_node.GetNode() != nullptr) {
    if (cur_node.GetNode()->GetType() != ASCEND_QUANT) {
      return SUCCESS;
    }
    if (prenode.GetNode() == nullptr) {
      return SUCCESS;
    }
    if (prenode.GetBelongUnitType() != POSTACTUNIT) {
      return SUCCESS;
    }
  }
  float scale = 0.0;
  (void)ge::AttrUtils::GetFloat(cur_node.GetNode()->GetOpDesc(), ATTR_SCALE, scale);
  float offset = 0.0;
  (void)ge::AttrUtils::GetFloat(cur_node.GetNode()->GetOpDesc(), ATTR_OFFSET, offset);
  if (scale < 0) {
    return FAILED;
  }
  if (prenode.GetNode()->GetType() == LEAKY_RELU) {
    float attr_negative_slope_a = 0.0;
    (void)ge::AttrUtils::GetFloat(prenode.GetNode()->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
    if (attr_negative_slope_a < 0) {
      return FAILED;
    }
  }
  if (prenode.GetNode()->GetType() == PRELU) {
    if (!JudgeConstValue(prenode.GetNode())) {
      return FAILED;
    }
  }
  if (prenode.GetNode()->GetType() == RELU6) {
    if (scale * relu6_value + offset < 0) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FixPipePass::FiltrNodeStrategyForRelu(const FixPipeNodeInfo &node) const {
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0) == nullptr) {
    return FAILED;
  }
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0)->GetDataType() == ge::DT_FLOAT16) {
    return SUCCESS;
  }
  return FAILED;
}

Status FixPipePass::FiltrNodeStrategyForTransData(const FixPipeNodeInfo &node) const {
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0) == nullptr) {
    return FAILED;
  }
  if (node.GetNode()->GetOpDesc()->MutableOutputDesc(0) == nullptr) {
    return FAILED;
  }
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0)->GetFormat() == ge::FORMAT_FRACTAL_NZ &&
      node.GetNode()->GetOpDesc()->MutableOutputDesc(0)->GetFormat() == ge::FORMAT_ND) {
    return SUCCESS;
  }
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0)->GetFormat() == ge::FORMAT_NC1HWC0 &&
      node.GetNode()->GetOpDesc()->MutableOutputDesc(0)->GetFormat() == ge::FORMAT_NHWC) {
    return SUCCESS;
  }
  return FAILED;
}

Status FixPipePass::FiltrNodeStrategyForEltWise(const FixPipeNodeInfo &node) const {
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0) == nullptr) {
    return FAILED;
  }
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(1) == nullptr) {
    return FAILED;
  }
  if (node.GetNode()->GetOpDesc()->MutableInputDesc(0)->GetFormat() !=
      node.GetNode()->GetOpDesc()->MutableInputDesc(1)->GetFormat()) {
    return FAILED;
  }
  return SUCCESS;
}

Status FixPipePass::JudgeIsMatch(const FixPipeNodeInfo &node, stack<uint32_t> &cur_index, uint32_t &ret_index) const {
  if (!IsInWhitelist(node)) {
    FE_LOGD("geIsMatch node isn't inwhitelist name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return FAILED;
  }
  if (!node.GetIsHeadNode() && IsCubeOpType(node.GetNode())) {
    FE_LOGD("JudgeIsMatch node iscube but node headnode name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return FAILED;
  }
  if (cur_index.size() > m_idxtonodetypes_.size()) {
    FE_LOGD("JudgeIsMatch cur_index size = %zu  m_idxtonodetypes_ size = %zu", cur_index.size(),
            m_idxtonodetypes_.size());
    return FAILED;
  }
  if (!cur_index.empty() && cur_index.top() >= static_cast<uint32_t>(m_idxtonodetypes_.size() - 1)) {
    FE_LOGD("JudgeIsMatch cur_index is last top = %u  m_idxtonodetypes_ size = %zu", cur_index.top(),
            m_idxtonodetypes_.size());
    return FAILED;
  }
  if (!JudgeCachePass(node, cur_index, ret_index)) {
    FE_LOGD("JudgeIsMatch node isnt JudgeCachePass name = %s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    return FAILED;
  }
  if (FiltrNodeStrategy(node) != SUCCESS) {
    return FAILED;
  }
  FE_LOGD("JudgeIsMatch node is true name = %s type = %s", node.GetNode()->GetName().c_str(),
          node.GetNode()->GetType().c_str());
  return SUCCESS;
}

void FixPipePass::GenerateMatchedPassesImpl(FixPipeNodeInfo &node, stack<FixPipeNodeInfo> &cur_pass,
                                            stack<uint32_t> &cur_index, std::vector<ge::NodePtr> &new_nodes) {
  static uint32_t pass_index = 0;
  uint32_t tmp_cur_index = 0;
  FE_LOGD("GenerateMatchedPassesImpl start node name = %s type = %s", node.GetNode()->GetName().c_str(),
          node.GetNode()->GetType().c_str());
  Status ret = JudgeIsMatch(node, cur_index, tmp_cur_index);
  if (ret != SUCCESS) {
    FE_LOGD("GenerateMatchedPassesImpl node can't be fixpipe name =%s type = %s", node.GetNode()->GetName().c_str(),
            node.GetNode()->GetType().c_str());
    node.SetNodeFixpipeability(NODECANTACCES);
    return;
  }
  if (cur_index.size() > 1) {
    ret = FiltrNodeStrategyForQuant(node, cur_pass.top());
    if (ret != SUCCESS) {
      FE_LOGD("GenerateMatchedPassesImpl post relu+quant node can't be fixpipe name = %s type = %s",
              node.GetNode()->GetName().c_str(), node.GetNode()->GetType().c_str());
      node.SetNodeFixpipeability(NODECANTACCES);
      return;
    }
  }
  node.SetNodeFixpipeability(1);
  node.SetBelongUnitType(m_idxtonodetypes_[tmp_cur_index].GetName());
  node.SetBelongUnitIndex(tmp_cur_index);
  cur_pass.push(node);
  cur_index.push(tmp_cur_index);
  FE_LOGD("GenerateMatchedPassesImpl node cane fixpipe name = %s type = %s belong_unitname = %s index = %d",
          node.GetNode()->GetName().c_str(), node.GetNode()->GetType().c_str(), node.GetBelongUnitType().c_str(),
          tmp_cur_index);

  GenerateMatchedPassesFromStack(cur_pass, pass_index++, tmp_cur_index);
  if (node.GetNode()->GetOutDataNodesSize() > 1) {  // more one outdatanode may circle, only self putin
    return;
  }
  ge::Node::Vistor<ge::NodePtr> out_nodes = node.GetNode()->GetOutDataNodes();
  for (auto &out_node : out_nodes) {
    FE_LOGD(" GenerateMatchedPassesImpl node outnode fixpipe name = %s type = %s", out_node->GetName().c_str(),
            out_node->GetType().c_str());
    FixPipeNodeInfo grandnode(out_node);
    GenerateMatchedPassesImpl(grandnode, cur_pass, cur_index, new_nodes);
  }
  cur_index.pop();
  cur_pass.pop();
  return;
}

void FixPipePass::GenerateMatchedPassesFromStack(const stack<FixPipeNodeInfo> &cur_pass, const uint32_t &pass_index,
                                                 const uint32_t &cur_index) {
  stack<FixPipeNodeInfo> print_pass(cur_pass);
  FixPipePassInfo tmp_pass;
  tmp_pass.m_flag = 0;
  tmp_pass.pass_index = pass_index;
  tmp_pass.unit_index = cur_index;
  stack<FixPipeNodeInfo> res_pass;
  while (!print_pass.empty()) {
    auto node = print_pass.top();
    res_pass.push(node);
    print_pass.pop();
  }
  while (!res_pass.empty()) {
    auto node = res_pass.top();
    tmp_pass.m_opnodes.push_back(node);
    res_pass.pop();
  }
  ChangeOrInsertPass(tmp_pass);
  return;
}

Status FixPipePass::ModfiyMatchedPasses() {
  FE_LOGD("start");
  if (m_matchpasses_.empty()) {
    return FAILED;
  }
  std::vector<FixPipePassInfo> tmp_passes(m_matchpasses_);
  m_matchpasses_.clear();
  for (auto &tmp_pass : tmp_passes) {
    if (!NeedToCutPass(tmp_pass)) {
      FE_LOGD("ModfiyMatchedPasses pass dont needtocut id = %d", tmp_pass.pass_index);
      m_matchpasses_.push_back(tmp_pass);
    }
  }
  return SUCCESS;
}

void FixPipePass::ChangeOrInsertPass(FixPipePassInfo &tmp_pass) {
  bool find_flag = false;
  for (auto &m_already_pass : m_matchpasses_) {
    if (m_already_pass.m_opnodes.size() != tmp_pass.m_opnodes.size()) {
      continue;
    }
    bool m_find_flag = true;
    for (uint32_t index = 0; index < static_cast<uint32_t>(m_already_pass.m_opnodes.size()); index++) {
      if (m_already_pass.m_opnodes[index].GetNode()->GetName() != tmp_pass.m_opnodes[index].GetNode()->GetName()) {
        m_find_flag = false;
        FE_LOGD("ChangeOrInsertPass false index= %d", index);
        break;
      }
    }
    if (m_find_flag) {
      FE_LOGD("ChangeOrInsertPass find_flag = true");
      find_flag = true;
      break;
    }
  }
  if (!find_flag) {
    m_matchpasses_.push_back(tmp_pass);
  }
}

Status FixPipePass::GenerateMatchedPasses(FixPipeNodeInfo &conv_node, std::vector<ge::NodePtr> &new_nodes) {
  if (!m_matchpasses_.empty()) {
    FE_LOGI("GenerateMatchedPasses WARNNIGN passes already exist.");
  }
  stack<FixPipeNodeInfo> m_tofixtmp_nodes;
  stack<uint32_t> cur_index;
  GenerateMatchedPassesImpl(conv_node, m_tofixtmp_nodes, cur_index, new_nodes);
  return SUCCESS;
}

Status FixPipePass::RelinkHeadEdges(FixPipeNodeInfo &head_node, FixPipeNodeInfo &fixpipeenhancenode) const {
  if (head_node.GetNode()->GetOutDataAnchor(0) == nullptr) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkHeadEdges] head_node[%s] outdataanchor empty.",
                    head_node.GetNode()->GetName().c_str());
    return FAILED;
  }
  if (fixpipeenhancenode.GetNode()->GetInDataAnchor(0) == nullptr) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkHeadEdges] fixpipenode [%s] indataanchor empty.",
                    fixpipeenhancenode.GetNode()->GetName().c_str());
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(head_node.GetNode()->GetOutDataAnchor(0),
                              fixpipeenhancenode.GetNode()->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkHeadEdges] Fail to add edge between src node [%s] and dst node[%s].",
                    head_node.GetNode()->GetName().c_str(), fixpipeenhancenode.GetNode()->GetName().c_str());
    return FAILED;
  }
  if (head_node.GetNode()->GetOpDesc()->MutableOutputDesc(0) != nullptr) {
    fixpipeenhancenode.GetNode()->GetOpDesc()->UpdateInputDesc(
        0, head_node.GetNode()->GetOpDesc()->MutableOutputDesc(0)->Clone());
  }
  return SUCCESS;
}

Status FixPipePass::RelinkOutputEdges(const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode) const {
  std::vector<FixPipeNodeInfo> fixednodeids(match_pass.m_opnodes.begin() + 1, match_pass.m_opnodes.end());
  if (!CheckIsInVector(match_pass.m_opnodes)) {
    return FAILED;
  }
  FixPipeNodeInfo last_tofuzednode = match_pass.m_opnodes[match_pass.m_opnodes.size() - 1];
  auto out_nodes_dataanchor = last_tofuzednode.GetNode()->GetOutDataAnchor(0);
  size_t out_data_size_num = last_tofuzednode.GetNode()->GetOutDataNodesSize();
  FE_LOGD("RelinkOpEdges 4 size = %zu", out_data_size_num);
  if (out_nodes_dataanchor == nullptr) {
    return SUCCESS;
  }
  for (auto &outnodes_peeranchor : out_nodes_dataanchor->GetPeerInDataAnchors()) {
    if (outnodes_peeranchor == nullptr) {
      continue;
    }
    auto outchild_node = outnodes_peeranchor->GetOwnerNode();
    FixPipeNodeInfo grandnode(outchild_node);
    if (!PreMatchAcorrdingToPass(match_pass, grandnode) || (out_data_size_num > 1)) {
      outnodes_peeranchor->UnlinkAll();
      if (!AddEdges(fixpipeenhancenode.GetNode()->GetOutDataAnchor(0), outnodes_peeranchor,
                    fixpipeenhancenode.GetNode(), outchild_node)) {
        return FAILED;
      } else {
        FE_LOGD("src_node = %s dst_node = %s", fixpipeenhancenode.GetNode()->GetName().c_str(),
                outchild_node->GetName().c_str());
      }
      (void)RemoveAnchor(out_nodes_dataanchor, outnodes_peeranchor);
      (void)RemoveAnchor(last_tofuzednode.GetNode()->GetOutControlAnchor(), outchild_node->GetInControlAnchor());
    }
  }
  return SUCCESS;
}

Status FixPipePass::RelinkAntiEltWiseEdges(ge::NodePtr inputfather_node, const FixPipeNodeInfo &tofuzednode,
                                           FixPipeNodeInfo &fixpipeenhancenode) {
  FixPipeNodePair nodepair(inputfather_node, tofuzednode.GetNode());
  m_toantiquantnodes_.push_back(nodepair);
  if (inputfather_node->GetInDataAnchor(0) != nullptr &&
      inputfather_node->GetInDataAnchor(0)->GetPeerOutAnchor() != nullptr &&
      fixpipeenhancenode.GetNode()->GetInDataAnchor(1) != nullptr) {
    if (ge::GraphUtils::AddEdge(inputfather_node->GetInDataAnchor(0)->GetPeerOutAnchor(),
                                fixpipeenhancenode.GetNode()->GetInDataAnchor(1)) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FixpipePss][RelinkAntiEltWiseEdges]  Fail to add  edge between src node [%s] and dst node[%s].",
          inputfather_node->GetName().c_str(), fixpipeenhancenode.GetNode()->GetName().c_str());
      return FAILED;
    }
  }
  if (inputfather_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc()->MutableOutputDesc(0) !=
      nullptr) {
    fixpipeenhancenode.GetNode()->GetOpDesc()->UpdateInputDesc(1, inputfather_node->GetInDataAnchor(0)
                                                                      ->GetPeerOutAnchor()
                                                                      ->GetOwnerNode()
                                                                      ->GetOpDesc()
                                                                      ->MutableOutputDesc(0)
                                                                      ->Clone());
  }
  return SUCCESS;
}

Status FixPipePass::RelinkOtherEltWiseEdges(ge::OutDataAnchorPtr peer_anchor, const FixPipeNodeInfo &tofuzednode,
                                            FixPipeNodeInfo &fixpipeenhancenode, ge::NodePtr inputfather_node) const {
  if (peer_anchor != nullptr && fixpipeenhancenode.GetNode()->GetInDataAnchor(1) != nullptr) {
    if (ge::GraphUtils::AddEdge(peer_anchor, fixpipeenhancenode.GetNode()->GetInDataAnchor(1)) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FixpipePss][RelinkOtherEltWiseEdges] Fail to add  edge between src node [%s] and dst node[%s].",
          inputfather_node->GetName().c_str(), fixpipeenhancenode.GetNode()->GetName().c_str());
      return FAILED;
    }
  }
  if (fixpipeenhancenode.GetNode()->GetInControlAnchor() != nullptr &&
      HasEdge(inputfather_node->GetOutControlAnchor(), tofuzednode.GetNode()->GetInControlAnchor())) {
    if (ge::GraphUtils::AddEdge(inputfather_node->GetOutControlAnchor(),
                                fixpipeenhancenode.GetNode()->GetInControlAnchor()) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FixpipePss][RelinkOtherEltWiseEdges] Fail to add  edge between src node [%s] and dst node[%s].",
          inputfather_node->GetName().c_str(), fixpipeenhancenode.GetNode()->GetName().c_str());
      return FAILED;
    }
  }
  if (inputfather_node->GetOpDesc()->MutableOutputDesc(0) != nullptr) {
    fixpipeenhancenode.GetNode()->GetOpDesc()->UpdateInputDesc(
        1, inputfather_node->GetOpDesc()->MutableOutputDesc(0)->Clone());
  }
  return SUCCESS;
}

Status FixPipePass::RelinkEltWiseEdges(const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode) {
  std::vector<FixPipeNodeInfo> fixednodeids(match_pass.m_opnodes.begin() + 1, match_pass.m_opnodes.end());
  FE_LOGD("RelinkOpEdges 3 ");
  for (auto &tofuzednode : fixednodeids) {
    if (tofuzednode.GetBelongUnitType() == POSTELTWISEUNIT) {
      return RelinkEltWiseEdgesImpl(match_pass, tofuzednode, fixpipeenhancenode);
    }
  }
  return SUCCESS;
}

Status FixPipePass::RelinkEltWiseEdgesImpl(const FixPipePassInfo &match_pass, FixPipeNodeInfo &tofuzednode,
                                           FixPipeNodeInfo &fixpipeenhancenode) {
  FE_LOGD("RelinkOpEdges 3.1 ");
  for (auto &in_nodes_dataanchor : tofuzednode.GetNode()->GetAllInDataAnchors()) {
    if (in_nodes_dataanchor == nullptr) {
      FE_LOGD("in_nodes_dataanchor == null");
      continue;
    }
    auto peer_anchor = in_nodes_dataanchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      FE_LOGD("peer_outanchor == null");
      continue;
    }
    auto inputfather_node = peer_anchor->GetOwnerNode();
    if (IsNodeInPass(match_pass.m_opnodes, inputfather_node)) {
      FE_LOGD("inputfathernode is infixpipenode name = %s", inputfather_node->GetName().c_str());
      continue;
    }
    if (inputfather_node->GetType() == ASCENDANTIQUANT) {
      return RelinkAntiEltWiseEdges(inputfather_node, tofuzednode, fixpipeenhancenode);
    } else {
      return RelinkOtherEltWiseEdges(peer_anchor, tofuzednode, fixpipeenhancenode, inputfather_node);
    }
  }
  return SUCCESS;
}

Status FixPipePass::RelinkOpEdges(FixPipeNodeInfo &head_node,
                                  const FixPipePassInfo &match_pass, FixPipeNodeInfo &fixpipeenhancenode) {
  if (RelinkHeadEdges(head_node, fixpipeenhancenode) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkOpEdges]  Fail to add edge between head node [%s] and dst node[%s].",
                    head_node.GetNode()->GetName().c_str(), fixpipeenhancenode.GetNode()->GetName().c_str());
    return FAILED;
  }
  if (RelinkEltWiseEdges(match_pass, fixpipeenhancenode) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkOpEdges]  Fail to add edge between  dst node[%s].",
                    fixpipeenhancenode.GetNode()->GetName().c_str());
    return FAILED;
  }
  if (RelinkOutputEdges(match_pass, fixpipeenhancenode) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelinkOpEdges]  Fail to add edge between  dst node[%s].",
                    fixpipeenhancenode.GetNode()->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool FixPipePass::IsNodeInPass(const std::vector<FixPipeNodeInfo> &fixednodeids, const ge::NodePtr input_node) const {
  bool found = (fixednodeids.end() != std::find(fixednodeids.begin(), fixednodeids.end(), input_node));
  FE_LOGD("IsNodeInPass is %u name = %s type = %s", found, input_node->GetName().c_str(),
          input_node->GetType().c_str());
  return found;
}

Status FixPipePass::AddInputs(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                              ge::NodePtr fixpipenode, std::vector<ge::NodePtr> &new_nodes) {
  for (uint32_t i = FIXPIPE_INPUT_2_INDEX; i < 10; i++) {
    FixPipeFunctionParamPtr funtcparam;
    FE_MAKE_SHARED(funtcparam = std::make_shared<FixPipeFunctionParam>("Dummy", fixpipenode, i), return FAILED);
    FixPipeAddInputPtr addinputptr = AddInputStrategy(match_pass, funtcparam);
    if (addinputptr != nullptr) {
      if (addinputptr->DoAddInput(graph, match_pass, funtcparam, new_nodes) != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status FixPipePass::AddInputDesc(ge::OpDescPtr cur_new_fixpipeopdesc) const {
  ge::GeShape shape{};
  ge::GeTensorDesc fakedesc(shape, ge::FORMAT_RESERVED, ge::DT_UNDEFINED);
  cur_new_fixpipeopdesc->AddInputDesc(X1INPUTNAME, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(X2INPUTNAME, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(QUANTSCALE0INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(RELUWEIGHT0INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(CLIPVALUE0INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(QUANTSCALE1INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(RELUWEIGHT1INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(CLIPVALUE1INPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(ANTIQUANTSACALEINPUT, fakedesc);
  cur_new_fixpipeopdesc->AddInputDesc(ANTIQUANTOFFSETINPUT, fakedesc);
  return SUCCESS;
}

ge::NodePtr FixPipePass::CreateFixpipeNode(const FixPipePassInfo &match_pass, const FixPipeNodeInfo &head_node,
                                           ge::ComputeGraph &graph) const {
  FE_LOGD("CreateFixpipeNode 1 ");
  std::string op_name = head_node.GetNode()->GetOpDesc()->GetName() + "_pass." + std::to_string(match_pass.pass_index);
  ge::OpDescPtr fixpipenodedesc = nullptr;
  FE_MAKE_SHARED(fixpipenodedesc = std::make_shared<ge::OpDesc>(op_name, FIXPIPE), return nullptr);
  FE_CHECK(fixpipenodedesc == nullptr, REPORT_FE_ERROR("[GraphOpt][Fixpipepas][CrtHostNd] Fail to make shared \
           of host op desc."),
           return nullptr);
  FE_LOGD("CreateFixpipeNode 2 ");
  fixpipenodedesc->SetType(FIXPIPE);
  std::vector<FixPipeNodeInfo> fixednodeids(match_pass.m_opnodes.begin() + 1, match_pass.m_opnodes.end());
  std::vector<std::string> fuzedoptypes;
  std::vector<std::string> activatefixpipeunits;
  std::string eltwiseops;
  for (auto &tofuzednode : fixednodeids) {
    fuzedoptypes.push_back(tofuzednode.GetNode()->GetType());
    activatefixpipeunits.push_back(tofuzednode.GetBelongUnitType());
    if (tofuzednode.GetBelongUnitType() == POSTELTWISEUNIT) {
      if (tofuzednode.GetNode()->GetType() == ELTWISE) {
        eltwiseops = GetEltWiseType(tofuzednode);
      } else {
        eltwiseops = tofuzednode.GetNode()->GetType();
      }
    }
  }
  if (fixednodeids[0].GetNode()->GetType() == ASCEND_DEQUANT &&
      ge::AttrUtils::HasAttr(fixednodeids[0].GetNode()->GetOpDesc(), ATTR_OFFSET)) {
    if (fixednodeids.size() > 1 && fixednodeids[1].GetBelongUnitType() == PREACTUNIT) {
      fuzedoptypes.insert(fuzedoptypes.cbegin() + INSERTWOINDEX, ASCEND_QUANT);
    } else {
      fuzedoptypes.insert(fuzedoptypes.cbegin() + 1, ASCEND_QUANT);
    }
  }
  StringUtils::ToUpperString(eltwiseops);
  (void)ge::AttrUtils::SetStr(fixpipenodedesc, ELTWISE_MODE, eltwiseops.substr(0, ELTWISEOPSUBSTRINDEX));
  (void)ge::AttrUtils::SetListStr(fixpipenodedesc, FUSIONOPLIST, fuzedoptypes);
  (void)ge::AttrUtils::SetListStr(fixpipenodedesc, ACTIVATIEUNIT, activatefixpipeunits);
  AddInputDesc(fixpipenodedesc);
  if (!CheckIsInVector(match_pass.m_opnodes)) {
    return nullptr;
  }
  FixPipeNodeInfo last_tofuzednode = match_pass.m_opnodes[match_pass.m_opnodes.size() - 1];
  fixpipenodedesc->AddOutputDesc("output", *(last_tofuzednode.GetNode()->GetOpDesc()->MutableOutputDesc(0)));
  FE_LOGD("reateFixpipeNode node_name = %s", op_name.c_str());
  ge::NodePtr fixpipenode = graph.AddNode(fixpipenodedesc);
  return fixpipenode;
}

Status FixPipePass::DeleteToFusedNodeEdge(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                          std::vector<ge::NodePtr> &todeletenode) const {
  std::vector<FixPipeNodeInfo> fixednodeids(match_pass.m_opnodes.begin() + 1, match_pass.m_opnodes.end());
  for (auto &tofuzednode : fixednodeids) {
    if (graph.FindNode(tofuzednode.GetNode()->GetName()) == nullptr) {
      continue;
    }
    FE_LOGD("DeleteToFusedNodeEdge 2 name = %s type = %s", tofuzednode.GetNode()->GetName().c_str(),
            tofuzednode.GetNode()->GetType().c_str());
    if (tofuzednode.GetNode()->GetType() == ASCENDDEQUANT || tofuzednode.GetNode()->GetType() == ASCENDREQUANT ||
        tofuzednode.GetNode()->GetType() == PRELU) {
      if (HasInput1Node(tofuzednode.GetNode())) {
        auto inputnode = tofuzednode.GetNode()->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
        RemoveAnchor(inputnode->GetOutDataAnchor(0), tofuzednode.GetNode()->GetInDataAnchor(1));
        RemoveAnchor(inputnode->GetOutControlAnchor(), tofuzednode.GetNode()->GetInControlAnchor());
      }
    }
    if (HasInputNode(tofuzednode.GetNode(), 0)) {
      auto inputfather_node = tofuzednode.GetNode()->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
      if (!IsNodeInPass(match_pass.m_opnodes, inputfather_node)) {
        FE_LOGD("DeleteToFusedNodeEdge 5 fathernode name = %s type = %s", inputfather_node->GetName().c_str(),
                inputfather_node->GetType().c_str());
        continue;
      }
    }
    todeletenode.push_back(tofuzednode.GetNode());
  }
  return SUCCESS;
}

Status FixPipePass::DeleteNode(const std::vector<ge::NodePtr> &todeletenode) {
  for (auto &node : todeletenode) {
    (void)ge::GraphUtils::IsolateNode(node, {});
    (void)ge::GraphUtils::RemoveNodeWithoutRelink(node->GetOwnerComputeGraph(), node);
  }
  for (auto &node : m_toantiquantnodes_) {
    RemoveAnchor(node.GetParent()->GetOutDataAnchor(0), node.GetChild()->GetInDataAnchor(0));
    RemoveAnchor(node.GetParent()->GetOutControlAnchor(), node.GetChild()->GetInControlAnchor());
    if (node.GetParent()->GetOutDataNodesSize() == 0) {
      if (node.GetParent() == nullptr) {
        continue;
      } else {
        (void)ge::GraphUtils::IsolateNode(node.GetParent(), {});
        (void)ge::GraphUtils::RemoveNodeWithoutRelink(node.GetParent()->GetOwnerComputeGraph(), node.GetParent());
      }
    }
  }
  return SUCCESS;
}

Status FixPipePass::FusionImpl(FixPipeNodeInfo &head_node, ge::ComputeGraph &graph,
                               std::vector<ge::NodePtr> &new_nodes) {
  for (auto &pass : m_matchpasses_) {
    FE_LOGD("FusionImpl 1.1");
    ge::NodePtr fixpipenode = CreateFixpipeNode(pass, head_node, graph);
    FE_LOGD("FusionImpl 1.2");
    FixPipeNodeInfo fixpipeenhancenode(fixpipenode);
    FE_LOGD("FusionImpl 1.3");
    AddInputs(graph, pass, fixpipenode, new_nodes);
    FE_LOGD("FusionImpl 1.4");
    RelinkOpEdges(head_node, pass, fixpipeenhancenode);
    new_nodes.push_back(fixpipenode);
  }
  std::vector<ge::NodePtr> todeletenode;
  for (auto &pass : m_matchpasses_) {
    DeleteToFusedNodeEdge(graph, pass, todeletenode);
  }
  DeleteNode(todeletenode);
  return SUCCESS;
}

Status FixPipePass::InitInput2() {
  FixPipeAddInputPtr strategy21ptr = nullptr;
  FE_MAKE_SHARED(strategy21ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy21>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(QUANTSCALE0INPUT + ":" + ASCENDQUANT, strategy21ptr));
  FE_LOGD("strategy21 name = %s", (QUANTSCALE0INPUT + ":" + ASCENDQUANT).c_str());
  FixPipeAddInputPtr strategy22ptr = nullptr;
  FE_MAKE_SHARED(strategy22ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy22>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(QUANTSCALE0INPUT + ":" + ASCENDREQUANT, strategy22ptr));
  FE_LOGD("strategy22 name = %s", (QUANTSCALE0INPUT + ":" + ASCENDREQUANT).c_str());
  m_opmaps_.emplace(make_pair(QUANTSCALE0INPUT + ":" + ASCENDDEQUANT, strategy22ptr));
  FE_LOGD("strategy22 name = %s", (QUANTSCALE0INPUT + ":" + ASCENDDEQUANT).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput3() {
  FixPipeAddInputPtr strategy31ptr = nullptr;
  FE_MAKE_SHARED(strategy31ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy31>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDQUANT + "_" + PRELU, strategy31ptr));
  FE_LOGD("strategy31 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDQUANT + "_" + PRELU).c_str());
  FixPipeAddInputPtr strategy32ptr = nullptr;
  FE_MAKE_SHARED(strategy32ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy32>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDDEQUANT + "_" + PRELU, strategy32ptr));
  FE_LOGD("strategy32 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDDEQUANT + "_" + PRELU).c_str());
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDREQUANT + "_" + PRELU, strategy32ptr));
  FE_LOGD("strategy32 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDREQUANT + "_" + PRELU).c_str());
  FixPipeAddInputPtr strategy33ptr = nullptr;
  FE_MAKE_SHARED(strategy33ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy33>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + PRELU, strategy33ptr));
  FE_LOGD("strategy33 name = %s", (RELUWEIGHT0INPUT + ":" + PRELU).c_str());
  FixPipeAddInputPtr strategy34ptr = nullptr;
  FE_MAKE_SHARED(strategy34ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy34>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDQUANT + "_" + LEAKYRELU, strategy34ptr));
  FE_LOGD("strategy34 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDQUANT + "_" + LEAKYRELU).c_str());
  FixPipeAddInputPtr strategy35ptr = nullptr;
  FE_MAKE_SHARED(strategy35ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy35>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDDEQUANT + "_" + LEAKYRELU, strategy35ptr));
  FE_LOGD("strategy35 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDDEQUANT + "_" + LEAKYRELU).c_str());
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + ASCENDREQUANT + "_" + LEAKYRELU, strategy35ptr));
  FE_LOGD("strategy35 name = %s", (RELUWEIGHT0INPUT + ":" + ASCENDREQUANT + "_" + LEAKYRELU).c_str());
  FixPipeAddInputPtr strategy36ptr = nullptr;
  FE_MAKE_SHARED(strategy36ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy36>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + LEAKYRELU, strategy36ptr));
  FE_LOGD("strategy36 name = %s", (RELUWEIGHT0INPUT + ":" + LEAKYRELU).c_str());
  FixPipeAddInputPtr strategy37ptr = nullptr;
  FE_MAKE_SHARED(strategy37ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy37>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + CAST + "_" + PRELU, strategy37ptr));
  FE_LOGD("strategy37 name = %s", (RELUWEIGHT0INPUT + ":" + CAST + "_" + PRELU).c_str());
  FixPipeAddInputPtr strategy38ptr = nullptr;
  FE_MAKE_SHARED(strategy38ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy38>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT0INPUT + ":" + CAST + "_" + LEAKYRELU, strategy38ptr));
  FE_LOGD("strategy38 name = %s", (RELUWEIGHT0INPUT + ":" + CAST + "_" + LEAKYRELU).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput4() {
  FixPipeAddInputPtr strategy41ptr = nullptr;
  FE_MAKE_SHARED(strategy41ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy41>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(CLIPVALUE0INPUT + ":" + RELU6, strategy41ptr));
  FE_LOGD("strategy41 name = %s", (CLIPVALUE0INPUT + ":" + RELU6).c_str());
  FixPipeAddInputPtr strategy42ptr = nullptr;
  FE_MAKE_SHARED(strategy42ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy42>()),
                 return FAILED);
  FixPipeAddInputPtr strategy43ptr = nullptr;
  FE_MAKE_SHARED(strategy43ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy43>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(CLIPVALUE0INPUT + ":" + ASCENDDEQUANT + "_" + RELU6, strategy43ptr));
  FE_LOGD("strategy43 name = %s", (CLIPVALUE0INPUT + ":" + ASCENDDEQUANT + "_" + RELU6).c_str());
  m_opmaps_.emplace(make_pair(CLIPVALUE0INPUT + ":" + ASCENDREQUANT + "_" + RELU6, strategy43ptr));
  FE_LOGD("strategy43 name = %s", (CLIPVALUE0INPUT + ":" + ASCENDREQUANT + "_" + RELU6).c_str());

  m_opmaps_.emplace(make_pair(CLIPVALUE1INPUT + ":" + RELU6, strategy41ptr));
  FE_LOGD("strategy41 name = %s", (CLIPVALUE1INPUT + ":" + RELU6).c_str());
  m_opmaps_.emplace(make_pair(CLIPVALUE1INPUT + ":" + RELU6 + "_" + ASCENDQUANT, strategy42ptr));
  FE_LOGD("strategy42 name = %s", (CLIPVALUE1INPUT + ":" + RELU6 + "_" + ASCENDQUANT).c_str());
  FixPipeAddInputPtr strategy44ptr = nullptr;
  FE_MAKE_SHARED(strategy44ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy44>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(CLIPVALUE0INPUT + ":" + CAST + "_" + RELU6, strategy44ptr));
  FE_LOGD("strategy44 name = %s", (CLIPVALUE0INPUT + ":" + CAST + "_" + RELU6).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput5() {
  FixPipeAddInputPtr strategy51ptr = nullptr;
  FE_MAKE_SHARED(strategy51ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy51>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(QUANTSCALE1INPUT + ":" + ASCENDQUANT, strategy51ptr));
  FE_LOGD("strategy51 name = %s", (QUANTSCALE1INPUT + ":" + ASCENDQUANT).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput6() {
  FixPipeAddInputPtr strategy61ptr = nullptr;
  FE_MAKE_SHARED(strategy61ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy61>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT1INPUT + ":" + PRELU + "_" + ASCENDQUANT, strategy61ptr));
  FE_LOGD("strategy61 name = %s", (RELUWEIGHT1INPUT + ":" + PRELU + "_" + ASCENDQUANT).c_str());

  FixPipeAddInputPtr strategy62ptr = nullptr;
  FE_MAKE_SHARED(strategy62ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy62>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT1INPUT + ":" + PRELU, strategy62ptr));
  FE_LOGD("strategy62 name = %s", (RELUWEIGHT1INPUT + ":" + PRELU).c_str());

  FixPipeAddInputPtr strategy63ptr = nullptr;
  FE_MAKE_SHARED(strategy63ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy63>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT1INPUT + ":" + LEAKYRELU + "_" + ASCENDQUANT, strategy63ptr));
  FE_LOGD("strategy63 name = %s", (RELUWEIGHT1INPUT + ":" + LEAKYRELU + "_" + ASCENDQUANT).c_str());
  FixPipeAddInputPtr strategy64ptr = nullptr;
  FE_MAKE_SHARED(strategy64ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy64>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(RELUWEIGHT1INPUT + ":" + LEAKYRELU, strategy64ptr));
  FE_LOGD("strategy64 name = %s", (RELUWEIGHT1INPUT + ":" + LEAKYRELU).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput8() {
  FixPipeAddInputPtr strategy81ptr = nullptr;
  FE_MAKE_SHARED(strategy81ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy81>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(ANTIQUANTSACALEINPUT + ":" + ASCENDANTIQUANT, strategy81ptr));
  FE_LOGD("strategy81 name = %s", (ANTIQUANTSACALEINPUT + ":" + ASCENDANTIQUANT).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInput9() {
  FixPipeAddInputPtr strategy91ptr = nullptr;
  FE_MAKE_SHARED(strategy91ptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategy91>()),
                 return FAILED);
  m_opmaps_.emplace(make_pair(ANTIQUANTOFFSETINPUT + ":" + ASCENDANTIQUANT, strategy91ptr));
  FE_LOGD("strategy91 name = %s", (ANTIQUANTOFFSETINPUT + ":" + ASCENDANTIQUANT).c_str());
  return SUCCESS;
}

Status FixPipePass::InitInputDefault() {
  FixPipeAddInputPtr strategydefaultptr = nullptr;
  FE_MAKE_SHARED(
      strategydefaultptr = shared_ptr<FixPipeAddInputBase>(std::make_shared<FixPipeAddInputStrategyDefault>()),
      return FAILED);
  m_opmaps_.emplace(make_pair("DEFAULT", strategydefaultptr));
  FE_LOGD("strategydefault name = DEFAULT");
  return SUCCESS;
}

Status FixPipePass::Init() {
  if (InitInput2() != SUCCESS) {
    return FAILED;
  }
  if (InitInput3() != SUCCESS) {
    return FAILED;
  }
  if (InitInput4() != SUCCESS) {
    return FAILED;
  }
  if (InitInput5() != SUCCESS) {
    return FAILED;
  }
  if (InitInput6() != SUCCESS) {
    return FAILED;
  }
  if (InitInput8() != SUCCESS) {
    return FAILED;
  }
  if (InitInput9() != SUCCESS) {
    return FAILED;
  }
  if (InitInputDefault() != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

void FixPipePass::ClearPasses() {
  m_matchpasses_.clear();
  m_toantiquantnodes_.clear();
  m_idxtonodetypes_.clear();
  unitmapindex_.clear();
}

Status FixPipePass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, std::vector<ge::NodePtr> &new_nodes) {
  ClearPasses();
  if (!ReadConfig()) {
    return NOT_CHANGED;
  }
  if (Init() != SUCCESS) {
    FE_LOGD("Fusion init failed.");
    return NOT_CHANGED;
  }
  ge::NodePtr conv_node = GetNodeFromMapping(PATTERN_CONV, mapping);
  FE_CHECK(conv_node == nullptr, REPORT_FE_ERROR("[GraphOpt][FixpipePss][Fusion] Conv node is nullptr."),
           return FAILED);
  FE_LOGD("convnode_name = %s type = %s", conv_node->GetName().c_str(), conv_node->GetType().c_str());
  if (!IsCubeOpType(conv_node)) {
    return NOT_CHANGED;
  }
  FE_LOGD("Fusion 2");
  FixPipeNodeInfo cur_head_info(conv_node);
  cur_head_info.SetIsHeadNode(true);
  GenerateMatchedPasses(cur_head_info, new_nodes);
  ModfiyMatchedPasses();
  for (auto &pass : m_matchpasses_) {
    FE_LOGD("Fusion 3 matchedpass passid = %d", pass.pass_index);
    for (auto &node : pass.m_opnodes) {
      FE_LOGD("Fusion 3 node_name = %s type = %s", node.GetNode()->GetName().c_str(),
              node.GetNode()->GetType().c_str());
    }
  }
  FE_LOGD("Fusion 4");
  return FusionImpl(cur_head_info, graph, new_nodes);
}

FixPipeAddInputPtr FixPipePass::AddInputStrategy(const FixPipePassInfo &match_pass,
                                                 FixPipeFunctionParamPtr funtcparam) {
  switch (funtcparam->GetParaIndex()) {
    case FIXPIPE_INPUT_2_INDEX:  // quant_scale0
      return AddInput2Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_3_INDEX:  // relu_weight_0
      return AddInput3Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_4_INDEX:  // clip_value_0
      return AddInput4Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_5_INDEX:  // quant_scale1
      return AddInput5Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_6_INDEX:  // relu_weight_1
      return AddInput6Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_7_INDEX:  // clip_value_1
      return AddInput7Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_8_INDEX:  // eltwise+antiquant
      return AddInput8Strategy(match_pass, funtcparam);
    case FIXPIPE_INPUT_9_INDEX:  // eltwise+antiquant
      return AddInput9Strategy(match_pass, funtcparam);
    default:
      return nullptr;
  }
}

FixPipeAddInputPtr FixPipePass::AddInputSingleUnitStrategy(const FixPipePassInfo &match_pass,
                                                           FixPipeFunctionParamPtr funtcparam,
                                                           const std::string &first_unitname) {
  FE_LOGD("AddInputSingleUnitStrategy funtcparam->GetInputName = %s inputdex = %d first_unitname = %s",
          funtcparam->GetInputName().c_str(), funtcparam->GetParaIndex(), first_unitname.c_str());
  for (size_t idx = 1; idx < match_pass.m_opnodes.size(); idx++) {
    auto tofuzednode = match_pass.m_opnodes[idx];
    if (tofuzednode.GetBelongUnitType() == first_unitname) {
      funtcparam->SetFirstIndex(idx);
      funtcparam->SetSecondIndex(idx);
      FE_LOGD("first_index = %d firstname = %s second_index = %d secondname = %s", funtcparam->GetFirstIndex(),
              match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType().c_str(),
              funtcparam->GetSecondIndex(),
              match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType().c_str());
      return m_opmaps_[funtcparam->GetInputName() + ":" + tofuzednode.GetNode()->GetType()];
    }
  }
  FE_LOGD("AddInputSingleUnitStrategy default");
  return m_opmaps_["DEFAULT"];
}

FixPipeAddInputPtr FixPipePass::AddInputAntiStrategy(const FixPipePassInfo &match_pass,
                                                     FixPipeFunctionParamPtr funtcparam,
                                                     const std::string &first_unitname) {
  FE_LOGD("AddInputAntiStrategy funtcparam->GetInputName = %s inputdex = %d first_unitname = %s",
          funtcparam->GetInputName().c_str(), funtcparam->GetParaIndex(), first_unitname.c_str());
  for (size_t idx = 1; idx < match_pass.m_opnodes.size(); idx++) {
    auto tofuzednode = match_pass.m_opnodes[idx];
    if (tofuzednode.GetBelongUnitType() != first_unitname) {
      continue;
    }
    for (auto &in_nodes_dataanchor : tofuzednode.GetNode()->GetAllInDataAnchors()) {
      if (in_nodes_dataanchor == nullptr) {
        REPORT_FE_ERROR("[GraphOpt][FixpipePss][AddInputAntiStrategy] node [%s] ",
                        tofuzednode.GetNode()->GetName().c_str());
        continue;
      }
      auto peer_anchor = in_nodes_dataanchor->GetPeerOutAnchor();
      if (peer_anchor == nullptr) {
        continue;
      }
      auto inputfather_node = peer_anchor->GetOwnerNode();
      if (IsNodeInPass(match_pass.m_opnodes, inputfather_node)) {
        continue;
      }
      if (inputfather_node->GetType() != ASCENDANTIQUANT) {
        continue;
      }
      float scale_tmp = 0.0;
      if (!ge::AttrUtils::GetFloat(inputfather_node->GetOpDesc(), ATTR_SCALE, scale_tmp)) {
        FE_LOGW("Get scale attr of quant node[%s] failed!", inputfather_node->GetName().c_str());
        return m_opmaps_["DEFAULT"];
      }
      float offset_a = 0.0f;
      (void)ge::AttrUtils::GetFloat(inputfather_node->GetOpDesc(), ATTR_OFFSET, offset_a);
      funtcparam->SetFirstIndex(idx);
      funtcparam->SetSecondIndex(idx);
      funtcparam->SetDataType(inputfather_node->GetOpDesc()->MutableInputDesc(0)->GetDataType());
      (void)ge::AttrUtils::SetFloat(tofuzednode.GetNode()->GetOpDesc(), ATTR_OFFSET, offset_a);
      (void)ge::AttrUtils::SetFloat(tofuzednode.GetNode()->GetOpDesc(), ATTR_SCALE, scale_tmp);
      FE_LOGD("first_index = %d firstname = %s second_index = %d secondname = %s", funtcparam->GetFirstIndex(),
              match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType().c_str(),
              funtcparam->GetSecondIndex(),
              match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType().c_str());
      return m_opmaps_[funtcparam->GetInputName() + ":" + inputfather_node->GetType()];
    }
  }
  FE_LOGD("AddInputAntiStrategy default");
  return m_opmaps_["DEFAULT"];
}

FixPipeAddInputPtr FixPipePass::AddInputTwoUnitStrategy(const FixPipePassInfo &match_pass,
                                                        FixPipeFunctionParamPtr funtcparam,
                                                        const std::string &first_unitname,
                                                        const std::string &second_unitname) {
  FE_LOGD(
      "AddInputTwoUnitStrategy funtcparam->GetInputName = %s inputdex = %d first_unitname = %s second_unitname = %s",
      funtcparam->GetInputName().c_str(), funtcparam->GetParaIndex(), first_unitname.c_str(), second_unitname.c_str());
  bool has_first_unit = false;
  bool has_second_unit = false;
  for (size_t idx = 1; idx < match_pass.m_opnodes.size(); idx++) {
    auto tofuzednode = match_pass.m_opnodes[idx];
    if (tofuzednode.GetBelongUnitType() == first_unitname) {
      funtcparam->SetFirstIndex(idx);
      has_first_unit = true;
    }
    if (tofuzednode.GetBelongUnitType() == second_unitname) {
      funtcparam->SetSecondIndex(idx);
      has_second_unit = true;
    }
  }
  if (has_second_unit) {
    if (has_first_unit) {
      FE_LOGD("1.first_index = %d second_index = %d firstname = %ssecondname = %s", funtcparam->GetFirstIndex(),
              funtcparam->GetSecondIndex(),
              match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType().c_str(),
              match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType().c_str());
      return m_opmaps_[funtcparam->GetInputName() + ":" +
                       match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType() + "_" +
                       match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType()];
    } else {
      funtcparam->SetFirstIndex(funtcparam->GetSecondIndex());
      FE_LOGD("2.second_index = %d secondname = %s", funtcparam->GetSecondIndex(),
              match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType().c_str());
      return m_opmaps_[funtcparam->GetInputName() + ":" +
                       match_pass.m_opnodes[funtcparam->GetSecondIndex()].GetNode()->GetType()];
    }
  } else {
    if (has_first_unit) {
      funtcparam->SetSecondIndex(funtcparam->GetFirstIndex());
      FE_LOGD("3.first_index = %d firstname = %s", funtcparam->GetFirstIndex(),
              match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType().c_str());
      return m_opmaps_[funtcparam->GetInputName() + ":" +
                       match_pass.m_opnodes[funtcparam->GetFirstIndex()].GetNode()->GetType()];
    }
  }
  FE_LOGD("AddInputTwoUnitStrategy default");
  return m_opmaps_["DEFAULT"];
}

FixPipeAddInputPtr FixPipePass::AddInput2Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(QUANTSCALE0INPUT);
  funtcparam->SetDataType(ge::DT_UINT64);
  return AddInputSingleUnitStrategy(match_pass, funtcparam, PRECONVUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput3Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(RELUWEIGHT0INPUT);
  funtcparam->SetDataType(ge::DT_FLOAT);
  return AddInputTwoUnitStrategy(match_pass, funtcparam, PRECONVUNIT, PREACTUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput4Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(CLIPVALUE0INPUT);
  return AddInputTwoUnitStrategy(match_pass, funtcparam, PRECONVUNIT, PREACTUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput5Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(QUANTSCALE1INPUT);
  funtcparam->SetDataType(ge::DT_UINT64);
  return AddInputSingleUnitStrategy(match_pass, funtcparam, POSTQUANTUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput6Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(RELUWEIGHT1INPUT);
  funtcparam->SetDataType(ge::DT_FLOAT);
  return AddInputTwoUnitStrategy(match_pass, funtcparam, POSTACTUNIT, POSTQUANTUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput7Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(CLIPVALUE1INPUT);
  funtcparam->SetDataType(ge::DT_FLOAT16);
  return AddInputTwoUnitStrategy(match_pass, funtcparam, POSTACTUNIT, POSTQUANTUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput8Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(ANTIQUANTSACALEINPUT);
  funtcparam->SetDataType(ge::DT_FLOAT16);
  return AddInputAntiStrategy(match_pass, funtcparam, POSTELTWISEUNIT);
}

FixPipeAddInputPtr FixPipePass::AddInput9Strategy(const FixPipePassInfo &match_pass,
                                                  FixPipeFunctionParamPtr funtcparam) {
  funtcparam->SetInputName(ANTIQUANTOFFSETINPUT);
  return AddInputAntiStrategy(match_pass, funtcparam, POSTELTWISEUNIT);
}

REGISTER_PASS(FIXPIPE_PASS_NAME, SECOND_ROUND_BUILT_IN_GRAPH_PASS, FixPipePass);
}  // namespace fe
