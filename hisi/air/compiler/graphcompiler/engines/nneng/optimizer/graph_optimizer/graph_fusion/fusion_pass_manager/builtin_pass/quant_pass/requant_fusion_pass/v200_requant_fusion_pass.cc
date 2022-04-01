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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_fusion_pass.h"
#include <cmath>
#include <string>
#include <vector>
#include "securec.h"
#include "common/configuration.h"
#include "common/math_util.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_util.h"

namespace fe {

static const string PATTERN_DEQUANT = "dequant";
static const string PATTERN_QUANT   = "quant";
static const string ASCENDREQUANT   = "AscendRequant";
static const int32_t MAX_INT9_VALUE = 255;
static const int32_t MIN_INT9_VALUE = -256;
static const uint32_t SECONDE_OPDESC_INDEX = 2;
const float kFloatEsp = 10e-6;

ge::NodePtr V200RequantFusionPass::GetFirstNoConstInput(const ge::NodePtr &node_ptr) const {
  if (node_ptr->GetInAllNodes().empty()) {
    return nullptr;
  }

  if (node_ptr->GetInAllNodes().at(0)->GetType() == CONSTANT) {
    return node_ptr->GetInAllNodes().at(1);
  }
  return node_ptr->GetInAllNodes().at(0);
}

// judge not to remove leakyrelu node
bool V200RequantFusionPass::NotRemoveLeakyRelu(ge::NodePtr node) const {
  if (node == nullptr) {
    return false;
  }
  if (node->GetType() == LEAKY_RELU && !CheckNeedRemoveRelu(node)) {
    return true;
  }
  return false;
}

Status V200RequantFusionPass::CheckConcatInputNode(const ge::NodePtr &concat_node) const {
  for (auto in_node : concat_node->GetInDataNodes()) {
    if (in_node->GetType() != CONSTANT && in_node->GetType() != CONV2D) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::SetRequantReluFlag(const ge::NodePtr &requant_node,
                                                 const int8_t &offset_quant,
                                                 const bool &relu_flag) const {
  if (relu_flag && offset_quant == -128) {
    if (!ge::AttrUtils::SetBool(requant_node->GetOpDesc(), ATTR_RELU_FLAG, false)) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][SetReqntReluFlag] set relu flag false failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::GetBiasValue(const ge::NodePtr &dequant_node, const string &quant_mode,
                                           vector<ge::NodePtr> &cube_nodes, int32_t &bias_size, bool &del_bias_flag,
                                           vector<int32_t> &bias_value) const {
  int32_t *bias = nullptr;
  auto in_node = dequant_node->GetInAllNodes().at(0);
  if (in_node->GetType() == CONCAT || in_node->GetType() == CONCATV2) {
    if (CheckConcatInputNode(in_node) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][GetBiasVal] Concat node[%s] input node is not valid.",
                      in_node->GetName().c_str());
      return FAILED;
    }
    for (auto node : in_node->GetInDataNodes()) {
      // skip const node
      if (node->GetType() == CONSTANT) {
        continue;
      }
      del_bias_flag = quant_mode == STR_QUANT_HIGH_PERFORMANCE && node->GetInAllNodes().size() >= 3;
      cube_nodes.push_back(node);

      if (del_bias_flag) {
        /* get original int32 bias */
        ge::NodePtr hostNode = node->GetInAllNodes().at(2);
        if (hostNode->GetType() != QUANT_BIAS_OPTIMIZATION) {
          REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][GetBiasVal] cube node [%s] has bias, but not do bias optimize.",
                          node->GetName().c_str());
          return FAILED;
        }
        vector<ge::GeTensorPtr> weights_hostop = ge::OpDescUtils::MutableWeights(hostNode);
        if (weights_hostop.empty()) {
          REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][GetBiasVal] Get weight failed, node[%s].",
                          hostNode->GetName().c_str());
          return FAILED;
        }
        // In host op, bias is first input
        ge::GeTensorPtr biasPtr = weights_hostop[0];
        std::uint8_t *data = const_cast<uint8_t *>(biasPtr->GetData().data());
        bias = reinterpret_cast<int32_t *>(data);
        int32_t bias_size_tmp = biasPtr->GetData().size() / sizeof(int32_t);
        bias_size += bias_size_tmp;
        for (auto index = 0; index < bias_size_tmp; index++) {
          bias_value.push_back(bias[index]);
        }
      }
    }
  } else {
    FE_LOGD("Name of cube node is: %s.", in_node->GetName().c_str());
    // cube node hava bias
    del_bias_flag = quant_mode == STR_QUANT_HIGH_PERFORMANCE && in_node->GetInAllNodes().size() >= 3;
    cube_nodes.push_back(in_node);

    if (del_bias_flag) {
      /* get original int32 bias */
      ge::NodePtr hostNode = in_node->GetInAllNodes().at(2);
      if (hostNode->GetType() != QUANT_BIAS_OPTIMIZATION) {
        REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][GetBiasVal] cube node [%s] has bias, but not do bias optimize.",
                        in_node->GetName().c_str());
        return FAILED;
      }
      vector<ge::GeTensorPtr> weights_hostop = ge::OpDescUtils::MutableWeights(hostNode);
      if (weights_hostop.empty()) {
        REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][GetBiasVal] Get weight failed, node[%s].",
                        hostNode->GetName().c_str());
        return FAILED;
      }
      // In host op, bias is first input
      ge::GeTensorPtr biasPtr = weights_hostop[0];
      uint8_t *data = const_cast<uint8_t *>(biasPtr->GetData().data());
      bias = reinterpret_cast<int32_t *>(data);
      bias_size = biasPtr->GetData().size() / sizeof(int32_t);
      for (auto index = 0; index < bias_size; index++) {
        bias_value.push_back(bias[index]);
      }
    }
  }
  return SUCCESS;
}

void V200RequantFusionPass::DealWithCubeNodes(ge::ComputeGraph &graph, vector<ge::NodePtr> &cube_nodes,
                                              const bool &del_bias_flag, const bool &no_bias_s9_flag) const {
  if (!no_bias_s9_flag && del_bias_flag) {
    for (auto cube_node : cube_nodes) {
      auto in_nodes = cube_node->GetInAllNodes();
      if (in_nodes.size() <= 2) {
        REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealCubeNd] node %s in nodes is less than 2.",
                        cube_node->GetName().c_str());
        return;
      }
      auto bias_host_op = in_nodes.at(2);

      in_nodes = bias_host_op->GetInAllNodes();
      if (in_nodes.empty()) {
        REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealCubeNd] node %s in nodes is empty.",
                        bias_host_op->GetName().c_str());
        return;
      }
      auto bias_node = in_nodes.at(0);
      (void) ge::GraphUtils::RemoveEdge(bias_host_op->GetOutDataAnchor(0), cube_node->GetInDataAnchor(2));
      (void) graph.RemoveNode(bias_host_op);
      (void) graph.RemoveNode(bias_node);
      // clear input data anchor
      ge::NodeUtils::ClearInDataAnchor(cube_node, cube_node->GetInDataAnchor(2));
      ge::OpDescUtils::ClearInputDesc(cube_node->GetOpDesc(), static_cast<uint32_t>(SECONDE_OPDESC_INDEX));
    }
  }
}

Status V200RequantFusionPass::RefreshNodeDtype(ge::NodePtr &next_node, const ge::DataType &data_type) const {
  FE_CHECK_NOTNULL(next_node);
  size_t const_index = 0;
  if (next_node->GetType() == CONCATV2) {
    const_index = static_cast<size_t>(next_node->GetAllInDataAnchorsSize() - 1);
  }
  if (IsValidConcatNode(next_node)) {
    for (size_t index = 0; index < next_node->GetOpDesc()->GetAllInputsSize(); index++) {
      if ((next_node->GetType() == CONCAT || next_node->GetType() == CONCATV2) && index == const_index) {
        continue;
      }
      auto input = next_node->GetOpDesc()->MutableInputDesc(index);
      if (input == nullptr) {
        continue;
      }
      input->SetOriginDataType(data_type);
      input->SetDataType(data_type);
    }
    for (size_t index = 0; index < next_node->GetOpDesc()->GetOutputsSize(); index++) {
      auto output = next_node->GetOpDesc()->MutableOutputDesc(index);
      output->SetOriginDataType(data_type);
      output->SetDataType(data_type);
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::DealDequantV200(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequant_nodes,
                                              vector<ge::NodePtr> &quants,
                                              float scale_quant, int8_t offset_quant,
                                              vector<ge::NodePtr> &fusion_nodes) const {
  static std::atomic<uint64_t> name_id(0);
  std::string quant_mode;
  for (size_t iter = 0; iter < dequant_nodes.size(); iter++) {
    // get deqaunt node quant mode
    (void)ge::AttrUtils::GetStr(dequant_nodes[iter]->GetOpDesc(), STR_QUANT_MODE, quant_mode);
    vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(dequant_nodes[iter]);
    if (weights.size() < 1) {
      FE_LOGI("weights get failed.");
      return NOT_CHANGED;
    }

    ge::GeTensorPtr scale_input = weights[0];
    FE_CHECK(scale_input == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] scale_input is nullptr."), return PARAM_INVALID);

    if (SetReluFlagToDequant(dequant_nodes[iter]) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] Set relu flag to node %s failed.",
                      dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }
    bool relu_flag = false;
    (void)ge::AttrUtils::GetBool(dequant_nodes[iter]->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
    // we need judge whether is leakyrelu,
    // becase dequant + quant reluflag is false
    auto dequant_out_nodes = dequant_nodes[iter]->GetOutAllNodes();
    if (dequant_out_nodes.empty()) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] node %s has no out nodes.",
                      dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }
    bool check_flag = dequant_out_nodes.at(0)->GetType() == LEAKY_RELU && !relu_flag;
    if (check_flag) {
      FE_LOGD("leakyrelu can not fusion, not do requant.");
      return SUCCESS;
    }

    /* Create Host Cpu Op */
    FE_LOGD("Create host op to calc deq_scale of node:[%s].", dequant_nodes[iter]->GetName().c_str());
    Status ret = CreateNewRequantHostCpuOp(REQUANT_HOST_CPU_OP_V2_RE, dequant_nodes[iter], scale_quant, graph,
                                           fusion_nodes);
    if ((ret != SUCCESS || fusion_nodes.empty())) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] Create host cpu op for dequant node %s failed",
                      dequant_nodes[iter]->GetName().c_str());
      return ret;
    }

    int32_t bias_size = 0;
    vector<int32_t> bias_value;
    vector<ge::NodePtr> cube_nodes;
    bool del_bias_flag = false;
    if (GetBiasValue(dequant_nodes[iter], quant_mode, cube_nodes, bias_size, del_bias_flag, bias_value) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] Get bias value failed, dequant node: [%s].",
                      dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }

    int scale_size = scale_input->GetData().size() / sizeof(uint64_t);
    int reqscale_size = bias_size == 0 ? scale_size : bias_size;

    ge::NodePtr host_cpu_node = dequant_nodes[iter]->GetInDataNodes().at(1);
    (void)ge::AttrUtils::SetBool(host_cpu_node->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
    (void)ge::AttrUtils::SetStr(host_cpu_node->GetOpDesc(), STR_QUANT_MODE, quant_mode);
    (void)ge::AttrUtils::SetInt(host_cpu_node->GetOpDesc(), ATTR_BIAS_SIZE, static_cast<int64_t>(bias_size));
    (void)ge::AttrUtils::SetInt(host_cpu_node->GetOpDesc(), ATTR_OFFSET_QUANT, static_cast<int64_t>(offset_quant));
    (void)ge::AttrUtils::SetListInt(host_cpu_node->GetOpDesc(), ATTR_BIAS_VALUE, bias_value);

    // set relu_flag
    if (SetRequantReluFlag(dequant_nodes[iter], offset_quant, relu_flag) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] set relu flag failed, requant node [%s].",
                      dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }

    bool no_bias_s9_flag = true;
    // delete bias input
    DealWithCubeNodes(graph, cube_nodes, del_bias_flag, no_bias_s9_flag);

    auto dequant_desc = dequant_nodes[iter]->GetOpDesc();
    dequant_desc->SetType(ASCENDREQUANT);
    dequant_desc->SetName("requant_" + dequant_desc->GetName());
    fusion_nodes.push_back(dequant_nodes[iter]);
    name_id.fetch_add(1, std::memory_order_relaxed);

    std::map<string, uint32_t> input_names;
    input_names["x"] = 0;
    input_names["req_scale"] = 1;
    dequant_desc->UpdateInputName(input_names);

    // ge::NodePtr host_cpu_node = dequant_nodes[iter]->GetInAllNodes().at(1);
    host_cpu_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_UINT64);
    host_cpu_node->GetOpDesc()->MutableOutputDesc(0)->SetShape(ge::GeShape({reqscale_size}));
    host_cpu_node->GetOpDesc()->MutableOutputDesc(0)->SetOriginShape(ge::GeShape({reqscale_size}));
    dequant_desc->MutableInputDesc(1)->SetShape(ge::GeShape({reqscale_size}));
    dequant_desc->MutableInputDesc(1)->SetOriginShape(ge::GeShape({reqscale_size}));
    if (quants.empty()) {
      REPORT_FE_ERROR("Requant fusion pass pattern is wrong, dequant node[%s]", dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }
    ge::DataType data_type = quants[0]->GetOpDesc()->GetOutputDesc(0).GetDataType();
    dequant_desc->MutableOutputDesc(0)->SetDataType(data_type);
    dequant_desc->MutableOutputDesc(0)->SetOriginDataType(data_type);
    /* Set ConcatD's datatype as int8.
     * ConcatD is the only consumer of dequant/requant */
    auto next_node = dequant_nodes[iter]->GetOutAllNodes().at(0);
    if (RefreshNodeDtype(next_node, data_type) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQntV200] Refresh next node [%s] data type failed.",
                      next_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::DealQuant(ge::ComputeGraph &graph, vector<ge::NodePtr> &quant_nodes) const {
  for (vector<ge::NodePtr>::iterator i = quant_nodes.begin(); i < quant_nodes.end(); ++i) {
    auto quant_all_in_nodes = (*i)->GetInAllNodes();
    if (quant_all_in_nodes.empty()) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQnt] node [%s] has no in nodes",
                      (*i)->GetOpDesc()->GetName().c_str());
      return FAILED;
    }

    auto pre_node = quant_all_in_nodes.at(0);
    if (NotRemoveLeakyRelu(pre_node) ||
        (IsValidConcatNode(pre_node) && NotRemoveLeakyRelu(GetFirstNoConstInput(pre_node)))) {
      return SUCCESS;
    }
    ge::DataType data_type = (*i)->GetOpDesc()->GetOutputDesc(0).GetDataType();
    /* Set ConcatD's datatype as int8.
     * ConcatD is in front of quant. */
    if (RefreshNodeDtype(pre_node, data_type) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQnt] Refresh next node [%s] data type failed.",
                      pre_node->GetName().c_str());
      return FAILED;
    }
    Status ret = graph.RemoveNode(*i);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQnt] Remove node [%s] failed!",
                      (*i)->GetOpDesc()->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status V200RequantFusionPass::DealQuantScale(vector<ge::NodePtr> &quant_nodes) const {
  for (vector<ge::NodePtr>::iterator i = quant_nodes.begin(); i < quant_nodes.end(); ++i) {
    if (!ge::AttrUtils::SetFloat((*i)->GetOpDesc(), ATTR_SCALE, 1.0)) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealQnt] set quant scale failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

bool V200RequantFusionPass::CheckDequantNotRequant(const vector<ge::NodePtr> &dequants) const {
  bool not_requant = false;
  for (auto dequant : dequants) {
    (void)ge::AttrUtils::GetBool(dequant->GetOpDesc(), ATTR_DEQUANT_NO_REQUANT, not_requant);
  }
  return not_requant;
}

Status V200RequantFusionPass::RequantProcess(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequants,
                                             vector<ge::NodePtr> &quants, vector<ge::NodePtr> &relus,
                                             float &scale_quant, int8_t &offset_quant,
                                             vector<ge::NodePtr> &new_nodes) const {
  Status ret;
  FE_LOGD("Start to do requant fusion pass.");
  bool relu_not_remove = !relus.empty() && !CheckNeedRemoveRelu(relus.at(0));
  if (relu_not_remove) {
    FE_LOGD("Do requant fusion pass, and activate node can not be removed.");
    ret = DealDequantNotRequantV200(graph, dequants, scale_quant, new_nodes);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal dequant nodes failed");
      return FAILED;
    }

    ret = DealQuantScale(quants);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal quant nodes failed");
      return FAILED;
    }

    ret = DealRelu(graph, relus, scale_quant);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal relu nodes failed");
      return FAILED;
    }

    if (TagNodes(quants, dequants, 1) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] tag nodes failed");
      return FAILED;
    }
  } else {
    FE_LOGD("Start to do general requant fusion pass.");
    ret = DealDequantV200(graph, dequants, quants, scale_quant, offset_quant, new_nodes);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal dequant nodes failed");
      return FAILED;
    }

    ret = DealQuant(graph, quants);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal quant nodes failed");
      return FAILED;
    }

    ret = DealRelu(graph, relus, scale_quant);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ReqntPcs] deal relu nodes failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::CheckQuantOp(const ge::NodePtr &node_ptr, vector<ge::NodePtr> &quants, int &quant_num,
                                           float &scale_base, int8_t &offset_base) const {
  quant_num++;
  quants.push_back(node_ptr);

  // judge whether all scale equal
  float scale_tmp = 0;
  if (!ge::AttrUtils::GetFloat(node_ptr->GetOpDesc(), ATTR_SCALE, scale_tmp)) {
    FE_LOGW("Get scale attr of quant node[%s] failed!", node_ptr->GetName().c_str());
    return NOT_CHANGED;
  }
  if (fabs(scale_base) < kFloatEsp) {
    scale_base = scale_tmp;
  } else if (fabs(scale_tmp - scale_base) > kFloatEsp) {
    FE_LOGW("The scale attr of two quant nodes are not equal!");
    return NOT_CHANGED;
  }
  float_t offset_a = 0;
  (void)ge::AttrUtils::GetFloat(node_ptr->GetOpDesc(), ATTR_OFFSET, offset_a);
  offset_base = static_cast<int8_t>(offset_a);
  return SUCCESS;
}

Status V200RequantFusionPass::Pattern0Parse(ge::NodePtr dequant_node, vector<ge::NodePtr> &dequants,
                                            vector<ge::NodePtr> &quants, vector<ge::NodePtr> &relus,
                                            float &scale_quant, int8_t &offset_quant) const {
  bool has_relu = false;
  float scale_base = 0;
  int8_t offset_base = 0;
  int relu_num = 0;
  int quant_num = 0;
  int relu_del = 0;

  int direct_out_node_num = dequant_node->GetOutAllNodes().size();

  for (auto node_ptr : dequant_node->GetOutAllNodes()) {
    if (CheckReluValid(node_ptr)) {
      if (CheckNeedRemoveRelu(node_ptr)) {
        relu_del++;
      }
      relu_num++;
      relus.push_back(node_ptr);
      has_relu = true;
      for (auto node_next : node_ptr->GetOutAllNodes()) {
        if (node_next->GetType() != ASCEND_QUANT) {
          FE_LOGW("next node != quant");
          return NOT_CHANGED;
        }
        if (CheckQuantOp(node_next, quants, quant_num, scale_base, offset_base) != SUCCESS) {
          FE_LOGW("Check quant op[%s] failed.", node_next->GetName().c_str());
          return NOT_CHANGED;
        }
      }

    } else if (node_ptr->GetType() == ASCEND_QUANT) {
      if (CheckQuantOp(node_ptr, quants, quant_num, scale_base, offset_base) != SUCCESS) {
        FE_LOGW("Check quant op[%s] failed.", node_ptr->GetName().c_str());
        return NOT_CHANGED;
      }
    } else {
      return NOT_CHANGED;
    }
  }

  bool cond = JudgeCondition(has_relu, relu_num, direct_out_node_num, quant_num, relu_del);
  if (cond) {
    return NOT_CHANGED;
  }
  bool del_relu_flag = relu_del ? true : false;
  if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
    REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][Ptn0Parse] set relu del failed! node name: %s",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  dequants.push_back(dequant_node);
  scale_quant = scale_base;
  offset_quant = offset_base;
  return SUCCESS;
}

bool V200RequantFusionPass::JudgeCondition(const bool &has_relu, const int &relu_num, const int &direct_out_node_num,
                                           const int &quant_num, const int &relu_del) const {
  bool cond = ((has_relu && relu_num != direct_out_node_num) || (has_relu && relu_num != 1 && relu_num != quant_num) ||
          (!has_relu && quant_num != direct_out_node_num) || (quant_num == 0) ||
          (relu_del && relu_del != direct_out_node_num));
  return cond;
}

// if leakyrelu not deleted, not requant
Status V200RequantFusionPass::HandlePattern0(ge::ComputeGraph &graph,
                                             ge::NodePtr dequant_node,
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
  float scale_quant;
  int8_t offset_quant;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;

  if (Pattern0Parse(dequant_node, dequants, quants, relus, scale_quant, offset_quant) != SUCCESS) {
    FE_LOGD("Do not need do requant pattern0 here, dequant name[%s]", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  if (CheckDequantNotRequant(dequants)) {
    return NOT_CHANGED;
  }
  FE_LOGD("Size of relu node size: %zu, size of quant node is %zu", relus.size(), quants.size());

  Status ret = RequantProcess(graph, dequants, quants, relus, scale_quant, offset_quant, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][HdlPtn0] requant fusion pass fail, dequant node [%s].",
                    dequant_node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

bool V200RequantFusionPass::IsConcatDimC(const ge::NodePtr &node_ptr, int32_t &dim_attr_value,
                                         const uint32_t &const_index) const {
  std::shared_ptr<NodeOptimizeCheckerBase> node_checker_ptr = nullptr;
  FE_MAKE_SHARED(node_checker_ptr = std::make_shared<NodeOptimizeCheckerBase>(), return FAILED);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  string node_name = node_ptr->GetName();

  for (uint32_t index = 0; index < node_ptr->GetAllInDataAnchorsSize(); index++) {
    if (index == const_index) {
      continue;
    }
    auto tensor_desc = op_desc_ptr->GetInputDesc(index);
    // 2. get the postion of the c axis
    int pos = 0;
    Status status = node_checker_ptr->GetPosOfDimC(tensor_desc, pos);
    if (status != SUCCESS) {
      FE_LOGD("Node[%s]: get the dim_c position of the input [%d] not success.", node_name.c_str(), index);
      return false;
    }

    // 3. if the dim_attr_value < 0, add the dim_num
    int dim_num = tensor_desc.GetOriginShape().GetDimNum();
    if (dim_attr_value < 0) {
      dim_attr_value += dim_num;
    }
    if (pos != dim_attr_value) {
      FE_LOGD(
          "Node[%s]: the dim_c position of the input [%d] is not equal to "
          "concat_dim, check failed.",
          node_name.c_str(), index);
      return false;
    }
  }
  return true;
}

Status V200RequantFusionPass::CheckConcatDOpAligned(const ge::NodePtr &concat_node, const ge::DataType &data_type)
    const {
  std::shared_ptr<NodeOptimizeCheckerBase> node_checker_ptr = nullptr;
  FE_MAKE_SHARED(node_checker_ptr = std::make_shared<NodeOptimizeCheckerBase>(), return FAILED);
  // concat dim is not dim c, not need to check
  if (!node_checker_ptr->IsDimC(concat_node, CONCAT_DIM, true)) {
    FE_LOGD("concat node %s concat dim is not dim_c.", concat_node->GetName().c_str());
    return SUCCESS;
  }
  int dim_c = 0;
  for (auto &input_desc : concat_node->GetOpDesc()->GetAllInputsDesc()) {
    // check all input_desc
    if (node_checker_ptr->GetDimC(input_desc, dim_c) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ChkConcatDOpAlig] Get dim_c value failed, concat node:[%s]",
                      concat_node->GetName().c_str());
      return FAILED;
    }
    if (!node_checker_ptr->IsDimCOfInputAligned(input_desc, dim_c, data_type)) {
      FE_LOGD("concat node [%s] dim_c is not aligned, not requant fusion.", concat_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status V200RequantFusionPass::CheckConcatOpAligned(ge::NodePtr &concat_node, ge::DataType &data_type) {
  std::shared_ptr<NodeOptimizeCheckerBase> node_checker_ptr = nullptr;
  FE_MAKE_SHARED(node_checker_ptr = std::make_shared<NodeOptimizeCheckerBase>(), return FAILED);
  // get const input index concat: 0; concatv2: InDataAnchorsSize - 1
  uint32_t const_index = 0;
  if (concat_node->GetType() == CONCATV2) {
    if (concat_node->GetAllInDataAnchorsSize() == 0) {
      FE_LOGW("concat node:[%s] get 0 in data anchors", concat_node->GetName().c_str());
      return FAILED;
    }
    const_index = concat_node->GetAllInDataAnchorsSize() - 1;
  }
  ge::NodePtr const_node = concat_node->GetInAllNodes().at(const_index);
  vector<ge::ConstGeTensorPtr> weights = ge::OpDescUtils::GetWeights(concat_node);
  if (weights.size() != 1) {
    FE_LOGW("concat node[%s] can not do const to attr.", concat_node->GetName().c_str());
    return FAILED;
  }
  ge::ConstGeTensorPtr const_input = weights[0];
  uint8_t *data = const_cast<uint8_t *>(const_input->GetData().GetData());
  int32_t *concat_dim = reinterpret_cast<int32_t *>(data);
  FE_CHECK(concat_dim == nullptr, FE_LOGW("The data of const node[%s] is nullptr.", const_node->GetName().c_str()),
           return FAILED);
  // concat dim is not dim c, not need to check
  if (!IsConcatDimC(concat_node, concat_dim[0], const_index)) {
    FE_LOGD("concat node %s concat dim is not dim_c.", concat_node->GetName().c_str());
    return SUCCESS;
  }
  int dim_c = 0;
  for (uint32_t index = 0; index < concat_node->GetAllInDataAnchorsSize(); index++) {
    if (index == const_index) {
      continue;
    }
    auto input_desc = concat_node->GetOpDesc()->GetInputDesc(index);
    // check all input_desc
    if (node_checker_ptr->GetDimC(input_desc, dim_c) != SUCCESS) {
      FE_LOGW("Get dim_c value failed, concat node:[%s]", concat_node->GetName().c_str());
      return FAILED;
    }
    if (!node_checker_ptr->IsDimCOfInputAligned(input_desc, dim_c, data_type)) {
      FE_LOGD("concat node [%s] dim_c is not aligned, not requant fusion.", concat_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

bool V200RequantFusionPass::IsConstToAttrInput(const ge::NodePtr &concat_node, const ge::NodePtr &const_node) const {
  uint32_t index = const_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetIdx();
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

Status V200RequantFusionPass::CheckOpInputAligned(ge::NodePtr &concat_node, ge::DataType &data_type) {
  if (concat_node->GetType() == CONCATD || concat_node->GetType() == CONCATV2D) {
    if (CheckConcatDOpAligned(concat_node, data_type) != SUCCESS) {
      FE_LOGD("concat node [%s] dim_c is not aligned.", concat_node->GetName().c_str());
      return FAILED;
    }
  } else if (concat_node->GetType() == CONCAT || concat_node->GetType() == CONCATV2) {
    if (CheckConcatOpAligned(concat_node, data_type) != SUCCESS) {
      FE_LOGD("concat node [%s] dim_c is not aligned.", concat_node->GetName().c_str());
      return FAILED;
    }
  } else {
    FE_LOGD("node [%s] is not concat node.", concat_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status V200RequantFusionPass::CheckConcatOpInput(const ge::NodePtr &concat_node, vector<ge::NodePtr> &dequants,
                                                 vector<ge::NodePtr> &relus) const {
  for (auto node_next : concat_node->GetInAllNodes()) {
    if (CheckReluValid(node_next)) {
      relus.push_back(node_next);
      auto node_n_n = node_next->GetInAllNodes().at(0);
      if (node_n_n->GetType() != ASCEND_DEQUANT || node_next->GetInAllNodes().size() != 1) {
        FE_LOGD("next-next node != dequant, not need do requant");
        return FAILED;
      }
      bool del_relu_flag = CheckNeedRemoveRelu(node_next);
      if (!ge::AttrUtils::SetBool(node_n_n->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
        REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][ChkConcatOpIn] set relu del failed! node name: %s",
                        node_n_n->GetName().c_str());
        return FAILED;
      }
      dequants.push_back(node_n_n);
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

Status V200RequantFusionPass::CheckConcatOpInput(const ge::NodePtr &concat_node, const ge::NodePtr &relu_node,
                                                 vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &relus) const {
  for (auto input_node : concat_node->GetInAllNodes()) {
    if (input_node->GetType() == ASCEND_DEQUANT) {
      bool del_relu_flag = CheckNeedRemoveRelu(relu_node);
      FE_LOGD("delete relu nodes flag: [%d].", del_relu_flag);
      if (!ge::AttrUtils::SetBool(input_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
        FE_LOGW("set relu del failed! node name: %s", input_node->GetName().c_str());
        return FAILED;
      }
      dequants.push_back(input_node);
    } else if (CheckReluValid(input_node)) {
      relus.push_back(input_node);
      auto first_input_node = input_node->GetInAllNodes().at(0);
      if (first_input_node->GetType() != ASCEND_DEQUANT || input_node->GetInAllNodes().size() != 1) {
        FE_LOGD("next-next node != dequant, dont need do requant");
        return NOT_CHANGED;
      }
      bool del_relu_flag = CheckNeedRemoveRelu(input_node);
      if (del_relu_flag) {
        FE_LOGD("The flag value of _need_delrelu_of_dequant is true");
      }
      if (!ge::AttrUtils::SetBool(first_input_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
        FE_LOGW("set relu del failed! node name: %s", first_input_node->GetName().c_str());
        return FAILED;
      }
      dequants.push_back(first_input_node);
    } else if (input_node->GetType() == CONSTANT) {
      if (!IsConstToAttrInput(concat_node, input_node)) {
        FE_LOGD("const node [%s] can not convert to attr.", concat_node->GetName().c_str());
        return NOT_CHANGED;
      }
    } else {
      return NOT_CHANGED;
    }
  }
  return SUCCESS;
}
/*
  dequant dequant dequant
     |       |       |
  relu     relu    relu   (optional)
             |
          concat
             |
            relu     (optional)
             |
           qaunt
*/
Status V200RequantFusionPass::Pattern1Parse(ge::NodePtr quant_node,
                                            vector<ge::NodePtr> &dequants, vector<ge::NodePtr> &quants,
                                            vector<ge::NodePtr> &relus, float &scale_quant, int8_t &offset_quant) {
  float_t offset_a = 0;
  ge::DataType data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  for (auto node_ptr : quant_node->GetInAllNodes()) {
    if (IsValidConcatNode(node_ptr)) {
      if (node_ptr->GetOutAllNodes().size() != 1) {
        FE_LOGD("concat's out num should be one.");
        return NOT_CHANGED;
      }
      if (CheckOpInputAligned(node_ptr, data_type) != SUCCESS) {
        FE_LOGD("concat node [%s] dim_c is not aligned.", node_ptr->GetName().c_str());
        return NOT_CHANGED;
      }
      if (CheckConcatOpInput(node_ptr, dequants, relus) != SUCCESS) {
        FE_LOGD("Pattern1 not all matched, concat node[%s].", node_ptr->GetName().c_str());
        return NOT_CHANGED;
      }
    } else if (CheckReluValid(node_ptr)) {
      relus.push_back(node_ptr);
      for (auto node_next : node_ptr->GetInAllNodes()) {
        if (!IsValidConcatNode(node_next)) {
          FE_LOGD("next node[%s], type[%s] is not concat, no need to do requant",
                  node_next->GetName().c_str(), node_next->GetType().c_str());
          return NOT_CHANGED;
        }
        if (node_next->GetOutAllNodes().size() != 1) {
          FE_LOGD("concat's out num should be one.");
          return NOT_CHANGED;
        }
        if (CheckOpInputAligned(node_next, data_type) != SUCCESS) {
          FE_LOGD("concat node [%s] dim_c is not aligned.", node_next->GetName().c_str());
          return NOT_CHANGED;
        }
        if (CheckConcatOpInput(node_next, node_ptr, dequants, relus) != SUCCESS) {
          FE_LOGD("Pattern1 not all matched, concat node[%s].", node_ptr->GetName().c_str());
          return NOT_CHANGED;
        }
      }
    } else {
      return NOT_CHANGED;
    }
  }

  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_SCALE, scale_quant)) {
    FE_LOGW("Cannot find scale attr in Node[%s].", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  (void)ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_OFFSET, offset_a);
  offset_quant = static_cast<int8_t>(offset_a);
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
Status V200RequantFusionPass::HandlePattern1(ge::ComputeGraph &graph, ge::NodePtr quant_node,
                                             vector<ge::NodePtr> &new_nodes) {
  Status ret;
  float scale_quant = 1;
  int8_t offset_quant = 0;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;

  ret = Pattern1Parse(quant_node, dequants, quants, relus, scale_quant, offset_quant);
  if (ret != SUCCESS) {
    FE_LOGD("Do not need do requant pattern1 for node[%s]", quant_node->GetName().c_str());
    return SUCCESS;
  }
  for (auto dequant_node : dequants) {
    if (dequant_node->GetOutAllNodes().size() != 1) {
      FE_LOGD("Dequant node:[%s] is not single reference, not fusion", dequant_node->GetName().c_str());
      return NOT_CHANGED;
    }
  }
  if (CheckDequantNotRequant(dequants)) {
    return NOT_CHANGED;
  }
  ret = RequantProcess(graph, dequants, quants, relus, scale_quant, offset_quant, new_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][HdlPtn1] requant fusion pass fail, quant node [%s].",
                    quant_node->GetName().c_str());
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
 *        |                                |
 *     requant                          requant
 *        |                            |   |   |
 *
 *
 * ------------------pattern1-------------------
 * ####pattern1-scenario1                      ####pattern1-scenario2
 *
 * requant requant requant                 requant requant requant
 *    |       |       |                               |
 *         concat                                   concat
 *            |                                       |
 *
 */
vector<FusionPattern *> V200RequantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("requantPassPattern0");
  FE_CHECK(pattern0 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DefPtn] new FusionPattern object failed!"), return patterns);
  pattern0->AddOpDesc(PATTERN_DEQUANT, {ASCEND_DEQUANT}).SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern0);

  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("requantPassPattern1");
  FE_CHECK(pattern1 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DefPtn] new FusionPattern object failed!"), return patterns);
  pattern1->AddOpDesc(PATTERN_QUANT, {ASCEND_QUANT}).SetOutput(PATTERN_QUANT);

  patterns.push_back(pattern1);
  return patterns;
}

Status V200RequantFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr dequant_node = GetNodeFromMapping(PATTERN_DEQUANT, mapping);
  if (dequant_node.get()) {
    return HandlePattern0(graph, dequant_node, fusion_nodes);
  }
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);
  if (quant_node.get()) {
    return HandlePattern1(graph, quant_node, fusion_nodes);
  }
  return NOT_CHANGED;
}

}  // namespace fe
