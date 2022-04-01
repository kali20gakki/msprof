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
 * in Fixpipe feature, in dequant+[relu]+quant, quant node also need to be fuzed into pre unit,
 * but pre covunit has already hold by dequant,so add thispass to deal dequant+[relu]+quant
 * cube + dequant + [relu] + quant = cubt + dequant' + [relu], relu node is optional
 * the new dequantnode has 2 extra attrs :quantnode's scale and offset and dequantnode
 * descale = descale * scale
 */

#include "fixpipe_pre_quant_pass.h"
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common/aicore_util_constants.h"
#include "common/configuration.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/util/platform_info.h"
#include "fixpipe_common.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

namespace fe {
static const string FIXPIPE_PRE_QUANT_PASS_NAME = "FIXPIPEAPREQUANTFUSIONPASS";
std::vector<FusionPattern *> FixPipePreQuantPass::DefinePatterns() {
  std::vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern("fixpipeprequant");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][pattern][DefPtn] new an object failed."), return patterns);
  pattern->AddOpDesc(PATTERN_CONV, CUBE_OP_TYPE_VEC).SetOutput(PATTERN_CONV);
  patterns.push_back(pattern);
  return patterns;
}

bool FixPipePreQuantPass::ReadConfig(const std::string &soc_version) {
  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos) !=
      SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return false;
  }
  std::map<std::string, std::map<std::string, std::vector<CONFIGDTYPE>>> fixpipe_map;
  std::map<std::string, std::vector<std::string>> depends_units;
  ReadPlatFormConfig(soc_version, m_units_, depends_units, fixpipe_map);
  if ((std::find(m_units_.begin(), m_units_.end(), PRECONVUNIT) != m_units_.end()) &&
      (std::find(m_units_.begin(), m_units_.end(), PREACTUNIT) != m_units_.end())) {
    return true;
  }
  return false;
}

bool FixPipePreQuantPass::ReadConfig() {
  std::string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  return ReadConfig(soc_version);
}

Status FixPipePreQuantPass::JudgeQuantReluValue(const ge::NodePtr &dequant_node, const ge::NodePtr &relu_node,
                                                const ge::NodePtr &quant_node) const {
  if (!JudgeConstValue(dequant_node)) {
    return FAILED;
  }
  float quant_scale = 0.0;
  float offset = 0;
  (void)ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_SCALE, quant_scale);
  (void)ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_OFFSET, offset);
  if (quant_scale < 0) {
    return FAILED;
  }
  if (relu_node->GetType() == LEAKY_RELU) {
    float attr_negative_slope_a = 0.0;
    (void)ge::AttrUtils::GetFloat(relu_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
    if (attr_negative_slope_a < 0) {
      return FAILED;
    }
  }
  if (relu_node->GetType() == PRELU) {
    if (!JudgeConstValue(relu_node)) {
      return FAILED;
    }
  }
  if (relu_node->GetType() == RELU6) {
    if (quant_scale * relu6_value + offset < 0) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::Pattern0Parse(const ge::NodePtr &dequant_node, vector<ge::NodePtr> &quants,
                                          std::map<ge::NodePtr, vector<ge::NodePtr>> &relus) const {
  if (dequant_node->GetOutDataNodesSize() != 1) {
    return FAILED;
  }
  for (auto &node_ptr : dequant_node->GetOutDataNodes()) {
    if (node_ptr->GetType() == RELU || node_ptr->GetType() == LEAKY_RELU || node_ptr->GetType() == RELU6 ||
        node_ptr->GetType() == PRELU) {
      std::vector<ge::NodePtr> reluquants;
      if (node_ptr->GetOutDataNodesSize() != 1) {
        return FAILED;
      }
      auto node_next = node_ptr->GetOutDataNodes().at(0);
      if (node_next == nullptr) {
        continue;
      }
      if (node_next->GetOutDataNodesSize() != 1) {
        return FAILED;
      }
      if (node_next->GetType() != ASCEND_QUANT) {
        continue;
      }
      FE_LOGD("next node = quant node's name = %s", node_next->GetName().c_str());
      if (JudgeQuantReluValue(dequant_node, node_ptr, node_next) != SUCCESS) {
        return FAILED;
      }
      reluquants.push_back(node_next);
      relus.emplace(make_pair(node_ptr, reluquants));
    } else if (node_ptr->GetType() == ASCEND_QUANT) {
      quants.push_back(node_ptr);
    }
  }
  return SUCCESS;
}

ge::NodePtr FixPipePreQuantPass::DealDequantInput(ge::OpDescPtr op_desc_tmp, ge::ComputeGraph &graph,
                                                  const ge::NodePtr &dequant, const ge::NodePtr &quant,
                                                  vector<ge::NodePtr> &new_nodes) const {
  float scale_tmp = 0.0f;
  (void)ge::AttrUtils::GetFloat(quant->GetOpDesc(), ATTR_SCALE, scale_tmp);
  float offset_a = 0.0f;
  (void)ge::AttrUtils::GetFloat(quant->GetOpDesc(), ATTR_OFFSET, offset_a);
  if (HasInput1Node(dequant) != SUCCESS) {
    return nullptr;
  }
  ge::OpDescPtr newopdesc = nullptr;
  FE_MAKE_SHARED(newopdesc = std::make_shared<ge::OpDesc>("VectorMulScalar" + dequant->GetName(), "VectorMulScalar"),
                 return nullptr);
  (void)ge::AttrUtils::SetBool(newopdesc, "_is_single_op", ATTRTRUE);
  ge::GeTensorDesc cur_tensor_desc = dequant->GetOpDesc()->MutableInputDesc(1)->Clone();
  cur_tensor_desc.SetDataType(ge::DT_FLOAT);
  cur_tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  newopdesc->AddOutputDesc("y", cur_tensor_desc);
  newopdesc->AddInputDesc(X1INPUTNAME, *(dequant->GetOpDesc()->MutableInputDesc(1)));
  (void)ge::AttrUtils::SetFloat(newopdesc, ATTR_SCALA, static_cast<float>(scale_tmp));
  ge::NodePtr newopnode = graph.AddNode(newopdesc);
  op_desc_tmp->MutableInputDesc(1)->SetDataType(ge::DT_FLOAT);
  op_desc_tmp->MutableInputDesc(1)->SetOriginDataType(ge::DT_FLOAT);
  op_desc_tmp->MutableOutputDesc(0)->SetDataType(quant->GetOpDesc()->MutableOutputDesc(0)->GetDataType());
  op_desc_tmp->MutableOutputDesc(0)->SetOriginDataType(quant->GetOpDesc()->MutableOutputDesc(0)->GetDataType());
  op_desc_tmp->MutableOutputDesc(0)->SetShape(quant->GetOpDesc()->MutableOutputDesc(0)->MutableShape());
  FE_LOGD("scale = %f offset = %f", scale_tmp, offset_a);
  (void)ge::AttrUtils::SetFloat(op_desc_tmp, ATTR_OFFSET, offset_a);
  (void)ge::AttrUtils::SetFloat(op_desc_tmp, ATTR_SCALE, scale_tmp);
  auto enhanceddequantnode = graph.AddNode(op_desc_tmp);
  auto inputnode = dequant->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  if (!AddEdges(inputnode->GetOutDataAnchor(0), newopnode->GetInDataAnchor(0), inputnode, newopnode)) {
    return nullptr;
  }
  if (!AddEdges(newopnode->GetOutDataAnchor(0), enhanceddequantnode->GetInDataAnchor(1), newopnode,
                enhanceddequantnode)) {
    return nullptr;
  }
  new_nodes.push_back(newopnode);
  new_nodes.push_back(enhanceddequantnode);
  return enhanceddequantnode;
}

Status FixPipePreQuantPass::RelinkReluWithQuant(ge::NodePtr head_node, ge::NodePtr enhanceddequantnode,
                                                ge::NodePtr new_relu_node, ge::NodePtr relu_node,
                                                ge::NodePtr quant_node) const {
  if (!AddEdges(head_node->GetOutDataAnchor(0), enhanceddequantnode->GetInDataAnchor(0), head_node,
                enhanceddequantnode) ||
      !AddEdges(enhanceddequantnode->GetOutDataAnchor(0), new_relu_node->GetInDataAnchor(0), enhanceddequantnode,
                new_relu_node)) {
    return FAILED;
  }
  if (relu_node->GetType() == PRELU) {
    if (!AddEdges(relu_node->GetInDataAnchor(1)->GetPeerOutAnchor(), new_relu_node->GetInDataAnchor(1), relu_node,
                  new_relu_node)) {
      return FAILED;
    }
    if (HasEdge(relu_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetOutControlAnchor(),
                relu_node->GetInControlAnchor())) {
      if (!AddEdges(relu_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode()->GetOutControlAnchor(),
                    new_relu_node->GetInControlAnchor(), relu_node, new_relu_node)) {
        return FAILED;
      }
    }
  }
  RemoveAnchor(relu_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  RemoveAnchor(relu_node->GetOutControlAnchor(), quant_node->GetInControlAnchor());
  if (quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 0) {
    FE_LOGI("The size of node is [%zu].", quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size());
    for (ge::InDataAnchorPtr &inAnchorPtr : quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors()) {
      inAnchorPtr->UnlinkAll();
      FE_CHECK(ge::GraphUtils::AddEdge(new_relu_node->GetOutDataAnchor(0), inAnchorPtr) != SUCCESS,
               FE_LOGE("Add edge from fused node:%s's index to fusion node:%s's 1st index failed.",
                       quant_node->GetName().c_str(), new_relu_node->GetName().c_str()),
               return FAILED);
      FE_LOGD("Add edge from fused node:%s's 1st index to fusion node:%s's 1st ", quant_node->GetName().c_str(),
              new_relu_node->GetName().c_str());
    }
  }
  if (quant_node->GetOutControlAnchor()->GetPeerInControlAnchors().size() > 0) {
    FE_LOGI("The size of node is [%zu].", quant_node->GetOutControlAnchor()->GetPeerInControlAnchors().size());
    for (ge::InControlAnchorPtr &inAnchorPtr : quant_node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
      inAnchorPtr->UnlinkAll();
      FE_CHECK(ge::GraphUtils::AddEdge(new_relu_node->GetOutControlAnchor(), inAnchorPtr) != SUCCESS,
               FE_LOGE("Add edge from fused node:%s's index to fusion node:%s's 1st index failed.",
                       quant_node->GetName().c_str(), new_relu_node->GetName().c_str()),
               return FAILED);
      FE_LOGD("Add edge from fused node:%s's 1st index to fusion node:%s's 1st ", quant_node->GetName().c_str(),
              new_relu_node->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::RelinkDequantWithQuant(ge::NodePtr head_node, ge::NodePtr enhanceddequantnode,
                                                   ge::NodePtr dequant_node, ge::NodePtr quant_node) const {
  if (!AddEdges(head_node->GetOutDataAnchor(0), enhanceddequantnode->GetInDataAnchor(0), head_node,
                enhanceddequantnode)) {
    return FAILED;
  }
  if (HasEdge(head_node->GetOutControlAnchor(), dequant_node->GetInControlAnchor())) {
    if (!AddEdges(head_node->GetOutControlAnchor(), enhanceddequantnode->GetInControlAnchor(), head_node,
                  enhanceddequantnode)) {
      return FAILED;
    }
  }
  RemoveAnchor(dequant_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  RemoveAnchor(dequant_node->GetOutControlAnchor(), quant_node->GetInControlAnchor());
  if (quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 0) {
    FE_LOGI("The size of node is [%zu].", quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size());
    for (ge::InDataAnchorPtr &inAnchorPtr : quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors()) {
      inAnchorPtr->UnlinkAll();
      FE_CHECK(ge::GraphUtils::AddEdge(enhanceddequantnode->GetOutDataAnchor(0), inAnchorPtr) != SUCCESS,
               FE_LOGE("Add edge from fused node: %s's index to fusion "
                       "node:%s's 1st index failed.",
                       quant_node->GetName().c_str(), enhanceddequantnode->GetName().c_str()),
               return FAILED);
      FE_LOGD("Add edge from fused node: %s's index to fusion node: %s's 1st", quant_node->GetName().c_str(),
              enhanceddequantnode->GetName().c_str());
    }
  }
  if (quant_node->GetOutControlAnchor()->GetPeerInControlAnchors().size() > 0) {
    FE_LOGI("The size of node is [%zu].", quant_node->GetOutControlAnchor()->GetPeerInControlAnchors().size());
    for (ge::InControlAnchorPtr &inAnchorPtr : quant_node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
      inAnchorPtr->UnlinkAll();
      FE_CHECK(ge::GraphUtils::AddEdge(enhanceddequantnode->GetOutControlAnchor(), inAnchorPtr) != SUCCESS,
               FE_LOGE("Add edge from fused node: %s's index to fusion "
                       "node:%s's 1st index failed.",
                       quant_node->GetName().c_str(), enhanceddequantnode->GetName().c_str()),
               return FAILED);
      FE_LOGD("Add edge from fused node: %s's index to fusion node: %s's 1st", quant_node->GetName().c_str(),
              enhanceddequantnode->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::DealDequant(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                        vector<ge::NodePtr> &quants, std::map<ge::NodePtr, vector<ge::NodePtr>> &relus,
                                        vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &new_nodes) const {
  ge::OpDescPtr fuzedesc = dequant_node->GetOpDesc();
  uint32_t i = 0;
  for (auto &quant_node : quants) {
    ge::OpDescPtr op_desc_tmp = ge::AttrUtils::CloneOpDesc(fuzedesc);
    if (op_desc_tmp == nullptr) {
      return FAILED;
    }
    op_desc_tmp->SetName(fuzedesc->GetName() + "_requant_" + std::to_string(i++));
    ge::NodePtr enhanceddequantnode = DealDequantInput(op_desc_tmp, graph, dequant_node, quant_node, new_nodes);
    if (enhanceddequantnode == nullptr) {
      return FAILED;
    }
    RelinkDequantWithQuant(head_node, enhanceddequantnode, dequant_node, quant_node);
    dequants.push_back(enhanceddequantnode);
  }
  for (auto iter = relus.begin(); iter != relus.end(); iter++) {
    ge::NodePtr relu_node = iter->first;
    std::vector<ge::NodePtr> &quantnodes = iter->second;
    for (auto &quant_node : quantnodes) {
      ge::OpDescPtr op_desc_tmp = ge::AttrUtils::CloneOpDesc(fuzedesc);
      if (op_desc_tmp == nullptr) {
        return FAILED;
      }
      op_desc_tmp->SetName(fuzedesc->GetName() + "_requant_" + std::to_string(i++));
      ge::OpDescPtr op_relu_desc_tmp = ge::AttrUtils::CloneOpDesc(relu_node->GetOpDesc());
      if (op_relu_desc_tmp == nullptr) {
        return FAILED;
      }
      op_relu_desc_tmp->SetName(relu_node->GetName() + "_requant_" + std::to_string(i++));
      op_relu_desc_tmp->MutableOutputDesc(0)->SetDataType(quant_node->GetOpDesc()->MutableOutputDesc(0)->GetDataType());
      op_relu_desc_tmp->MutableOutputDesc(0)->SetOriginDataType(
          quant_node->GetOpDesc()->MutableOutputDesc(0)->GetDataType());

      op_relu_desc_tmp->MutableOutputDesc(0)->SetShape(quant_node->GetOpDesc()->MutableOutputDesc(0)->MutableShape());
      ge::NodePtr enhanceddequantnode = DealDequantInput(op_desc_tmp, graph, dequant_node, quant_node, new_nodes);
      if (enhanceddequantnode == nullptr) {
        return FAILED;
      }
      ge::NodePtr new_relunode = graph.AddNode(op_relu_desc_tmp);
      RelinkReluWithQuant(head_node, enhanceddequantnode, new_relunode, relu_node, quant_node);
      new_nodes.push_back(new_relunode);
      dequants.push_back(enhanceddequantnode);
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::DeleteNode(ge::NodePtr todeletenode) const {
  if (ge::GraphUtils::IsolateNode(todeletenode, {}) != SUCCESS) {
    return FAILED;
  }
  if (ge::GraphUtils::RemoveNodeWithoutRelink(todeletenode->GetOwnerComputeGraph(), todeletenode) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::DealQuant(vector<ge::NodePtr> &quant_nodes) const {
  if (quant_nodes.empty()) {
    FE_LOGD("quant_nodes is empty");
    return SUCCESS;
  }
  bool result = std::any_of(quant_nodes.begin(), quant_nodes.end(),
                            [&](ge::NodePtr node) { return (DeleteNode(node) == SUCCESS); });
  FE_LOGD("result = %d", result);
  return result ? SUCCESS : FAILED;
}

Status FixPipePreQuantPass::DealRelus(ge::NodePtr &dequant_node,
                                      std::map<ge::NodePtr, vector<ge::NodePtr>> &relus) const {
  for (auto iter = relus.begin(); iter != relus.end(); iter++) {
    ge::NodePtr relu_node = iter->first;
    std::vector<ge::NodePtr> &quantnodes = iter->second;
    if (relu_node->GetOutDataNodesSize() == 0) {
      RemoveAnchor(dequant_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
      RemoveAnchor(dequant_node->GetOutControlAnchor(), relu_node->GetInControlAnchor());
    }
    bool result = std::any_of(quantnodes.begin(), quantnodes.end(),
                              [&](ge::NodePtr node) { return DeleteNode(node) == SUCCESS; });
    if (!result) {
      return FAILED;
    }
    if (relu_node->GetOutDataNodesSize() == 0) {
      if (DeleteNode(relu_node) != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::DeleteDequant(ge::NodePtr dequant_node) const {
  if (dequant_node->GetOutDataNodesSize() == 0) {
    if (DeleteNode(dequant_node) != SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::ReDequantProcess(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                             vector<ge::NodePtr> &quants,
                                             std::map<ge::NodePtr, vector<ge::NodePtr>> &relus,
                                             vector<ge::NodePtr> &new_nodes) const {
  Status ret;
  FE_LOGD("Start to do general requant fusion pass.");
  std::vector<ge::NodePtr> dequants;
  ret = DealDequant(head_node, graph, dequant_node, quants, relus, dequants, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][fixpipeprequant][ReqntPcs] deal dequant nodes failed");
    return FAILED;
  }

  ret = DealRelus(dequant_node, relus);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][fixpipeprequant][ReqntPcs] deal relu nodes failed");
    return FAILED;
  }
  (void)DeleteDequant(dequant_node);
  ret = DealQuant(quants);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][fixpipeprequant][ReqntPcs] deal quant nodes failed");
    return FAILED;
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::HandlePattern0(ge::NodePtr head_node, ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                           vector<ge::NodePtr> &new_nodes) const {
  vector<ge::NodePtr> quants;
  std::map<ge::NodePtr, vector<ge::NodePtr>> relus;
  if (Pattern0Parse(dequant_node, quants, relus) != SUCCESS) {
    FE_LOGD("Do not need do requant pattern0 here, dequant name [%s]", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  Status ret = ReDequantProcess(head_node, graph, dequant_node, quants, relus, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][fixpipeprequant][HdlPtn0] requant fusion pass fail, dequant node [%s].",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::FusionImpl(ge::NodePtr head_node, ge::ComputeGraph &graph,
                                       std::vector<ge::NodePtr> &new_nodes) const {
  if (head_node->GetOutDataNodesSize() != 1) {
    return NOT_CHANGED;
  }
  for (auto node_output : head_node->GetOutAllNodes()) {
    if (node_output->GetType() == ASCEND_DEQUANT) {
      Status ret = HandlePattern0(head_node, graph, node_output, new_nodes);
      if (ret == FAILED) {
        return NOT_CHANGED;
      }
    }
  }
  if (new_nodes.empty()) {
    return NOT_CHANGED;
  }
  return SUCCESS;
}

Status FixPipePreQuantPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, std::vector<ge::NodePtr> &new_nodes) {
  if (!ReadConfig()) {
    return NOT_CHANGED;
  }
  ge::NodePtr conv_node = GetNodeFromMapping(PATTERN_CONV, mapping);
  return FusionImpl(conv_node, graph, new_nodes);
}
REGISTER_PASS(FIXPIPE_PRE_QUANT_PASS_NAME, SECOND_ROUND_BUILT_IN_GRAPH_PASS, FixPipePreQuantPass);
}  // namespace fe
