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

#include "data_task_builder.h"

namespace ffts {
const int64_t kMaxBurstLen = 1000000;
std::map<CACHE_OPERATION, std::tuple<string, ContextType, bool, string>> kDataOptInfo;

DataTaskBuilder::DataTaskBuilder() : operation_(CACHE_OPERATION::CACHE_OPERATION_BOTTOM) {
  kDataOptInfo[CACHE_OPERATION::PREFETCH] = std::make_tuple(kPrefetchEnableBm, RT_CTX_TYPE_FLUSH_DATA, true,
                                                            "prefetch");
  kDataOptInfo[CACHE_OPERATION::INVALIDATE] = std::make_tuple(kInvalidateBm, RT_CTX_TYPE_INVALIDATE_DATA, false,
                                                              "invalidate");
  kDataOptInfo[CACHE_OPERATION::WRITE_BACK] = std::make_tuple(kWriteBackBm, RT_CTX_TYPE_WRITEBACK_DATA, false,
                                                              "write back");
}

DataTaskBuilder::DataTaskBuilder(CACHE_OPERATION operation) : operation_(operation), burst_len_(kMaxBurstLen) {
  kDataOptInfo[CACHE_OPERATION::PREFETCH] = std::make_tuple(kPrefetchEnableBm, RT_CTX_TYPE_FLUSH_DATA, true,
                                                            "prefetch");
  kDataOptInfo[CACHE_OPERATION::INVALIDATE] = std::make_tuple(kInvalidateBm, RT_CTX_TYPE_INVALIDATE_DATA, false,
                                                              "invalidate");
  kDataOptInfo[CACHE_OPERATION::WRITE_BACK] = std::make_tuple(kWriteBackBm, RT_CTX_TYPE_WRITEBACK_DATA, false,
                                                              "write back");
}

DataTaskBuilder::~DataTaskBuilder() {}

Status DataTaskBuilder::GenManualDataCtxDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("DataTaskBuilder::GenManualContextDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();

  const auto &operation = kDataOptInfo.at(operation_);
  string bm_name = std::get<0>(operation);
  rtFftsPlusContextType_t context_type = std::get<1>(operation);
  bool is_input = std::get<2>(operation);
  string operation_name = std::get<3>(operation);
  int64_t bm = 0;
  if (!ge::AttrUtils::GetInt(op_desc, bm_name, bm) || bm == 0) {
    return SUCCESS;
  }
  FFTS_LOGD("Node %s need %s context.", node->GetName().c_str(), operation_name.c_str());
  auto indices = GetIndices(node);
  size_t curr_num = 0;

  // for static shape
  for (auto index : indices) {
    uint64_t curr_bm = 0;
    SetBitOne(static_cast<uint32_t>(index), curr_bm);
    if (!(bm & curr_bm)) {
      FFTS_LOGD("bm is %lu and curr_bm is %lu", bm, curr_bm);
      continue;
    }

    FFTS_LOGD("Node %s's tensor %d need %s", node->GetName().c_str(), index, operation_name.c_str());

    std::vector<DataContextParam> data_ctx_params;
    Status ret = MemorySlice::GenerateManualDataCtxParam(node, index, is_input, burst_len_, data_ctx_params);
    if (ret != SUCCESS) {
      continue;
    }

    if (ExceedMaxCtxNum(curr_num, data_ctx_params.size())) {
      FFTS_LOGI("Exceed the upper limit of prefetch context. Current total is %zu, pending total is %zu.",
                curr_num, data_ctx_params.size());
      continue;
    }

    curr_num += data_ctx_params.size();
    FFTS_LOGD("Size of context is %zu, curr_num is %zu.", data_ctx_params.size(), curr_num);

    for (auto &param : data_ctx_params) {
      domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
      FFTS_CHECK_NOTNULL(ffts_plus_ctx_def);
      ffts_plus_ctx_def->set_context_type(context_type);
      domi::FftsPlusDataCtxDef *data_ctx_def = ffts_plus_ctx_def->mutable_data_ctx();
      FFTS_CHECK_NOTNULL(data_ctx_def);
      size_t anchor_index = static_cast<size_t>(index);

      FFTS_LOGD("Fill one %s context for tensor %zu of node %s.", operation_name.c_str(),
                anchor_index, node->GetName().c_str());
      Status status = FillManualDataCtx(anchor_index, node, param, ffts_plus_task_def, data_ctx_def);
      if (status != SUCCESS) {
        REPORT_FFTS_ERROR("[GenTask][InvldTsk][GenCtxDef]Fill %s context %zu failed. Op[%s, optype[%s]]",
                          operation_name.c_str(), anchor_index, op_desc->GetName().c_str(), op_desc->GetType().c_str());
        return status;
      }
    }
  }
  return SUCCESS;
}

Status DataTaskBuilder::GenAutoDataCtxDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("DataTaskBuilder::GenAutoContextDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();

  const auto &operation = kDataOptInfo.at(operation_);
  string bm_name = std::get<0>(operation);
  rtFftsPlusContextType_t context_type = std::get<1>(operation);
  bool is_input = std::get<2>(operation);
  string operation_name = std::get<3>(operation);
  int64_t bm = 0;
  if (!ge::AttrUtils::GetInt(op_desc, bm_name, bm) || bm == 0) {
    return SUCCESS;
  }
  FFTS_LOGD("Node %s need %s context.", node->GetName().c_str(), operation_name.c_str());
  auto indices = GetIndices(node);
  size_t curr_num = 0;

  // for static shape
  for (auto index : indices) {
    uint64_t curr_bm = 0;
    SetBitOne(static_cast<uint32_t>(index), curr_bm);
    if (!(bm & curr_bm)) {
      FFTS_LOGD("bm is %lu and curr_bm is %lu", bm, curr_bm);
      continue;
    }

    FFTS_LOGD("Node %s's tensor %d need %s", node->GetName().c_str(), index, operation_name.c_str());

    // one context has nontail and tail params
    std::vector<DataContextParam> param_nontail_tail;
    Status ret = MemorySlice::GenerateAutoDataCtxParam(node, index, is_input, burst_len_, param_nontail_tail);
    if (ret != SUCCESS) {
      continue;
    }

    if (ExceedMaxCtxNum(curr_num, param_nontail_tail.size())) {
      FFTS_LOGI("Exceed the upper limit of prefetch context. Current total is %zu, pending total is %zu.",
                curr_num, param_nontail_tail.size());
      continue;
    }

    curr_num += param_nontail_tail.size();
    FFTS_LOGD("curr_num is %zu, data_ctx_params size is %zu", curr_num, param_nontail_tail.size());

    std::vector<std::vector<DataContextParam>> data_ctx_params;
    ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    FFTS_CHECK_NOTNULL(slice_info_ptr);
    for (size_t i = 0; i < static_cast<size_t>(slice_info_ptr->parallel_window_size); i++) {
      data_ctx_params.push_back(param_nontail_tail);
    }

    for (size_t i = 0; i < data_ctx_params.size(); i++) {
      domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
      FFTS_CHECK_NOTNULL(ffts_plus_ctx_def);
      ffts_plus_ctx_def->set_context_type(context_type);
      domi::FftsPlusDataCtxDef *data_ctx_def = ffts_plus_ctx_def->mutable_data_ctx();
      FFTS_CHECK_NOTNULL(data_ctx_def);
      size_t anchor_index = static_cast<size_t>(index);

      FFTS_LOGD("Fill one %s context for tensor %zu of node %s, window_id: %zu.", operation_name.c_str(),
                anchor_index, node->GetName().c_str(), i);
      Status status = FillAutoDataCtx(anchor_index, node, data_ctx_params[i], ffts_plus_task_def, data_ctx_def, i);
      if (status != SUCCESS) {
        REPORT_FFTS_ERROR("[GenTask][InvldTsk][GenCtxDef]Fill %s context %zu failed. Op[%s, optype[%s]]",
                          operation_name.c_str(), anchor_index, op_desc->GetName().c_str(), op_desc->GetType().c_str());
        return status;
      }
    }
  }
  return SUCCESS;
}

Status DataTaskBuilder::GenDynamicDataCtxDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("DataTaskBuilder::GenDynamicDataCtxDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();

  const auto &operation = kDataOptInfo.at(operation_);
  string bm_name = std::get<0>(operation);
  rtFftsPlusContextType_t context_type = std::get<1>(operation);
  string operation_name = std::get<3>(operation);
  int64_t bm = 0;
  if (!ge::AttrUtils::GetInt(op_desc, bm_name, bm) || bm == 0) {
    return SUCCESS;
  }
  FFTS_LOGD("Node %s need %s context.", node->GetName().c_str(), operation_name.c_str());
  auto indices = GetIndices(node);

  vector<uint32_t> context_id_list;
  (void)ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list);
  std::vector<int32_t> cmo_idx;
  for (auto index : indices) {
    uint64_t curr_bm = 0;
    SetBitOne(static_cast<uint32_t>(index), curr_bm);
    if (!(bm & curr_bm)) {
      FFTS_LOGD("bm is %lu and curr_bm is %lu", bm, curr_bm);
      continue;
    }
    FFTS_LOGD("Node %s's tensor %d need %s", node->GetName().c_str(), index, operation_name.c_str());
    cmo_idx.emplace_back(index);
    Status ret = FillDynamicDataCtx(static_cast<size_t>(index), node, ffts_plus_task_def, context_type,
                                    context_id_list);
    if (ret == FAILED) {
      return FAILED;
    }
  }
  (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), operation_name + "_idx", cmo_idx);
  return SUCCESS;
}

std::vector<int> DataTaskBuilder::GetIndices(const ge::NodePtr &node) const {
  vector<int> indices;
  if (operation_ == CACHE_OPERATION::PREFETCH) {
    for (const auto &in_anchor : node->GetAllInDataAnchors()) {
      if (!in_anchor) {
        continue;
      }
      uint32_t idx = in_anchor->GetIdx();
      if (idx >= 0 && idx < kMaxIdx) {
        indices.emplace_back(in_anchor->GetIdx());
      }
    }
  } else {
    for (const auto &output_anchor : node->GetAllOutDataAnchors()) {
      if (!output_anchor) {
        continue;
      }
      uint32_t idx = output_anchor->GetIdx();
      if (idx < kMaxIdx) {
        indices.emplace_back(idx);
      }
    }
  }
  return indices;
}

bool DataTaskBuilder::ExceedMaxCtxNum(size_t curr_num, size_t pending_num) const {
  if (operation_ == CACHE_OPERATION::PREFETCH) {
    return (curr_num + pending_num) > kMaxPretchNum;
  }
  return false;
}

/* In manual slicing, the memory of one thread node may not be continuous.
 * we need to calculate the following params based on the memory slicing
 * info. */
void DataTaskBuilder::FillManualThreadingParam(const DataContextParam &param,
                                               domi::FftsPlusDataCtxDef *data_ctx_def) const {
  FFTS_LOGD("start to fill Manual threading param.");
  data_ctx_def->set_non_tail_len_inner(param.len_inner);
  data_ctx_def->set_non_tail_num_inner(param.num_inner);
  data_ctx_def->set_non_tail_num_outter(param.num_outter);
  data_ctx_def->set_non_tail_stride_inner(param.stride_inner);
  data_ctx_def->set_non_tail_stride_outter(param.stride_outter);

  data_ctx_def->set_tail_len_inner(param.len_inner);
  data_ctx_def->set_tail_num_inner(param.num_inner);
  data_ctx_def->set_tail_num_outter(param.num_outter);
  data_ctx_def->set_tail_stride_inner(param.stride_inner);
  data_ctx_def->set_tail_stride_outter(param.stride_outter);
}

/* We assume the data address is continuous. So there is no gap
 * between two sgt thread and the offset is equal to size of thread.
 * All auto threads use the same data context and hardware use
 * the thread id to differenciate them.
 *
 * For one non-tail thread the base offset is equal to:
 * non-tail thread dim size * thread_id.
 * For the tail thread the base offset is equal to:
 * non-tail thread dim size * (thread_dim - 1) */
void DataTaskBuilder::FillAutoThreadingParam(const vector<DataContextParam> &params,
                                             domi::FftsPlusDataCtxDef *data_ctx_def, const uint32_t &slice_num) const {
  if (params.size() <= 1 || slice_num <= 1) {
    return ;
  }
  FFTS_LOGD("start to fill auto threading param, params[1].base_addr_offset: %ld, slice_num: %u, addr_offset: %ld.",
            params[1].base_addr_offset, slice_num, params[1].base_addr_offset / (slice_num - 1));
  data_ctx_def->set_addr_offset(params[1].base_addr_offset / (slice_num - 1));
  data_ctx_def->set_non_tail_len_inner(params[0].len_inner);
  data_ctx_def->set_non_tail_num_inner(params[0].num_inner);
  data_ctx_def->set_non_tail_num_outter(params[0].num_outter);
  data_ctx_def->set_non_tail_stride_inner(params[0].stride_inner);
  data_ctx_def->set_non_tail_stride_outter(params[0].stride_outter);

  data_ctx_def->set_tail_len_inner(params[1].len_inner);
  data_ctx_def->set_tail_num_inner(params[1].num_inner);
  data_ctx_def->set_tail_num_outter(params[1].num_outter);
  data_ctx_def->set_tail_stride_inner(params[1].stride_inner);
  data_ctx_def->set_tail_stride_outter(params[1].stride_outter);
}

Status DataTaskBuilder::GetAddrBase(size_t in_anchor_index, const ge::NodePtr &node, uint64_t &addr_base) const {
  vector<int64_t> input_addrs;
  if (!ge::AttrUtils::GetListInt(node->GetOpDesc(), "input_addrs", input_addrs)) {
    REPORT_FFTS_ERROR("[GenTsk][PrefetchTsk][FillCtxt][node %s, type %s]Attr input_addrs is empty.",
                      node->GetName().c_str(), node->GetType().c_str());
    return SUCCESS;
  }

  if (in_anchor_index >= input_addrs.size()) {
    REPORT_FFTS_ERROR("[GenTsk][PrefetchTsk][FillCtxt][node %s, type %s]in anchor %zu is >= the size of input_addrs \
                      %zu.", node->GetName().c_str(), node->GetType().c_str(), in_anchor_index, input_addrs.size());
    return SUCCESS;
  }
  addr_base = static_cast<uint64_t>(input_addrs[in_anchor_index]);
  return SUCCESS;
}


Status DataTaskBuilder::UpdateSrcSlotAndPfBm(domi::FftsPlusTaskDef *ffts_plus_task_def, uint32_t context_id) const {
  FFTS_LOGD("Update src slot and pf bm for context %u.", context_id);
  FFTS_CHECK_NOTNULL(ffts_plus_task_def);
  domi::FftsPlusCtxDef *ctx = ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(context_id));
  FFTS_CHECK_NOTNULL(ctx);
  uint32_t context_type = ctx->context_type();
  uint32_t prefetch_ctx_id = ffts_plus_task_def->ffts_plus_ctx_size() - 1;
  if (context_type == RT_CTX_TYPE_AICORE || context_type == RT_CTX_TYPE_AIV) {
    auto aic_aiv_ctx = ctx->mutable_aic_aiv_ctx();
    return AddSrcSlotAndBmToCtx(prefetch_ctx_id, aic_aiv_ctx);
  } else if (context_type == RT_CTX_TYPE_MIX_AIC || context_type == RT_CTX_TYPE_MIX_AIV) {
    auto mix_aic_aiv_ctx = ctx->mutable_mix_aic_aiv_ctx();
    return AddSrcSlotAndBmToCtx(prefetch_ctx_id, mix_aic_aiv_ctx);
  } else {
    REPORT_FFTS_ERROR("This context type %u, id %u, does not need prefetch.", context_type, context_id);
    return FAILED;
  }
}


/*
 * Just for auto_mode and dynamic_mode
 * Manual_mode will override it.
 *
 * Just record first context_id generate by node(B and C) the reason is that you can update all window context's
 * (generate by A) succ_list according this first record.
 *
 *   For example: a_1's succ_list(5, 9), window size: 4.
 *                     A (1, 2, 3, 4)
 *                      /          \
 *                     /            \
 *             B(5, 6, 7, 8)     C(9, 10, 11, 12)
 *
 *   You can get the reset of context's succ_list generate by A:
 *                a_(1+1)'s succ_list(5+1, 9+1)
 *                a_(1+2)'s succ_list(5+2, 9+2)
 *                a_(1+3)'s succ_list(5+3, 9+3)
 */
Status DataTaskBuilder::GetSuccessorContextId(uint32_t out_anchor_index, const ge::NodePtr &node,
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
        vector<uint32_t> peer_in_context_id_list;
        (void)ge::AttrUtils::GetListInt(peer_op_of_pc, kAutoCtxIdList, peer_in_context_id_list);
        if (peer_in_context_id_list.empty()) {
          FFTS_LOGI("PhonyConcat %s peer op %s needs successor list but it do not have a context id.",
                    peer_op->GetName().c_str(), peer_op_of_pc->GetName().c_str());
          continue;
        }
        ctx_id_tmp = peer_in_context_id_list[0];
        FFTS_LOGD("Peer input op for PhonyConcat is %s, context id is %u.",
                  peer_op_of_pc->GetName().c_str(), ctx_id_tmp);
        peer_in_context_id.emplace_back(ctx_id_tmp);
      }
    } else {
      vector<uint32_t> peer_in_context_id_list;
      (void)ge::AttrUtils::GetListInt(peer_op, kAutoCtxIdList, peer_in_context_id_list);
      if (peer_in_context_id_list.empty()) {
        FFTS_LOGI("Node %s needs successor list but it has a successor %s which do not have a context id.",
                  node->GetName().c_str(), peer_op->GetName().c_str());
        continue;
      }
      ctx_id_tmp = peer_in_context_id_list[0];
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

void DataTaskBuilder::SetOperation(CACHE_OPERATION operation) {
  operation_ = operation;
}

void DataTaskBuilder::SetBurstLen(int64_t burst_len) {
  burst_len_ = burst_len;
}
}  // namespace ffts
