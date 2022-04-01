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

#include "generate_cmo_type_writeback.h"
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/configuration.h"

namespace fe {
GenerateCMOTypeWriteback::GenerateCMOTypeWriteback()
    : GenerateCMOTypeBase() {
}

bool GenerateCMOTypeWriteback::CheckReadDistance(const ge::OpDescPtr &op_desc,
                                                 const ge::InDataAnchorPtr &in_anchor) const {
  auto op_name    = op_desc->GetName();
  auto op_type    = op_desc->GetType();
  auto in_idx     = in_anchor->GetIdx();
  auto input_desc = op_desc->MutableInputDesc(in_idx);
  if (!ge::AttrUtils::HasAttr(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE)) {
    return false;
  }

  vector<int32_t> data_visit_dist_vec;
  (void)ge::AttrUtils::GetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, data_visit_dist_vec);

  auto data_visit_dist_threshold = Configuration::Instance(AI_CORE_NAME).GetDataVisitDistThreshold();
  int32_t data_vist_dist_from_pre_node = -1;
  if (!data_visit_dist_vec.empty()) {
    data_vist_dist_from_pre_node = data_visit_dist_vec[0];
  }
  FE_LOGD("Op[name=%s,type=%s,input=%d]: parent node visit distance:%d, threshold:%d", op_name.c_str(), op_type.c_str(),
          in_idx, data_vist_dist_from_pre_node, data_visit_dist_threshold);
  return data_vist_dist_from_pre_node >= data_visit_dist_threshold;
}

void GenerateCMOTypeWriteback::LabeledWriteback(const ge::InDataAnchorPtr &in_anchor) const {
  auto out_anchor  = in_anchor->GetPeerOutAnchor();
  auto out_idx     = out_anchor->GetIdx();
  auto src_node    = out_anchor->GetOwnerNode();
  auto src_op_desc = src_node->GetOpDesc();
  auto dst_node    = out_anchor->GetOwnerNode();
  auto dst_op_desc = dst_node->GetOpDesc();
  CmoAttr attr{dst_node, CmoTypeObject::OUTPUT, out_idx};
  std::vector<CmoAttr> vec_attr{attr};
  AddToNodeCmoAttr(src_op_desc, kCmoWriteback, vec_attr);
  FE_LOGD("Op[name=%s, type=%s, output=%d] for Op[name=%s, type=%s] add label:Writeback",
          src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str(), out_idx,
          dst_op_desc->GetName().c_str(), dst_op_desc->GetType().c_str());
  return;
}

void GenerateCMOTypeWriteback::GenerateType(const ge::NodePtr &node) {
  /**
   * only writeback output
   * parent node is aicore
   * writeback label on its own
   */
  FE_LOGD("begin to generate writeback for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  auto op_desc_ptr = node->GetOpDesc();
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    if (!CheckParentOpIsAiCore(in_data_anchor)) {
      FE_LOGD("Current Op[name=%s, type=%s, input=%d] has no parent or parent is not aicore op",
              op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), in_data_anchor->GetIdx());
      continue;
    }

    if (!ReadIsLifeCycleEnd(node, in_data_anchor)) {
      FE_LOGD("Op[name=%s,type=%s] life_end_cycle flag is false", op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str());
      continue;
    }

    if (!CheckReadDistance(op_desc_ptr, in_data_anchor)) {
      continue;
    }
    LabeledWriteback(in_data_anchor);
  }
  FE_LOGD("end to generate writeback for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  return;
}
} // namespace fe
