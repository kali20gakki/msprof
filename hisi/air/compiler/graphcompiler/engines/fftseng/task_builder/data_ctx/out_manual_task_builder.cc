/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "out_manual_task_builder.h"

namespace ffts {
OutTaskBuilder::OutTaskBuilder() {}

OutTaskBuilder::OutTaskBuilder(CACHE_OPERATION operation)
    : DataTaskBuilder(operation) {}

OutTaskBuilder::~OutTaskBuilder() {}

Status OutTaskBuilder::UptSuccListOfRelatedNode(const ge::NodePtr &node, const std::vector<uint32_t> &succ_list,
                                                domi::FftsPlusTaskDef *ffts_plus_task_def) const {
  /* If original context size is 9 and after generate this out data context,
   * the total size is 10. So the context id of this out data context is 10 - 1 = 9; */
  auto data_ctx_id = ffts_plus_task_def->ffts_plus_ctx_size() - 1;
  FFTS_LOGD("Data context id is %d.", data_ctx_id);
  vector<uint32_t> target_ctx_ids;
  if (operation_ == CACHE_OPERATION::WRITE_BACK) {
    auto op = node->GetOpDesc();
    int ctx_id = -1;
    if (ge::AttrUtils::GetInt(op, kContextId, ctx_id)) {
      target_ctx_ids.emplace_back(ctx_id);
    }
  } else {
    target_ctx_ids = succ_list;
  }
  for (auto target_id : target_ctx_ids) {
    UpdateSuccList(data_ctx_id, target_id, ffts_plus_task_def);
  }
  return SUCCESS;
}

Status OutTaskBuilder::GetSuccessorContextId(uint32_t out_anchor_index, const ge::NodePtr &node,
                                             std::vector<uint32_t> &succ_list, uint32_t &cons_cnt) {
  cons_cnt = 0;
  auto anchors = node->GetAllOutDataAnchors();
  auto output_size = anchors.size();
  if (out_anchor_index >= output_size) {
    REPORT_FFTS_ERROR("[GenTask][DataTskBuilder][GetSuccList]Output anchor index %u >= output size %zu of %s.",
                      out_anchor_index, output_size, node->GetName().c_str());
    return FAILED;
  }

  auto output_anchor = anchors.at(out_anchor_index);
  if (output_anchor == nullptr) {
    return SUCCESS;
  }
  for (const auto &peer_in_anchor : output_anchor->GetPeerInDataAnchors()) {
    FFTS_CHECK_NOTNULL(peer_in_anchor);
    auto peer_in_node = peer_in_anchor->GetOwnerNode();
    FFTS_CHECK_NOTNULL(peer_in_node);
    vector<uint32_t> peer_in_context_id;
    uint32_t ctx_id_tmp = 0;
    auto peer_op = peer_in_node->GetOpDesc();
    /* PhonyConcat is no-task op. We need to penetrate it
     * and find its successors. */
    if (peer_op->GetType() == kTypePhonyConcat) {
      FFTS_LOGD("Peer input op for output %d of %s is PhonyConcat %s.", peer_in_anchor->GetIdx(),
                node->GetName().c_str(), peer_op->GetName().c_str());
      for (const auto &peer_node_of_pc : peer_in_node->GetOutDataNodes()) {
        auto peer_op_of_pc = peer_node_of_pc->GetOpDesc();
        if (!ge::AttrUtils::GetInt(peer_op_of_pc, kContextId, ctx_id_tmp)) {
          FFTS_LOGI("PhonyConcat %s peer op %s needs successor list but it do not have a context id.",
                    peer_op->GetName().c_str(), peer_op_of_pc->GetName().c_str());
          continue;
        }
        FFTS_LOGD("Peer input op for PhonyConcat is %s, context id is %u.",
                  peer_op_of_pc->GetName().c_str(), ctx_id_tmp);
        peer_in_context_id.emplace_back(ctx_id_tmp);
      }
    } else {
      if (!ge::AttrUtils::GetInt(peer_op, kContextId, ctx_id_tmp)) {
        FFTS_LOGI("Node %s needs successor list but it has a successor %s which do not have a context id.",
                  node->GetName().c_str(), peer_op->GetName().c_str());
        continue;
      }
      FFTS_LOGD("Peer input op of %s is %s, context id is %u.",
                node->GetName().c_str(), peer_op->GetName().c_str(), ctx_id_tmp);
      peer_in_context_id.emplace_back(ctx_id_tmp);
    }

    for (auto ele : peer_in_context_id) {
      succ_list.emplace_back(ele);
      cons_cnt++;
    }
    FFTS_LOGD("Total successors(%zu) for node %s is %s.", succ_list.size(), node->GetName().c_str(),
              fe::StringUtils::IntegerVecToString(succ_list).c_str());
  }
  return SUCCESS;
}

Status OutTaskBuilder::FillManualDataCtx(size_t out_anchor_index, const ge::NodePtr &node,
                                         const DataContextParam &param, domi::FftsPlusTaskDef *ffts_plus_task_def,
                                         domi::FftsPlusDataCtxDef *data_ctx_def) {
  auto op_desc = node->GetOpDesc();
  uint32_t context_id = 0;
  if (!ge::AttrUtils::GetInt(op_desc, kContextId, context_id)) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillCtxt][node %s, type %s] Cannot get context id for this node.",
                      node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }

  data_ctx_def->set_aten(0);
  std::vector<uint32_t> succ_list;
  if (operation_ == CACHE_OPERATION::WRITE_BACK) {
    /* In write back case, the write back data context can only be
     * executed when the only producer has already been executed. */
    data_ctx_def->set_cnt_init(1);
    data_ctx_def->set_cnt(1);
    FFTS_LOGD("Fill write back for node %s, consumer count is 1.", node->GetName().c_str());
  } else {
    /* In invalidate case, the invalidate data context can only be
     * executed when all the consumers have already been executed. */
    uint32_t cons_cnt = 0;
    GetSuccessorContextId(out_anchor_index, node, succ_list, cons_cnt);
    data_ctx_def->set_cnt_init(cons_cnt);
    data_ctx_def->set_cnt(cons_cnt);
    data_ctx_def->set_orig_consumer_counter(cons_cnt);
    data_ctx_def->set_run_consumer_counter(cons_cnt);
    FFTS_LOGD("Fill invalidate for node %s, consumer count is %u.", node->GetName().c_str(),
              cons_cnt);
  }

  UptSuccListOfRelatedNode(node, succ_list, ffts_plus_task_def);

  vector<int64_t> output_addrs;
  if (!ge::AttrUtils::GetListInt(op_desc, "output_addrs", output_addrs)) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillCtxt][node %s, type %s]Attr output_addrs is empty.",
                      node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }

  if (out_anchor_index >= output_addrs.size()) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillCtxt][node %s, type %s]out anchor %zu is >= the size of output_addrs %zu.",
                      node->GetName().c_str(), node->GetType().c_str(),
                      out_anchor_index, output_addrs.size());
    return FAILED;
  }
  data_ctx_def->set_addr_base(output_addrs[out_anchor_index]);
  data_ctx_def->set_addr_offset(param.base_addr_offset);
  FFTS_LOGD("Base Addr for output %zu is %ld, offset is %ld", out_anchor_index, output_addrs[out_anchor_index],
            param.base_addr_offset);
  FillManualThreadingParam(param, data_ctx_def);
  FFTS_LOGI("Finish generate %d context for node %s.", static_cast<int>(operation_), node->GetName().c_str());
  return SUCCESS;
}
}