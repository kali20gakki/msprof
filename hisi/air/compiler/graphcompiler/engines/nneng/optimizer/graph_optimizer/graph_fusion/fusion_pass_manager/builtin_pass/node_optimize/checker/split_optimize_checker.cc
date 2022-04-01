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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/split_optimize_checker.h"
#include "common/unknown_shape_util.h"

namespace fe {
bool SplitOptimizeChecker::Check(const ge::NodePtr &node_ptr) const {
  return IsMultiDataOutputs(node_ptr) && IsDimC(node_ptr, SPLIT_DIM, false) && IsDimCAligned(node_ptr) &&
         IsInputNotData(node_ptr) && !IsUnKnownShapeOp(*(node_ptr->GetOpDesc().get()));
}
bool SplitOptimizeChecker::IsMultiDataOutputs(const ge::NodePtr &node_ptr) const {
  return node_ptr->GetAllOutDataAnchors().size() > 1;
}

bool SplitOptimizeChecker::IsDimCAligned(const ge::NodePtr &node_ptr) const {
  string node_name = node_ptr->GetName();
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  size_t output_size = op_desc_ptr->GetOutputsSize();
  for (size_t i = 0; i != output_size; i++) {
    ge::GeTensorDesc tensor_desc = op_desc_ptr->GetOutputDesc(i);
    int dim_c = 0;
    Status status = GetDimC(tensor_desc, dim_c);
    if (status != SUCCESS) {
      FE_LOGD(
          "Node[%s]: get the dim C of the input [%zu] not success, "
          "check failed.",
          node_name.c_str(), i);
      return false;
    }
    ge::NodePtr split_next_node = nullptr;
    ge::DataType quant_data_type = tensor_desc.GetOriginDataType();
    if (node_ptr->GetOutDataAnchor(0) != nullptr && !node_ptr->GetOutDataAnchor(0)->GetPeerInDataAnchors().empty()) {
      ge::OutDataAnchor::Vistor<ge::InDataAnchorPtr> peer_in_data_anchors =
          node_ptr->GetOutDataAnchor(0)->GetPeerInDataAnchors();
      ge::InDataAnchorPtr in_data_anchor_ptr = peer_in_data_anchors.at(0);
      if (in_data_anchor_ptr != nullptr && in_data_anchor_ptr->GetOwnerNode()->GetType() == QUANT) {
        auto quant_node = in_data_anchor_ptr->GetOwnerNode();
        FE_CHECK(quant_node == nullptr || quant_node->GetOpDesc()== nullptr,
          FE_LOGD("Get quant op desc failed."), return FAILED);
        quant_data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
      }
    }
    if (!IsDimCOfInputAligned(tensor_desc, dim_c, quant_data_type)) {
      FE_LOGD("Node[%s]: the dim C of the input [%zu] is not aliged, check failed.", node_name.c_str(), i);
      return false;
    }
  }
  return true;
}
}  // namespace fe
