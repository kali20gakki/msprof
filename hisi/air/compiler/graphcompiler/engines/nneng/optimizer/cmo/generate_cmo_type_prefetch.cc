/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "generate_cmo_type_prefetch.h"
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/configuration.h"
#include "common/graph/fe_graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {
GenerateCMOTypePrefetch::GenerateCMOTypePrefetch()
    : GenerateCMOTypeBase() {
}

ge::NodePtr GenerateCMOTypePrefetch::GetLastPreNode(const ge::NodePtr &node) const {
  int32_t topo_id = -1;
  ge::NodePtr nearest_in_node = nullptr;
  for (auto const &in_node : node->GetInDataNodes()) {
    if (in_node == nullptr || in_node->GetOpDesc() == nullptr) {
      continue;
    }
    auto in_node_desc = in_node->GetOpDesc();
    if (IsAiCoreOp(in_node) && in_node_desc->GetId() > topo_id) {
      nearest_in_node = in_node;
      topo_id = in_node_desc->GetId();
    }
  }
  return nearest_in_node;
}

void GenerateCMOTypePrefetch::LabeledPrefetch(const ge::NodePtr &src_node,
                                              const ge::NodePtr &dst_node) {
  std::vector<CmoAttr> vec_attr;
  ge::NodePtr tmp_node;
  for (const auto &in_data_anchor : dst_node->GetAllInDataAnchors()) {
    if (FeGraphUtils::IsPeerOutWeight(dst_node.get(), in_data_anchor->GetIdx(), tmp_node)) {
      bool is_cached = prefetch_cache_map_.find(tmp_node) != prefetch_cache_map_.end() &&
                       !CheckNeedPretch(prefetch_cache_map_[tmp_node], dst_node);
      if (is_cached) {
        FE_LOGD("Op[name=%s] has just been prefetched by Op[name=%s], no need prefetch", tmp_node->GetName().c_str(),
                prefetch_cache_map_[tmp_node]->GetName().c_str());
        continue;
      }
      CmoAttr attr{dst_node, CmoTypeObject::WEIGHT, in_data_anchor->GetIdx()};
      vec_attr.emplace_back(attr);
      prefetch_cache_map_[tmp_node] = dst_node;
    }
  }

  if (vec_attr.empty()) {
    return;
  }
  auto src_op_desc = src_node->GetOpDesc();
  auto dst_op_desc = dst_node->GetOpDesc();
  AddToNodeCmoAttr(src_op_desc, kCmoPrefetch, vec_attr);
  FE_LOGD("Op[name=%s, type=%s] for Op[name=%s, type=%s] add label:Prefetch", src_op_desc->GetName().c_str(),
          src_op_desc->GetType().c_str(), dst_op_desc->GetName().c_str(), dst_op_desc->GetType().c_str());
  return;
}

bool GenerateCMOTypePrefetch::CheckNeedPretch(const ge::NodePtr &src_node, const ge::NodePtr &dst_node) const {
  int32_t src_index = -1;
  int32_t dst_index = -1;
  (void)ge::AttrUtils::GetInt(src_node->GetOpDesc(), ge::ATTR_NAME_OP_READ_WRITE_INDEX, src_index);
  (void)ge::AttrUtils::GetInt(dst_node->GetOpDesc(), ge::ATTR_NAME_OP_READ_WRITE_INDEX, dst_index);
  int32_t threshold = Configuration::Instance(AI_CORE_NAME).GetMemReuseDistThreshold();
  FE_LOGD("Op[name=%s, type=%s, index=%d] and Op[name=%s, type=%s, index=%d] has same weight, threadhold=%d",
          src_node->GetName().c_str(), src_node->GetType().c_str(), src_index,
          dst_node->GetName().c_str(), dst_node->GetType().c_str(), dst_index,
          threshold);
  return dst_index - src_index >= threshold;
}

bool GenerateCMOTypePrefetch::CheckSizeIsAvailable(const ge::NodePtr &src_node, const ge::NodePtr &dst_node) const {
  int64_t src_node_input_size = GetInputTensorSize(src_node->GetOpDesc());
  int64_t src_node_output_size = GetOutputTensorSize(src_node->GetOpDesc());
  int64_t src_node_workspace_size = GetWorkSpaceSize(src_node->GetOpDesc());
  int64_t cur_node_total_weight_size = GetWeightSize(dst_node);
  int64_t cache_size = GetCacheSize();
  FE_LOGD("Op[%s] with Input Op[%s], Input node input_size:%ld, output_size:%ld, \
          workspace_size:%ld, Current node weight_size:%ld, Total cache_size:%ld",
          dst_node->GetOpDesc()->GetName().c_str(), src_node->GetOpDesc()->GetName().c_str(),
          src_node_input_size, src_node_output_size, src_node_workspace_size,
          cur_node_total_weight_size, cache_size);
  return (src_node_input_size + src_node_output_size + src_node_workspace_size +
          cur_node_total_weight_size <= cache_size);
}

void GenerateCMOTypePrefetch::GenerateType(const ge::NodePtr &node) {
  /**
   * only prefetch weight input
   * prefetch lable on parent node
   */
  FE_LOGD("begin to generate prefetch for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  vector<ge::ConstGeTensorPtr> weights = ge::OpDescUtils::GetWeights(*node);
  if (weights.empty()) {
    FE_LOGD("end to generate prefetch for node:[name=%s, type=%s], reason:no weights",
            node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
    return;
  }

  ge::NodePtr peer_out_node = GetLastPreNode(node);
  if (peer_out_node == nullptr) {
    FE_LOGD("end to generate prefetch for node:[name=%s, type=%s], reason:no parent node",
            node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
    return;
  }

  if (!CheckSizeIsAvailable(peer_out_node, node)) {
    FE_LOGD("end to generate prefetch for node:[name=%s, type=%s], reason:size not available",
            node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
    return;
  }

  LabeledPrefetch(peer_out_node, node);
  FE_LOGD("end to generate prefetch for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  return;
}
} // namespace fe
