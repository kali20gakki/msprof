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

#include "graph/passes/subgraph_multi_dims_pass.h"

#include "graph/utils/type_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
Status SubgraphMultiDimsPass::Run(ComputeGraphPtr graph) {
  GELOGD("SubgraphMultiDimsPass Start");

  if (graph->GetParentNode() != nullptr) {
    return SUCCESS;
  }

  for (const auto &subgraph : graph->GetAllSubgraphs()) {
    bool is_subgraph_multi_batch = false;
    (void)AttrUtils::GetBool(subgraph, ATTR_NAME_SUBGRAPH_IS_MULTI_DIMS, is_subgraph_multi_batch);
    if (!is_subgraph_multi_batch) {
      continue;
    }
    GELOGI("Proc multi dims subgraph[%s].", subgraph->GetName().c_str());
    const auto &case_node = subgraph->FindFirstNodeMatchType(CASE);
    GE_CHECK_NOTNULL(case_node);
    const auto &func_desc = case_node->GetOpDesc();
    GE_CHECK_NOTNULL(func_desc);
    if (!func_desc->HasAttr(ATTR_NAME_BATCH_NUM)) {
      GELOGD("Subgraph[%s] is not multi-dims, cause case[%s] has no batch_num attr",
             subgraph->GetName().c_str(), case_node->GetName().c_str());
      continue;
    }

    const auto &dynamic_branch_names = func_desc->GetSubgraphInstanceNames();
    for (size_t i = 0; i < dynamic_branch_names.size(); ++i) {
      const auto &sub_graph = graph->GetSubgraph(dynamic_branch_names[i]);
      GE_CHECK_NOTNULL(sub_graph);

      const std::string batch_label = "Batch_" + std::to_string(i);
      GE_CHK_STATUS_RET(CreateUpdateTensorDescNodes(sub_graph, batch_label),
                        "Insert UpdateTensorDesc for sub_graph[%s] failed.", sub_graph->GetName().c_str());
      for (const auto &node : sub_graph->GetDirectNode()) {
        (void)AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_BATCH_LABEL, batch_label);
      }
    }
  }

  GELOGD("SubgraphMultiDimsPass end");
  return SUCCESS;
}

Status SubgraphMultiDimsPass::CreateUpdateTensorDescNodes(const ComputeGraphPtr &subgraph,
                                                          const std::string &label) const {
  GELOGD("CreateUpdateTensorDescNodes start for subgraph[%s].", subgraph->GetName().c_str());

  const auto &output_node = subgraph->FindFirstNodeMatchType(NETOUTPUT);
  for (auto input_anchor : output_node->GetAllInDataAnchors()) {
    if (input_anchor == nullptr) {
      continue;
    }
    auto peer_anchor = input_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    const auto &peer_node = peer_anchor->GetOwnerNode();
    if (peer_node == nullptr) {
      continue;
    }
    const auto &peer_desc = peer_node->GetOpDesc();
    GE_CHECK_NOTNULL(peer_desc);
    const auto &peer_tensor = peer_desc->GetOutputDescPtr(peer_anchor->GetIdx());
    GE_CHECK_NOTNULL(peer_tensor);
    OpDescBuilder op_builder("subgraph_mbatch_update_tensor_desc_" + label + "_" +
                             std::to_string(input_anchor->GetIdx()), "UpdateTensorDesc");
    op_builder.AddInput("x", GeTensorDesc(*peer_tensor))
              .AddOutput("y", GeTensorDesc(*peer_tensor));

    const OpDescPtr op_desc = op_builder.Build();
    GE_CHECK_NOTNULL(op_desc);
    (void)AttrUtils::SetBool(op_desc, ATTR_INSERT_BY_MBATCH, true);
    (void)AttrUtils::SetListInt(op_desc, "shape", peer_tensor->GetShape().GetDims());
    const auto &output_tensor = op_desc->MutableOutputDesc(0U);
    TensorUtils::SetReuseInput(*output_tensor, true);
    TensorUtils::SetReuseInputIndex(*output_tensor, 0U);
    // mark UpdateTensorDesc's output tensor use tensor desc memory type
    (void)AttrUtils::SetBool(output_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    const NodePtr update_tensor_desc_node = subgraph->AddNode(op_desc);
    GE_CHECK_NOTNULL(update_tensor_desc_node);

    // mark netOutput's input tensor use tensor desc memory type
    const auto &output_desc = output_node->GetOpDesc();
    GE_CHECK_NOTNULL(output_desc);
    const auto &input_tensor = output_desc->MutableInputDesc(input_anchor->GetIdx());
    (void)AttrUtils::SetBool(input_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);

    if (GraphUtils::RemoveEdge(peer_anchor, input_anchor)) {
      REPORT_INNER_ERROR("E19999", "RemoveEdge for node[%s] out data anchor[%d] and node[%s] in data anchor[%d]"
                         " failed.", peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                         output_node->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(FAILED, "RemoveEdge for node[%s] out data anchor[%d] and node[%s] in data anchor[%d] failed.",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(),
             output_node->GetName().c_str(), input_anchor->GetIdx());
      return FAILED;
    }
    // add data edge for UpdateTensorDesc and pre node
    if (GraphUtils::AddEdge(peer_anchor, update_tensor_desc_node->GetInDataAnchor(0)) != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "AddEdge for node[%s] out data anchor[%d] and node[%s] in data anchor[0] failed.",
                         peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                         update_tensor_desc_node->GetName().c_str());
      GELOGE(FAILED, "AddEdge for node[%s] out data anchor[%d] and node[%s] in data anchor[0] failed.",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(), update_tensor_desc_node->GetName().c_str());
      return FAILED;
    }
    // add data edge for UpdateTensorDesc and NetOutput
    if (GraphUtils::AddEdge(update_tensor_desc_node->GetOutDataAnchor(0), input_anchor) != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "AddEdge for node[%s] out data anchor[0] and node[%s] in data anchor[%d] failed.",
                         update_tensor_desc_node->GetName().c_str(),
                         output_node->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(FAILED, "AddEdge for node[%s] out data anchor[0] and node[%s] in control anchor[%d] failed.",
             update_tensor_desc_node->GetName().c_str(), output_node->GetName().c_str(), input_anchor->GetIdx());
      return FAILED;
    }
  }

  GELOGD("CreateUpdateTensorDescNodes end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}
}  // namespace ge
