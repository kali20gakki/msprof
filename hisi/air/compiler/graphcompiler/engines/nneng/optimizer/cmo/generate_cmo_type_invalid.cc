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

#include "generate_cmo_type_invalid.h"
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/configuration.h"
#include "graph/debug/ge_attr_define.h"

namespace fe {
GenerateCMOTypeInvalid::GenerateCMOTypeInvalid()
    : GenerateCMOTypeBase() {
}

bool GenerateCMOTypeInvalid::CheckReadDistance(const ge::OpDescPtr &op_desc,
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
  int32_t data_vist_dist_from_pre_node = INT_MAX;
  if (!data_visit_dist_vec.empty()) {
    data_vist_dist_from_pre_node = data_visit_dist_vec[0];
  }
  FE_LOGD("Op[name=%s,type=%s,input=%s]: visit pre node distance:%d, threshold:%d", op_name.c_str(), op_type.c_str(),
          op_desc->GetName().c_str(), data_vist_dist_from_pre_node, data_visit_dist_threshold);
  return data_vist_dist_from_pre_node < data_visit_dist_threshold;
}

void GenerateCMOTypeInvalid::LabeledInvalidOrBarrier(const ge::NodePtr &src_node,
                                                     const CmoAttr &attr,
                                                     const std::string &cmo_type) const {
  std::vector<CmoAttr> vec_attr{};
  auto src_op_desc = src_node->GetOpDesc();
  vec_attr.emplace_back(attr);
  AddToNodeCmoAttr(src_op_desc, cmo_type, vec_attr);
  FE_LOGD("Op[name=%s, type=%s] for Op[name=%s, object=%s, index=%d], add label:%s",
          src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str(),
          attr.node->GetOpDesc()->GetName().c_str(),
          (attr.object == CmoTypeObject::OUTPUT ? "output" : "workspace"),
          attr.object_index, cmo_type.c_str());
}

bool GenerateCMOTypeInvalid::CheckReuseDistance(const ge::NodePtr &src_node,
                                                const ge::NodePtr &dst_node,
                                                const ge::NodePtr &last_use_node) const {
  int32_t src_stream_index = -1;
  int32_t dst_stream_index = -1;
  (void)ge::AttrUtils::GetInt(last_use_node->GetOpDesc(), ge::ATTR_NAME_OP_READ_WRITE_INDEX, src_stream_index);
  (void)ge::AttrUtils::GetInt(dst_node->GetOpDesc(), ge::ATTR_NAME_OP_READ_WRITE_INDEX, dst_stream_index);
  int32_t reuse_threshold = Configuration::Instance(AI_CORE_NAME).GetMemReuseDistThreshold();
  FE_LOGD("SrcOp[name=%s, type=%s] has been reused by DstOp[name=%s, type=%s], UseOp[name=%s, type=%s]",
          src_node->GetOpDesc()->GetName().c_str(), src_node->GetOpDesc()->GetType().c_str(),
          dst_node->GetOpDesc()->GetName().c_str(), dst_node->GetOpDesc()->GetType().c_str(),
          last_use_node->GetOpDesc()->GetName().c_str(), last_use_node->GetOpDesc()->GetType().c_str());

  uint32_t src_stream_id = last_use_node->GetOpDesc()->GetStreamId();
  uint32_t dst_stream_id = dst_node->GetOpDesc()->GetStreamId();
  FE_LOGD("UseOp stream_index:%d, stream_id:%u, DstOp stream_index:%d, stream_id:%u, threshold:%d",
          src_stream_index, src_stream_id, dst_stream_index, dst_stream_id, reuse_threshold);

  if (src_stream_id != dst_stream_id) {
    return false;
  }
  return (dst_stream_index - src_stream_index) >= reuse_threshold;
}

void GenerateCMOTypeInvalid::CheckReuseDistanceAndLabeled(const ge::NodePtr &node,
                                                          const ge::NodePtr &pre_node,
                                                          const CmoTypeObject cmo_type_obj,
                                                          const int32_t index) const {
  const ge::NodePtr& dst_node = (cmo_type_obj == CmoTypeObject::OUTPUT ? pre_node : node);
  CmoAttr attr{dst_node, cmo_type_obj, index};
  auto dst_op_desc = dst_node->GetOpDesc();
  std::map<std::string, std::vector<ge::MemReuseInfo>> mem_reuse_info{};
  mem_reuse_info = dst_op_desc->TryGetExtAttr(ge::ATTR_NAME_MEMORY_REUSE_INFO, mem_reuse_info);
  if (mem_reuse_info.empty()) {
    FE_LOGD("Op[name=%s, type=%s] no mem reuse attr", dst_op_desc->GetName().c_str(),
            dst_op_desc->GetType().c_str());
    return;
  }
  std::string tmp_key = (cmo_type_obj == CmoTypeObject::OUTPUT ? "output" : "workspace");
  std::string mem_info_key = tmp_key + std::to_string(index);
  if (mem_reuse_info.find(mem_info_key) == mem_reuse_info.end() || mem_reuse_info[mem_info_key].empty()) {
    FE_LOGD("Op[name=%s, type=%s, key=%s] gets no mem reuse", dst_op_desc->GetName().c_str(),
            dst_op_desc->GetType().c_str(), mem_info_key.c_str());
    return;
  }
  ge::MemReuseInfo &first_reuse_info = mem_reuse_info[mem_info_key][0];
  ge::NodePtr &mem_reuse_node = first_reuse_info.node;
  if (!IsAiCoreOp(mem_reuse_node)) {
    return;
  }
  if (!CheckReuseDistance(dst_node, mem_reuse_node, node)) {
    return;
  }
  LabeledInvalidOrBarrier(node, attr, kCmoInvalid);
  LabeledInvalidOrBarrier(mem_reuse_node, attr, kCmoBarrier);
}

void GenerateCMOTypeInvalid::GenerateInput(const ge::NodePtr &node) const {
  auto op_desc_ptr = node->GetOpDesc();
  std::vector<CmoAttr> vec_attr;
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    if (!CheckParentOpIsAiCore(in_data_anchor)) {
      FE_LOGD("Current Op[name=%s, type=%s,input=%d] has no parent or parent is not aicore op",
              op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(),
              in_data_anchor->GetIdx());
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

    auto out_anchor = in_data_anchor->GetPeerOutAnchor();
    auto out_node   = out_anchor->GetOwnerNode();
    auto out_idx    = out_anchor->GetIdx();
    CheckReuseDistanceAndLabeled(node, out_node, CmoTypeObject::OUTPUT, out_idx);
  }
  return;
}

void GenerateCMOTypeInvalid::GenerateWorkSpace(const ge::NodePtr &node) const {
  if (!IsAiCoreOp(node)) {
    FE_LOGD("Current Op[type=%s] is not Aicore Op", node->GetOpDesc()->GetType().c_str());
    return;
  }
  auto op_desc_ptr = node->GetOpDesc();
  for (size_t work_idx = 0; work_idx < op_desc_ptr->GetWorkspace().size(); work_idx++) {
    CheckReuseDistanceAndLabeled(node, nullptr, CmoTypeObject::WORKSPACE, static_cast<int>(work_idx));
  }
  return;
}

void GenerateCMOTypeInvalid::GenerateType(const ge::NodePtr &node) {
  /**
   * input check the parent output mem reuse info
   * workspace check current node mem reuse info
   */
  FE_LOGD("begin to generate invalid for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  GenerateInput(node);
  GenerateWorkSpace(node);
  FE_LOGD("end to generate invalid for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  return;
}
} // namespace fe
