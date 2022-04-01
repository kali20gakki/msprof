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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_util.h"
#include <string>
#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"

namespace fe {

Status DealDequantNotRequantV200(ge::ComputeGraph &graph, vector<ge::NodePtr> &dequant_nodes,
                                 float &scale_quant, vector<ge::NodePtr> &new_nodes) {
  FE_LOGD("Start to do dequant not requant in V200");
  std::string quant_mode;
  for (size_t iter = 0; iter < dequant_nodes.size(); iter++) {
    // get deqaunt node quant mode
    (void)ge::AttrUtils::GetStr(dequant_nodes[iter]->GetOpDesc(), STR_QUANT_MODE, quant_mode);
    FE_LOGD("Set relu flag to dequant node.");
    if (SetReluFlagToDequant(dequant_nodes[iter]) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealDeqntNotReQntV200] Set relu flag to node %s failed.",
                      dequant_nodes[iter]->GetName().c_str());
      return FAILED;
    }
    /* Create Host Cpu Op */
    FE_LOGD("Create host op to calc deq_scale of node:[%s].", dequant_nodes[iter]->GetName().c_str());
    Status ret = CreateNewRequantHostCpuOp(REQUANT_HOST_CPU_OP_V2, dequant_nodes[iter], scale_quant, graph, new_nodes);
    if ((ret != SUCCESS || new_nodes.empty())) {
      REPORT_FE_ERROR("[GraphOpt][V200ReqntFus][DealDeqntNotReQntV200] Create host cpu op for dequant node %s failed",
                      dequant_nodes[iter]->GetName().c_str());
      return ret;
    }

    auto host_cpu_node = dequant_nodes[iter]->GetInDataNodes().at(1);
    bool relu_flag = false;
    (void)ge::AttrUtils::GetBool(dequant_nodes[iter]->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
    (void)ge::AttrUtils::SetBool(host_cpu_node->GetOpDesc(), ATTR_RELU_FLAG, relu_flag);
    (void)ge::AttrUtils::SetStr(host_cpu_node->GetOpDesc(), STR_QUANT_MODE, quant_mode);
  }
  return SUCCESS;
}

}  // namespace fe
