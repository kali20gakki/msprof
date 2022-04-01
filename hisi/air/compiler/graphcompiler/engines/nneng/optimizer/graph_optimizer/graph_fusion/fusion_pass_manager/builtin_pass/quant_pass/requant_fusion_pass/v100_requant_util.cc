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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_requant_util.h"
#include <string>
#include <vector>
#include <cmath>
#include "common/math_util.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "common/configuration.h"

namespace fe {

Status SetDequantNodeAttr(ge::NodePtr &dequant_node, ge::NodePtr &host_node, const float &scale_quant) {
  vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(host_node);
  if (weights.size() < 1) {
    FE_LOGI("weights get failed. Node name: %s", host_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ge::GeTensorPtr scale_input = weights[0];
  FE_CHECK(scale_input == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][SetDeqntNdAttr] scaleInput is nullptr."), return PARAM_INVALID);

  // uint64: [31:0] for scale; [39:42] for N; [47:40] for offset_w
  int scale_size = scale_input->GetData().size() / sizeof(uint64_t);
  std::uint8_t *data = const_cast<uint8_t *>(scale_input->GetData().data());
  uint64_t *scale_org_data = reinterpret_cast<uint64_t *>(data);
  float scale_data[scale_size];

  FE_LOGD("Scale quant %f for dequant node: %s", scale_quant, dequant_node->GetName().c_str());
  FE_CHECK(scale_org_data == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][SetDeqntNdAttr] scaleOrgData is nullptr."), return PARAM_INVALID);
  for (int j = 0; j < scale_size; j++) {
    uint32_t scale_tmp = (GET_DEQUANT_SCALE_DEQ(scale_org_data[j]));
    scale_data[j] = reinterpret_cast<float &>(scale_tmp);
  }

  for (int j = 0; j < scale_size; j++) {
    FE_FLOAT_MULCHECK(scale_data[j], scale_quant);
    scale_data[j] = scale_data[j] * scale_quant;
  }

  bool need_sqrt = false;
  for (int j = 0; j < scale_size; j++) {
    if (scale_data[j] < pow(SCALE_BASE, SCALE_EXPONENT)) {
      need_sqrt = true;
      break;
    }
  }

  if (need_sqrt) {
    // sqrt(scale) & set sqrt_mode = 1
    for (int j = 0; j < scale_size; j++) {
      scale_data[j] = sqrt(scale_data[j]);
    }
    if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_SQRT_MODE, true)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][SetDeqntNdAttr] set sqrt_mode failed!");
      return FAILED;
    }
    FE_LOGD("Set sqrt_mode=true, node name: %s", dequant_node->GetName().c_str());
  } else {
    if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_SQRT_MODE, false)) {
      REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][SetDeqntNdAttr] set sqrt_mode failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status DealDequantV100Checkfp16(vector<ge::NodePtr> &dequant_nodes) {
  for (uint32_t i = 0; i < dequant_nodes.size(); i++) {
      if (dequant_nodes[i]->GetOpDesc()->GetInputDesc(1).GetDataType() != ge::DT_FLOAT16) {
        REPORT_FE_ERROR("[GraphOpt][V100ReqntFus][DealDeqntV100] not all deq_scale is fp16 input.");
        return FAILED;
      }
      continue;
  }
  return SUCCESS;
}

Status DealDequantV100NotCheckArch(ge::NodePtr dequant_node,
                                   const float &scale_quant, vector<ge::NodePtr> &new_nodes) {
  // After insertion of requant host cpu op, the weight of dequant node will
  // become host cpu op and the original const node will be the weight of
  // the new host cpu op
  // Get the const of new requant node
  auto host_cpu_node = new_nodes.back();
  if (SetDequantNodeAttr(dequant_node, host_cpu_node, scale_quant) != SUCCESS) {
    REPORT_FE_ERROR("Set dequant node[%s] attr failed.", dequant_node->GetName().c_str());
    return FAILED;
  }
  /* We should update the output datatype of tensor desc of host cpu op.
   * After folding, the output datatype will become the same as the host
   * cpu op's output datatyppe */
  host_cpu_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_FLOAT);
  host_cpu_node->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(ge::DT_FLOAT);
  if (host_cpu_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().empty()) {
    REPORT_FE_ERROR("set relu flag failed by peer in anchors empty!");
    return FAILED;
  }
  int idx = host_cpu_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetIdx();
  FE_LOGD("Set the output DataType[fp32] for const input of requant host op[%s]", host_cpu_node->GetName().c_str());
  auto input_desc_of_dequant_node = dequant_node->GetOpDesc()->MutableInputDesc(idx);
  input_desc_of_dequant_node->SetDataType(ge::DT_FLOAT16);
  input_desc_of_dequant_node->SetOriginDataType(ge::DT_FLOAT16);
  if (SetReluFlagToDequant(dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("Set relu flag to node %s failed.", dequant_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status DealDequantV100CheckArch(ge::NodePtr dequant_node,
                                std::string &quant_mode) {
  FE_LOGD("dequant op[%s] adopts 4bit quantification, create %s node.",
          dequant_node->GetName().c_str(), REQUANT_HOST_CPU_OP_V2.c_str());
  // get dequant node quant mode
  (void)ge::AttrUtils::GetStr(dequant_node->GetOpDesc(), STR_QUANT_MODE, quant_mode);
  if (quant_mode == STR_QUANT_HIGH_PERFORMANCE) {
    REPORT_FE_ERROR("Platform is v200 and quant node adpots int4, quant mode must be %s, but current is %s",
                    STR_QUANT_HIGH_PRECISION.c_str(), quant_mode.c_str());
    return FAILED;
  }
  FE_LOGD("Set relu flag to dequant node.");
  if (SetReluFlagToDequant(dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("Set relu flag to node %s failed.", dequant_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status DealDequantV100(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequant_nodes,
                       const float &scale_quant, vector<ge::NodePtr> &new_nodes) {
  bool check_fp16_dtype = !dequant_nodes.empty() &&
                          dequant_nodes[0]->GetOpDesc()->GetInputDesc(1).GetDataType() == ge::DT_FLOAT16;
  if (check_fp16_dtype) {
    return DealDequantV100Checkfp16(dequant_nodes);
  }
  ISAArchVersion isa_arch_version = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  for (size_t i = 0; i < dequant_nodes.size(); i++) {
    /* Create Host Cpu Op */
    auto input_nodes = dequant_nodes[i]->GetInDataNodes();
    if (input_nodes.size() < 1) {
      REPORT_FE_ERROR("The number of input nodes for node %s is less than 1.",  dequant_nodes[i]->GetName().c_str());
      return FAILED;
    }
    auto cube_node = input_nodes.at(0);
    FE_CHECK(cube_node == nullptr, FE_LOGE("cubeNode is null."), return FAILED);
    bool check_arch_version_and_int4_dtype = (isa_arch_version == ISAArchVersion::EN_ISA_ARCH_V200) &&
                                             cube_node->GetOpDesc()->GetInputDesc(0).GetDataType() == ge::DT_INT4;
    if (check_arch_version_and_int4_dtype) {
      std::string quant_mode;
      Status ret = DealDequantV100CheckArch(dequant_nodes[i], quant_mode);
      if (ret != SUCCESS) {
        return ret;
      }
      /* Create Host Cpu Op */
      FE_LOGD("Create host op to calc deq_scale of node:[%s].", dequant_nodes[i]->GetName().c_str());
      ret = CreateNewRequantHostCpuOp(REQUANT_HOST_CPU_OP_V2, dequant_nodes[i], scale_quant, graph, new_nodes);
      bool check_new_nodes_empty = (ret != SUCCESS || new_nodes.empty());
      if (check_new_nodes_empty) {
        REPORT_FE_ERROR("Create host cpu op for dequant node %s failed", dequant_nodes[i]->GetName().c_str());
        return ret;
      }
      auto host_cpu_node = new_nodes.back();
      bool relu_flag = false;
      (void)ge::AttrUtils::GetBool(dequant_nodes[i]->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
      (void)ge::AttrUtils::SetBool(host_cpu_node->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
      (void)ge::AttrUtils::SetStr(host_cpu_node->GetOpDesc(), STR_QUANT_MODE, quant_mode);
    } else {
      Status ret = CreateNewRequantHostCpuOp(REQUANT_HOST_CPU_OP, dequant_nodes[i], scale_quant, graph, new_nodes);
      bool check_new_nodes_empty_flag = (ret != SUCCESS || new_nodes.empty());
      if (check_new_nodes_empty_flag) {
        REPORT_FE_ERROR("Create host cpu op for dequant node %s failed", dequant_nodes[i]->GetName().c_str());
        return ret;
      }
      ret = DealDequantV100NotCheckArch(dequant_nodes[i], scale_quant, new_nodes);
      if (ret != SUCCESS) {
        return ret;
      }
    }
  }
  return SUCCESS;
}

}  // namespace fe
