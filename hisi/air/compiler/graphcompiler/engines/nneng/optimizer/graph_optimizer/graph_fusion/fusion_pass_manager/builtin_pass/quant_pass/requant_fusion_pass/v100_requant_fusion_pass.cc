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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_requant_fusion_pass.h"
#include <cmath>
#include <string>
#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_requant_util.h"
#include "common/configuration.h"

namespace fe {
static const string PATTERN_DEQUANT = "dequant";
static const string PATTERN_QUANT = "quant";

bool V100RequantFusionPass::CheckISAArchVersionWithQuantType(ge::DataType quant_type) const {
  ISAArchVersion isa_arch_version = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  return (isa_arch_version == ISAArchVersion::EN_ISA_ARCH_V100 ||
          (isa_arch_version == ISAArchVersion::EN_ISA_ARCH_V200 && quant_type == ge::DT_INT4));
}

const float kFloatEsp = 10e-6;
Status V100RequantFusionPass::DealQuant(vector<ge::NodePtr> &quant_nodes) const {
  for (vector<ge::NodePtr>::iterator i = quant_nodes.begin(); i < quant_nodes.end(); ++i) {
    // set origin scale to quant node
    float origin_scale;
    if (!ge::AttrUtils::GetFloat((*i)->GetOpDesc(), ATTR_SCALE, origin_scale)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DealQnt] get quant scale failed!");
      return FAILED;
    }
    if (!ge::AttrUtils::SetFloat((*i)->GetOpDesc(), ATTR_ORIGIN_SCALE, origin_scale)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DealQnt] set quant origin scale failed!");
      return FAILED;
    }
    if (!ge::AttrUtils::SetFloat((*i)->GetOpDesc(), ATTR_SCALE, 1)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DealQnt] set quant scale failed!");
      return FAILED;
    }
    if (!ge::AttrUtils::SetBool((*i)->GetOpDesc(), ATTR_SQRT_MODE, false)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DealQnt] set quant sort mode failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V100RequantFusionPass::CheckQuantOp(const ge::NodePtr &node_ptr, vector<ge::NodePtr> &quants,
                                           int &quant_num,
                                           float &scale_base) const {
  auto quant_type = node_ptr->GetOpDesc()->GetOutputDesc(0).GetDataType();
  if (!CheckISAArchVersionWithQuantType(quant_type)) {
    FE_LOGD("quant op[%s] adopts v200 8bit quantification, go v200 branch.", node_ptr->GetName().c_str());
    return NOT_CHANGED;
  }
  quant_num++;
  quants.push_back(node_ptr);

  float scale_tmp = 0;
  if (!ge::AttrUtils::GetFloat(node_ptr->GetOpDesc(), ATTR_SCALE, scale_tmp)) {
    FE_LOGW("Get scale attr of quant node[%s] failed!", node_ptr->GetName().c_str());
    return NOT_CHANGED;
  }
  // judge whether all scale equal
  if (fabs(scale_base) < kFloatEsp) {
    scale_base = scale_tmp;
  } else if (fabs(scale_tmp - scale_base) > kFloatEsp) {
    FE_LOGW("The scale attr of two quant nodes are not equal!");
    return NOT_CHANGED;
  }
  return SUCCESS;
}

Status V100RequantFusionPass::Pattern0Parse(ge::NodePtr dequant_node,
                                            vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &quants,
                                            vector<ge::NodePtr> &relus, float &scale_quant) const {
  bool has_relu = false;
  float scale_base = 0;
  int relu_num = 0;
  int quant_num = 0;
  int relu_del = 0;
  bool cond = false;
  bool del_relu_flag = false;

  int direct_out_node_num = dequant_node->GetOutAllNodes().size();

  for (auto node_ptr : dequant_node->GetOutAllNodes()) {
    if (CheckReluValid(node_ptr)) {
      if (CheckNeedRemoveRelu(node_ptr)) {
        relu_del++;
      }
      has_relu = true;
      relu_num++;
      relus.push_back(node_ptr);
      for (auto node_next : node_ptr->GetOutAllNodes()) {
        if (node_next->GetType() != ASCEND_QUANT) {
          FE_LOGW("next node[%s] is not quant", node_next->GetName().c_str());
          return NOT_CHANGED;
        }
        if (CheckQuantOp(node_next, quants, quant_num, scale_base) != SUCCESS) {
          FE_LOGW("Check quant op[%s] failed.", node_next->GetName().c_str());
          return NOT_CHANGED;
        }
      }

    } else if (node_ptr->GetType() == ASCEND_QUANT) {
      if (CheckQuantOp(node_ptr, quants, quant_num, scale_base) != SUCCESS) {
        FE_LOGW("Check quant op[%s] failed.", node_ptr->GetName().c_str());
        return NOT_CHANGED;
      }
    } else {
      return NOT_CHANGED;
    }
  }

  cond = ((has_relu && relu_num != direct_out_node_num) || (has_relu && relu_num != 1 && relu_num != quant_num) ||
          (!has_relu && quant_num != direct_out_node_num) || (quant_num == 0) ||
          (relu_del && relu_del != direct_out_node_num));
  if (cond) {
    return NOT_CHANGED;
  }
  del_relu_flag = relu_del ? true : false;
  if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][Ptn0Parse] Set relu del failed! node name: %s",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  dequants.push_back(dequant_node);
  scale_quant = scale_base;
  return SUCCESS;
}

Status V100RequantFusionPass::HandlePattern0(ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                             vector<ge::NodePtr> &new_nodes) const {
  /*
        0. parse graph for pattern0, judge and get information for
                                 whether has relu
                                 whether scale attrs of all quant nodes are same
                                 get quant nodes' nums
        1. deal dequant node's scale input
        2. deal quant node's scale attr
        3. delete relu op
   */
  Status ret;
  float scale_quant;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;

  if (Pattern0Parse(dequant_node, dequants, quants, relus, scale_quant) != SUCCESS) {
    FE_LOGD("Do not need do requant pattern0 here, dequant name[%s]", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }

  FE_LOGD("Size of relu node size: %zu, size of quant node is %zu", relus.size(), quants.size());
  ret = DealDequantV100(graph, dequants, scale_quant, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn0] deal dequant nodes failed");
    return FAILED;
  }

  ret = DealQuant(quants);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn0] deal quant nodes failed");
    return FAILED;
  }

  ret = DealRelu(graph, relus, scale_quant);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn0] deal relu nodes failed");
    return FAILED;
  }

  if (TagNodes(quants, dequants, 1) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn0] tag nodes failed");
    return FAILED;
  }

  return SUCCESS;
}

bool V100RequantFusionPass::IsConstToAttrInput(const ge::NodePtr &concat_node,
                                               const ge::NodePtr &const_node) const {
  auto peerInDataAnchors = const_node->GetOutDataAnchor(0)->GetPeerInDataAnchors();
  if (peerInDataAnchors.empty()) {
    FE_LOGW("Const node [%s] peer in data anchor is empty.", const_node->GetName().c_str());
    return false;
  }
  uint32_t index = peerInDataAnchors.at(0)->GetIdx();
  if (concat_node->GetType() == CONCAT && index == 0) {
    FE_LOGD("Const node [%s] can convert to attr.", const_node->GetName().c_str());
    return true;
  }
  if (concat_node->GetType() == CONCATV2 && (index == concat_node->GetAllInDataAnchorsSize() - 1)) {
    FE_LOGD("Const node [%s] can convert to attr.", const_node->GetName().c_str());
    return true;
  }
  return false;
}

Status V100RequantFusionPass::CheckConcatOpInput(const ge::NodePtr &concat_node,
                                                 vector<ge::NodePtr> &dequants,
                                                 vector<ge::NodePtr> &relus) const {
  for (auto node_next : concat_node->GetInAllNodes()) {
    if (CheckReluValid(node_next)) {
      relus.push_back(node_next);
      auto input_node = node_next->GetInAllNodes().at(0);
      if (input_node->GetType() != ASCEND_DEQUANT || node_next->GetInAllNodes().size() != 1) {
        FE_LOGD("next-next node != dequant, not need do requant");
        return FAILED;
      }
      bool del_relu_flag = CheckNeedRemoveRelu(node_next);
      if (!ge::AttrUtils::SetBool(input_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
        REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][ChkConcatOpIn] set relu del failed! node name: %s",
                        input_node->GetName().c_str());
        return FAILED;
      }
      dequants.push_back(input_node);
    } else if (node_next->GetType() == ASCEND_DEQUANT) {
      dequants.push_back(node_next);
    } else if (node_next->GetType() == CONSTANT) {
      if (!IsConstToAttrInput(concat_node, node_next)) {
        FE_LOGD("const node [%s] can not convert to attr.", node_next->GetName().c_str());
        return FAILED;
      }
    } else {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V100RequantFusionPass::CHeckReluOpInput(const ge::NodePtr &node_ptr, vector<ge::NodePtr> &dequants,
                                               vector<ge::NodePtr> &relus) const {
  for (auto node_next : node_ptr->GetInAllNodes()) {
    if (!IsValidConcatNode(node_next)) {
      FE_LOGD("next node[%s], type[%s] is not concat, no need to do requant", node_next->GetName().c_str(),
              node_next->GetType().c_str());
      return NOT_CHANGED;
    }
    if (node_next->GetOutAllNodes().size() != 1) {
      FE_LOGD("concat's out num should be one.");
      return NOT_CHANGED;
    }
    for (auto input_node : node_next->GetInAllNodes()) {
      if (input_node->GetType() == ASCEND_DEQUANT) {
        bool del_relu_flag = CheckNeedRemoveRelu(node_ptr);
        FE_LOGD("delete relu nodes flag: [%d].", del_relu_flag);
        if (!ge::AttrUtils::SetBool(input_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
          FE_LOGW("set relu del failed! node name: %s", input_node->GetName().c_str());
          return FAILED;
        }
        dequants.push_back(input_node);
      } else if (CheckReluValid(input_node)) {
        relus.push_back(input_node);
        auto dequant_node = input_node->GetInAllNodes().at(0);
        if (dequant_node->GetType() != ASCEND_DEQUANT || input_node->GetInAllNodes().size() != 1) {
          FE_LOGD("next-next node is not dequant, don't need do requant");
          return NOT_CHANGED;
        }
        bool del_relu_flag = CheckNeedRemoveRelu(input_node);
        FE_LOGD_IF(del_relu_flag, "The flag value of _need_delrelu_of_dequant is true, node[%s]",
                   dequant_node->GetName().c_str());
        if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
          FE_LOGW("set relu del failed! node name: %s", dequant_node->GetName().c_str());
          return FAILED;
        }
        dequants.push_back(dequant_node);
      } else if (input_node->GetType() == CONSTANT) {
        if (!IsConstToAttrInput(node_next, input_node)) {
          FE_LOGD("const node [%s] can not convert to attr.", node_next->GetName().c_str());
          return NOT_CHANGED;
        }
      } else {
        return NOT_CHANGED;
      }
    }
  }
  return SUCCESS;
}

Status V100RequantFusionPass::Pattern1Parse(ge::NodePtr quant_node,
                                            vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &quants,
                                            vector<ge::NodePtr> &relus, float &scale_quant) const {
  for (auto node_ptr : quant_node->GetInAllNodes()) {
    if (IsValidConcatNode(node_ptr)) {
      if (node_ptr->GetOutAllNodes().size() != 1) {
        FE_LOGD("concat's out num should be one.");
        return NOT_CHANGED;
      }
      if (CheckConcatOpInput(node_ptr, dequants, relus) != SUCCESS) {
        FE_LOGD("Pattern1 not all matched, quant_node node[%s].", quant_node->GetName().c_str());
        return NOT_CHANGED;
      }
    } else if (CheckReluValid(node_ptr)) {
      relus.push_back(node_ptr);
      if (CHeckReluOpInput(node_ptr, dequants, relus) != SUCCESS) {
        FE_LOGD("Pattern1 not all matched, quant node[%s].", quant_node->GetName().c_str());
        return NOT_CHANGED;
      }
    } else {
      return NOT_CHANGED;
    }
  }

  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_SCALE, scale_quant)) {
    FE_LOGW("Can not find scale attr of node[%s].", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  quants.push_back(quant_node);

  return SUCCESS;
}

/*
     0. parse graph for pattern0, judge and get information for
                              get all relu
                              get all dequant
                              get all quant
     1. deal dequant node's scale input
     2. deal quant node's scale attr
     3. delete relu op
*/
Status V100RequantFusionPass::HandlePattern1(ge::ComputeGraph &graph, ge::NodePtr quant_node,
                                             vector<ge::NodePtr> &new_nodes) const {
  Status ret;
  float scale_quant = 1;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;
  auto quant_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  if (!CheckISAArchVersionWithQuantType(quant_type)) {
    FE_LOGD("quant op[%s] adopts v200 8bit quantification, go v200 branch.", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ret = Pattern1Parse(quant_node, dequants, quants, relus, scale_quant);
  if (ret != SUCCESS) {
    FE_LOGD("Do not need do requant pattern1 for node[%s]", quant_node->GetName().c_str());
    return SUCCESS;
  }

  ret = DealDequantV100(graph, dequants, scale_quant, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn1] deal dequant nodes failed");
    return FAILED;
  }

  ret = DealQuant(quants);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn1] deal quant nodes failed");
    return FAILED;
  }

  ret = DealRelu(graph, relus, scale_quant);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn1] deal relu nodes failed");
    return FAILED;
  }

  if (TagNodes(quants, dequants, 1) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][HdlPtn1] tag nodes failed");
    return FAILED;
  }

  return SUCCESS;
}

/*
 * ===========================Before Fusion===============================
 * ------------------pattern0-------------------
 * ####pattern0-scenario1       ####pattern0-scenario2
 *
 *     dequant                         dequant
 *        |                         |     |     |
 *      relu(optional)             relu relu  relu   (optional)
 *        |                         |     |     |
 * qaunt quant quant               qaunt quant quant
 *
 *
 * ------------------pattern1-------------------
 * ####pattern1-scenario1                      ####pattern1-scenario2
 *
 * dequant dequant dequant                 dequant dequant dequant
 *    |       |       |                               |
 *  relu     relu    relu   (optional)             concat
 *            |                                       |
 *         concat                                    relu     (optional)
 *            |                                       |
 *          qaunt                                   qaunt
 *
 * ===========================After Fusion================================
 * ------------------pattern0-------------------
 *  * ####pattern0-scenario1       ####pattern0-scenario2
 *
 *     dequant                         dequant
 *        |                         |     |     |
 * qaunt quant quant               qaunt quant quant
 *
 *
 * ------------------pattern1-------------------
 * ####pattern1-scenario1                      ####pattern1-scenario2
 *
 * dequant dequant dequant                 dequant dequant dequant
 *    |       |       |                       |       |       |
 *         concat                                   concat
 *            |                                       |
 *          qaunt                                   qaunt
 *
 */
vector<FusionPattern *> V100RequantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("requantPassPattern0");
  FE_CHECK(pattern0 == nullptr, REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  pattern0->AddOpDesc(PATTERN_DEQUANT, {ASCEND_DEQUANT}).SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern0);

  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("requantPassPattern1");
  FE_CHECK(pattern1 == nullptr, REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  pattern1->AddOpDesc(PATTERN_QUANT, {ASCEND_QUANT}).SetOutput(PATTERN_QUANT);

  patterns.push_back(pattern1);
  return patterns;
}

Status V100RequantFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr dequant_node = GetNodeFromMapping(PATTERN_DEQUANT, mapping);
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);

  if (dequant_node.get()) {
    return HandlePattern0(graph, dequant_node, fusion_nodes);
  }
  if (quant_node.get()) {
    return HandlePattern1(graph, quant_node, fusion_nodes);
  }
  return NOT_CHANGED;
}

}  // namespace fe
