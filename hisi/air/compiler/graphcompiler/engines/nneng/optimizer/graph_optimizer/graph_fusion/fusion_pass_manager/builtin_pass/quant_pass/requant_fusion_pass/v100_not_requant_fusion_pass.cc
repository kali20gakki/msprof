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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_not_requant_fusion_pass.h"
#include <cmath>
#include <string>
#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_requant_util.h"

namespace fe {
static const string PATTERN_DEQUANT = "dequant";
static const string PATTERN_QUANT = "quant";

void V100NotRequantFusionPass::RemoveHostOpFromFusionNode(vector<ge::NodePtr> &new_nodes) const {
  if (new_nodes.size() == 2) {
    for (auto iter = new_nodes.begin(); iter != new_nodes.end();) {
      ge::NodePtr node = *iter;
      if (node->GetOpDesc()->GetType() == REQUANT_HOST_CPU_OP) {
        iter = new_nodes.erase(iter);
      } else {
        iter++;
      }
    }
  }
}

void V100NotRequantFusionPass::RecordNotRequantOutputAnchorMap(vector<ge::NodePtr> &relu_nodes,
                                                               vector<ge::NodePtr> &fuse_nodes) {
  if (relu_nodes.empty()) {
    for (auto requant_node : fuse_nodes) {
      RecordOutputAnchorMap(requant_node);
    }
  }

  for (auto relu_node : relu_nodes) {
    if (CheckNeedRemoveRelu(relu_node)) {
      RecordOutputAnchorMap(relu_node);
    } else {
      fuse_nodes.push_back(relu_node);
    }
  }
}

Status V100NotRequantFusionPass::NotRequantPattern0(ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                                    vector<ge::NodePtr> &new_nodes, Mapping &mapping) {
  Status ret;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;
  float scale_quant = 1;
  int requanted_flag;
  unsigned int relu_del = 0;

  if (ge::AttrUtils::GetInt(dequant_node->GetOpDesc(), ATTR_REQUANTED, requanted_flag)) {
    FE_LOGD("This node [%s] has been requanted, skip it.", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  dequants.push_back(dequant_node);
  for (auto node_ptr : dequant_node->GetOutAllNodes()) {
    if (CheckReluValid(node_ptr)) {
      if (CheckNeedRemoveRelu(node_ptr)) {
        relu_del++;
      }
      relus.push_back(node_ptr);
    }
  }

  FE_LOGD("reluDel is %u, size of dequant out node %zu.", relu_del, dequant_node->GetOutAllNodes().size());
  bool del_relu_flag = (relu_del && (relu_del == dequant_node->GetOutAllNodes().size()));
  if (!del_relu_flag) {
    relus.clear();
  }
  // add mapping for datadump to get all original nodes before fusion.
  if (!relus.empty()) {
    for (size_t i = 0; i < relus.size(); i++) {
      std::shared_ptr<OpDesc> op(new (std::nothrow) OpDesc());
      FE_CHECK(op == nullptr, REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn0] create op is nullptr"),
               return FAILED);
      std::string relu_name = "relu";
      op->id = relu_name.append(to_string(i));
      mapping[op].push_back(relus[i]);
    }
  }
  new_nodes.push_back(dequant_node);

  if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn0] set relu del failed! node name: %s",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("relu size is %zu, quant size %zu, relu_del_num %d", relus.size(), dequants.size(), relu_del);
  if (DealDequantV100(graph, dequants, scale_quant, new_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn0] deal dequant nodes failed");
    return FAILED;
  }

  RecordNotRequantOutputAnchorMap(relus, new_nodes);
  ret = DealRelu(graph, relus, scale_quant);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn0] deal relu nodes failed");
    return FAILED;
  }

  quants.clear();
  ret = TagNodes(quants, dequants, 0);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn0] tag nodes failed");
    return FAILED;
  }

  /* if fusion op only contains requant and RequantHostCpuOp, RequantHostCpuOp
     will be folded finally. host op do not need to record it to fusion op. */
  RemoveHostOpFromFusionNode(new_nodes);
  return SUCCESS;
}

Status V100NotRequantFusionPass::DealQauntNotRequant(vector<ge::NodePtr> &quant_nodes) const {
  float scale;

  for (vector<ge::NodePtr>::iterator i = quant_nodes.begin(); i < quant_nodes.end(); ++i) {
    if (!ge::AttrUtils::GetFloat((*i)->GetOpDesc(), ATTR_SCALE, scale)) {
      REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DealQauntNotReqnt] Get quant scale failed!");
      return FAILED;
    }

    if (scale < pow(SCALE_BASE, SCALE_EXPONENT)) {
      scale = sqrt(scale);
      if (!ge::AttrUtils::SetFloat((*i)->GetOpDesc(), ATTR_SCALE, scale)) {
        REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DealQauntNotReqnt] Set quant scale failed!");
        return FAILED;
      }
      if (!ge::AttrUtils::SetBool((*i)->GetOpDesc(), ATTR_SQRT_MODE, true)) {
        REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DealQauntNotReqnt] Set sqrt mode failed!");
        return FAILED;
      }
    } else {
      if (!ge::AttrUtils::SetBool((*i)->GetOpDesc(), ATTR_SQRT_MODE, false)) {
        REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DealQauntNotReqnt] Set sqrt mode failed!");
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

Status V100NotRequantFusionPass::NotRequantPattern1(ge::NodePtr quant_node) const {
  Status ret;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> dequants;
  int requanted_flag;

  if (ge::AttrUtils::GetInt(quant_node->GetOpDesc(), ATTR_REQUANTED, requanted_flag)) {
    FE_LOGD("this node has been requanted, skip, node name: %s.", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }

  quants.push_back(quant_node);

  ret = DealQauntNotRequant(quants);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn1] deal quant nodes failed");
    return FAILED;
  }

  dequants.clear();
  ret = TagNodes(quants, dequants, 0);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][NotReqntPtn1] tag nodes failed");
    return FAILED;
  }

  return SUCCESS;
}

/*
 * ===========================Before Fusion===============================
 * ####pattern0-scenario1       ####pattern1-scenario2
 *
 *     dequant                         quant
 *        |
 *      relu(optional)
 *
 * ===========================After Fusion================================
 * ####pattern0-scenario1       ####pattern1-scenario2
 *
 *     dequant                         quant
 *
 */
vector<FusionPattern *> V100NotRequantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("notRequantPassPattern0");
  FE_CHECK(pattern0 == nullptr, REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  pattern0->AddOpDesc(PATTERN_DEQUANT, {ASCEND_DEQUANT}).SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern0);

  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("notRequantPassPattern1");
  FE_CHECK(pattern1 == nullptr, REPORT_FE_ERROR("[GraphOpt][V100NotReqntFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  pattern1->AddOpDesc(PATTERN_QUANT, {ASCEND_QUANT}).SetOutput(PATTERN_QUANT);

  patterns.push_back(pattern1);

  return patterns;
}

Status V100NotRequantFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr dequant_node = GetNodeFromMapping(PATTERN_DEQUANT, mapping);
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);
  if (dequant_node.get()) {
    ClearOutputAnchorMap();
    return NotRequantPattern0(graph, dequant_node, fusion_nodes, mapping);
  }
  if (quant_node.get()) {
    return NotRequantPattern1(quant_node);
  }
  return NOT_CHANGED;
}

}  // namespace fe
