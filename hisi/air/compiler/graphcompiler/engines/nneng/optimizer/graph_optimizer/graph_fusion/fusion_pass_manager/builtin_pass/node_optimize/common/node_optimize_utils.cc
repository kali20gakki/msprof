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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/common/node_optimize_utils.h"

namespace fe {
Status NodeOptimizeUtils::GetPreNode(const ge::NodePtr &node_ptr, const int &index, ge::NodePtr &pre_node_ptr) {
  ge::InDataAnchorPtr in_data_anchor = node_ptr->GetInDataAnchor(index);
  FE_CHECK_NOTNULL(in_data_anchor);
  ge::OutDataAnchorPtr peer_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(peer_out_data_anchor);
  pre_node_ptr = peer_out_data_anchor->GetOwnerNode();
  FE_CHECK_NOTNULL(pre_node_ptr);
  return SUCCESS;
}
}  // namespace fe
