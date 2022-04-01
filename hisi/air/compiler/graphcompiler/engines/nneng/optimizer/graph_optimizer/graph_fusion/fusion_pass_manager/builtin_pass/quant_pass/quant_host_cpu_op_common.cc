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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include <atomic>
#include <sstream>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/math_util.h"
#include "graph/utils/op_desc_utils.h"
namespace fe {
const float kFloatEsp = 10e-6;

uint64_t GetHostCpuAtomicId() {
  static std::atomic<uint64_t> global_trans_atomic_id(0);
  return global_trans_atomic_id.fetch_add(1, std::memory_order_relaxed);
}

Status CreateNewRequantHostCpuOp(const string &op_type, const ge::NodePtr &dequant_node, const float &scale_quant,
                                 ge::ComputeGraph &graph, vector<ge::NodePtr> &new_nodes) {
  FE_LOGI("Create new requant host op for dequant node [%s].", dequant_node->GetName().c_str());
  std::stringstream op_name_temp;
  // The atomic id of trans nodes must be unique.(start from 0)
  op_name_temp << op_type << "_" << GetHostCpuAtomicId();
  ge::OpDescPtr requant_host_cpu_op;
  FE_MAKE_SHARED(requant_host_cpu_op = std::make_shared<ge::OpDesc>(op_name_temp.str(), op_type), return FAILED);
  // 1. Get the const of dequant
  vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(dequant_node);
  if (weights.size() < 1) {
    FE_LOGI("Get weights failed. Node name: %s", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ge::GeTensorPtr scale_output = weights[0];
  FE_CHECK(scale_output == nullptr, REPORT_FE_ERROR("[GraphOpt][Quant][CrtNewRqtHsCpuOp] scaleInput is nullptr."),
           return PARAM_INVALID);

  // 2. Add input and output desc of new host op
  ge::GeTensorDesc scale_input_of_new_host_cpu_op = scale_output->GetTensorDesc();
  requant_host_cpu_op->AddInputDesc(REQUANT_HOST_CPU_OP_INPUT, scale_input_of_new_host_cpu_op);
  requant_host_cpu_op->MutableInputDesc(0)->SetOriginDataType(scale_input_of_new_host_cpu_op.GetDataType());
  requant_host_cpu_op->MutableInputDesc(0)->SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(scale_input_of_new_host_cpu_op.GetFormat())));
  requant_host_cpu_op->MutableInputDesc(0)->SetOriginShape(scale_input_of_new_host_cpu_op.GetShape());

  /* The output original datatype is fp32, and it actually is fp32, so
   * we don't need to specify this. */
  requant_host_cpu_op->AddOutputDesc(REQUANT_HOST_CPU_OP_OUTPUT, scale_input_of_new_host_cpu_op);
  requant_host_cpu_op->MutableOutputDesc(0)->SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(scale_input_of_new_host_cpu_op.GetFormat())));
  requant_host_cpu_op->MutableOutputDesc(0)->SetOriginShape(scale_input_of_new_host_cpu_op.GetShape());

  auto requant_node = graph.AddNode(requant_host_cpu_op);
  FE_CHECK_NOTNULL(requant_node);
  new_nodes.emplace_back(requant_node);
  /* 3. Add edges between dequant_scale_weight <-> new_host_cpu_op:0 */
  ge::InDataAnchorPtr dequant_input_anchor = dequant_node->GetInDataAnchor(DEQUANT_SCALE_INDEX_OF_DEQUANT_OP);
  ge::OutDataAnchorPtr dequant_scale_peer_out_anchor = dequant_input_anchor->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(dequant_scale_peer_out_anchor);
  auto requant_host_cpu_input_anchor = requant_node->GetInDataAnchor(DEQUANT_SCALE_INDEX_OF_REQUANT_OP);
  if (ge::GraphUtils::AddEdge(dequant_scale_peer_out_anchor, requant_host_cpu_input_anchor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Quant][CrtNewRqtHsCpuOp] Add Edge between dequant scale %s and new host cpu op %s failed.",
        dequant_node->GetName().c_str(), requant_node->GetName().c_str());
    return FAILED;
  }

  if (ge::GraphUtils::RemoveEdge(dequant_scale_peer_out_anchor, dequant_input_anchor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Quant][CrtNewRqtHsCpuOp] Remove Edge between dequant scale %s and its weight failed.",
                    dequant_node->GetName().c_str());
    return FAILED;
  }

  auto requant_host_cpu_output_anchor = requant_node->GetOutDataAnchor(0);
  if (ge::GraphUtils::AddEdge(requant_host_cpu_output_anchor, dequant_input_anchor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Quant][CrtNewRqtHsCpuOp] Add Edge between new host cpu op %s and dequant scale %s failed.",
        dequant_node->GetName().c_str(), requant_node->GetName().c_str());
    return FAILED;
  }
  /* Set Attr of Scale Quant */
  (void)ge::AttrUtils::SetFloat(requant_node->GetOpDesc(), QUANT_SCALE, scale_quant);
  return SUCCESS;
}

Status PadShapeTo4Dim(const ge::Format &filter_format, const std::vector<int64_t> &filter_dims,
                      std::vector<int64_t> &filter_dims4_d) {
  size_t size_of_filter = filter_dims.size();
  FE_LOGD("sizeOfFilter is %zu", size_of_filter);
  for (size_t i = 0; i <= LAST_AXIS_INDEX; i++) {
    if (i < size_of_filter) {
      FE_LOGD("dim [%zu] is %ld", i, filter_dims.at(i));
      filter_dims4_d.emplace_back(filter_dims.at(i));
    } else {
      if (filter_format == ge::FORMAT_NCHW) {
        filter_dims4_d.emplace_back(1);
      } else if (filter_format == ge::FORMAT_HWCN) {
        filter_dims4_d.insert(filter_dims4_d.cbegin(), 1);
      } else if (filter_format == ge::FORMAT_NHWC) {
        filter_dims4_d.insert(filter_dims4_d.cbegin() + 1, 1);
      } else if (filter_format == ge::FORMAT_ND) {
        filter_dims4_d.emplace_back(0);
      } else {
        REPORT_FE_ERROR("[GraphOpt][Quant][PadShpTo4Dim] format %s can not pad shape.",
                        ge::TypeUtils::FormatToSerialString(filter_format).c_str());
        return FAILED;
      }
    }
  }

  if (!filter_dims4_d.empty() && filter_dims4_d.size() >= LAST_AXIS_INDEX) {
    FE_LOGD("Quant bias optimize, weight shape is %s:[%ld %ld %ld %ld].",
            ge::TypeUtils::FormatToSerialString(filter_format).c_str(), filter_dims4_d[NCHW_DIM_N],
            filter_dims4_d[NCHW_DIM_C], filter_dims4_d[NCHW_DIM_H], filter_dims4_d[NCHW_DIM_W]);
  }
  return SUCCESS;
}

bool IsValidConcatNode(const ge::NodePtr &concat_node) {
  std::string node_type = concat_node->GetType();
  return (node_type == CONCAT || node_type == CONCATD || node_type == CONCATV2 || node_type == CONCATV2D);
}

Status TagNodes(vector<ge::NodePtr> &quant_nodes, vector<ge::NodePtr> &dequant_nodes, const int &attr_value) {
  for (size_t i = 0; i < dequant_nodes.size(); i++) {
    if (!ge::AttrUtils::SetInt(dequant_nodes[i]->GetOpDesc(), ATTR_REQUANTED, attr_value)) {
      REPORT_FE_ERROR("[GraphOpt][Quant][TagNd] Set dequant requanted attr failed!");
      return FAILED;
    }
  }

  for (size_t i = 0; i < quant_nodes.size(); i++) {
    if (!ge::AttrUtils::SetInt(quant_nodes[i]->GetOpDesc(), ATTR_REQUANTED, attr_value)) {
      REPORT_FE_ERROR("[GraphOpt][Quant][TagNd] Set quant requanted attr failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

bool IsEnableReluOfDequant(const ge::NodePtr &dequant_node) {
  bool del_relu_flag = false;
  if (!ge::AttrUtils::GetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
    FE_LOGD("Relu del attr is empty, node name: %s", dequant_node->GetName().c_str());
    return false;
  }
  return del_relu_flag;
}

Status SetReluFlag(const ge::NodePtr &dequant_node) {
  ge::NodePtr node_next = dequant_node->GetOutAllNodes().at(0);
  ge::NodePtr node_next_next;

  if (node_next->GetType() == RELU || node_next->GetType() == LEAKY_RELU) {
    if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_RELU_FLAG, true)) {
      REPORT_FE_ERROR("[GraphOpt][Quant][SetReluFlag] set sqrt_mode failed! node name: %s",
                      dequant_node->GetName().c_str());
      return FAILED;
    }
    return SUCCESS;
  }

  if (IsValidConcatNode(node_next)) {
    if (node_next->GetOutAllNodes().empty()) {
      REPORT_FE_ERROR("[GraphOpt][Quant][SetReluFlag] node name: %s has no out nodes", node_next->GetName().c_str());
      return FAILED;
    }
    node_next_next = node_next->GetOutAllNodes().at(0);
    if (node_next_next->GetType() == RELU || node_next_next->GetType() == LEAKY_RELU) {
      if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_RELU_FLAG, true)) {
        REPORT_FE_ERROR("[GraphOpt][Quant][SetReluFlag] Set relu flag failed! node name: %s",
                        dequant_node->GetName().c_str());
        return FAILED;
      }
      return SUCCESS;
    }
  }

  if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_RELU_FLAG, false)) {
    REPORT_FE_ERROR("[GraphOpt][Quant][SetReluFlag] Set relu flag failed! node name: %s",
                    dequant_node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status SetReluFlagToDequant(ge::NodePtr &dequant_node) {
  if (IsEnableReluOfDequant(dequant_node)) {
    if (SetReluFlag(dequant_node) != SUCCESS) {
      FE_LOGE("set relu flag failed!");
      return FAILED;
    }
    FE_LOGD("set relu flag=true, node name: %s", dequant_node->GetName().c_str());
  } else {
    if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_RELU_FLAG, false)) {
      FE_LOGE("set relu flag failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

bool CheckReluValid(const ge::NodePtr &node) {
  if (node->GetType() != RELU && node->GetType() != LEAKY_RELU && node->GetType() != RELU6) {
    return false;
  }
  FE_LOGD("Relu op[%s], input size: %zu, out size: %zu.", node->GetName().c_str(), node->GetInAllNodes().size(),
          node->GetOutAllNodes().size());
  if ((node->GetInAllNodes().size() != 1) || (node->GetOutAllNodes().size() != 1)) {
    return false;
  }
  return true;
}

/**
 * only relu node can be removed,
 * or leakyrelu node and attr negative_slope is zero
 */
bool CheckNeedRemoveRelu(const ge::NodePtr &node) {
  if (node->GetType() == RELU) {
    return true;
  }
  if (node->GetType() == LEAKY_RELU) {
    float negative_slope;
    if (!ge::AttrUtils::GetFloat(node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, negative_slope)) {
      FE_LOGD("leakyRelu node[%s] does not have negative slope attr.", node->GetName().c_str());
      return false;
    }
    if (fabs(negative_slope) < kFloatEsp) {
      FE_LOGD("Node[%s]: attr negative_slope == 0", node->GetName().c_str());
      return true;
    }
  }
  return false;
}

Status DealRelu(ge::ComputeGraph &graph, vector<ge::NodePtr> &relu_nodes, const float &scale_quant) {
  for (auto relu_node : relu_nodes) {
    if (relu_node->GetType() == RELU6 && !FloatEqual(scale_quant, 1.0)) {
      relu_node->GetOpDesc()->SetType("Relu6D");
      (void)ge::AttrUtils::SetFloat(relu_node->GetOpDesc(), ATTR_SCALE, scale_quant);
    }
    if (CheckNeedRemoveRelu(relu_node)) {
      Status ret = graph.RemoveNode(relu_node);
      if (ret != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

}  // namespace fe
