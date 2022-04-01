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

#include "runtime_ops_manual_task_builder.h"
#include "inc/ffts_utils.h"

namespace ffts {
RuntimeOpsTaskBuilder::RuntimeOpsTaskBuilder() {}

RuntimeOpsTaskBuilder::~RuntimeOpsTaskBuilder() {}


Status RuntimeOpsTaskBuilder::JudgeContextType(const ge::NodePtr &node, rtFftsPlusContextType_t &context_type) {
  auto op_type = node->GetType();
  for (auto &type_context : RTS_CONTEXT_TYPE_MAP) {
    if (type_context.second.count(op_type)) {
      context_type = type_context.first;
      return SUCCESS;
    }
  }
  FFTS_LOGE("[JudgeContextType]:the node[%s] type[%s] is not supported, Judge Context type failed.",
            node->GetName().c_str(), node->GetType().c_str());
  return FAILED;
}

Status RuntimeOpsTaskBuilder::GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("RuntimeOpsTaskBuilder::GenContextDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();
  Status status = FAILED;
  vector<FftsPlusComCtx_t> sub_ffts_plus_context;
  GenFftsPlusTaskCommonInfo(node, sub_ffts_plus_context);

  FftsPlusCtxDefPtr ctx_def_ptr =  nullptr;
  ctx_def_ptr = op_desc->TryGetExtAttr("FFTS_PLUS_TASK_DEF", ctx_def_ptr);
  FFTS_CHECK_NOTNULL(ctx_def_ptr);
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  FFTS_CHECK_NOTNULL(ffts_plus_ctx_def);
  ffts_plus_ctx_def->set_op_index(op_desc->GetId());

  uint32_t context_id = 0;
  if (ge::AttrUtils::GetInt(op_desc, kContextId, context_id)) {
    ffts_plus_ctx_def->set_context_id(context_id);
  }

  rtFftsPlusContextType_t context_type = RT_CTX_TYPE_AICORE;
  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  if (JudgeContextType(node, context_type) != SUCCESS) {
    return FAILED;
  }
  ffts_plus_ctx_def->set_context_type(context_type);

  switch (context_type) {
    case RT_CTX_TYPE_CASE_SWITCH:
      status = FillCaseSwitchContext(op_desc, ffts_plus_ctx_def, ctx_def_ptr, sub_ffts_plus_context);
      break;
    case RT_CTX_TYPE_LABEL:
      status = FillLabelContext(op_desc, ffts_plus_ctx_def, sub_ffts_plus_context);
      break;
    case RT_CTX_TYPE_SDMA:
      status = FillSdmaContext(op_desc, ffts_plus_ctx_def, ctx_def_ptr, sub_ffts_plus_context);
      break;
    default:
      status = FAILED;
      break;
  }
  if (status != SUCCESS) {
    FFTS_LOGE("GenContextTaskDef failed. Op[%s, optype[%s]]",
              op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }
  return SUCCESS;
}

Status RuntimeOpsTaskBuilder::FillLabelContext(const ge::OpDescPtr &op_desc, domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                                               vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  domi::FftsPlusLabelCtxDef *label_ctx_def = ffts_plus_ctx_def->mutable_label_ctx();
  FFTS_CHECK_NOTNULL(label_ctx_def);
  // label do not need to fill context data.
  label_ctx_def->set_pred_cnt(sub_ffts_plus_context[0].pred_cnt);
  label_ctx_def->set_pred_cnt_init(sub_ffts_plus_context[0].pred_cnt);
  label_ctx_def->set_successor_num(sub_ffts_plus_context[0].successorNum);
  (void)ge::AttrUtils::SetListInt(op_desc, kSuccList, sub_ffts_plus_context[0].succ_list);
  for (uint32_t i = 0; i < sub_ffts_plus_context[0].succ_list.size(); i++) {
    label_ctx_def->add_successor_list(sub_ffts_plus_context[0].succ_list[i]);
  }
  return SUCCESS;
}

Status RuntimeOpsTaskBuilder::FillSdmaContext(const ge::OpDescPtr &op_desc,
                                              domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                                              FftsPlusCtxDefPtr &ctx_def_ptr,
                                              vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  domi::FftsPlusSdmaCtxDef *sdma_ctx_def_ptr = ctx_def_ptr->mutable_sdma_ctx();
  FFTS_CHECK_NOTNULL(sdma_ctx_def_ptr);
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_SDMA);
  domi::FftsPlusSdmaCtxDef *sdma_ctx_def = ffts_plus_ctx_def->mutable_sdma_ctx();
  FFTS_CHECK_NOTNULL(sdma_ctx_def);
  Status status = FillSdmaContextData(sdma_ctx_def_ptr, sdma_ctx_def);
  if (status != SUCCESS) {
    FFTS_LOGE("FillSdmaContextData failed. Op[%s, optype[%s]]", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }
  sdma_ctx_def->set_pred_cnt(sub_ffts_plus_context[0].pred_cnt);
  sdma_ctx_def->set_pred_cnt_init(sub_ffts_plus_context[0].pred_cnt);
  sdma_ctx_def->set_successor_num(sub_ffts_plus_context[0].successorNum);
  sdma_ctx_def->set_aten(0);

  (void)ge::AttrUtils::SetListInt(op_desc, kSuccList, sub_ffts_plus_context[0].succ_list);
  return status;
}

Status RuntimeOpsTaskBuilder::FillSdmaContextData(const domi::FftsPlusSdmaCtxDef *sdma_ctx_def_ptr,
                                                  domi::FftsPlusSdmaCtxDef *sdma_ctx_def) const {
  FFTS_CHECK_NOTNULL(sdma_ctx_def_ptr);
  FFTS_CHECK_NOTNULL(sdma_ctx_def);
  sdma_ctx_def->set_aten(0);
  sdma_ctx_def->set_atm(0);
  sdma_ctx_def->set_pmg(sdma_ctx_def_ptr->pmg());
  sdma_ctx_def->set_ns(sdma_ctx_def_ptr->ns());
  sdma_ctx_def->set_part_id(sdma_ctx_def_ptr->part_id());
  sdma_ctx_def->set_qos(sdma_ctx_def_ptr->qos());
  sdma_ctx_def->set_thread_id(sdma_ctx_def_ptr->thread_id());
  sdma_ctx_def->set_thread_dim(sdma_ctx_def_ptr->thread_dim());
  sdma_ctx_def->set_sdma_sqe_header(sdma_ctx_def_ptr->sdma_sqe_header());
  sdma_ctx_def->set_src_stream_id(sdma_ctx_def_ptr->src_stream_id());
  sdma_ctx_def->set_src_sub_stream_id(sdma_ctx_def_ptr->src_sub_stream_id());
  sdma_ctx_def->set_dst_stream_id(sdma_ctx_def_ptr->dst_stream_id());
  sdma_ctx_def->set_dst_sub_stream_id(sdma_ctx_def_ptr->dst_sub_stream_id());
  sdma_ctx_def->set_src_addr_base(sdma_ctx_def_ptr->src_addr_base());
  sdma_ctx_def->set_src_addr_offset(sdma_ctx_def_ptr->src_addr_offset());
  sdma_ctx_def->set_dst_addr_base(sdma_ctx_def_ptr->dst_addr_base());
  sdma_ctx_def->set_dst_addr_offset(sdma_ctx_def_ptr->dst_addr_offset());
  sdma_ctx_def->set_non_tail_data_len(sdma_ctx_def_ptr->non_tail_data_len());
  sdma_ctx_def->set_tail_data_len(sdma_ctx_def_ptr->tail_data_len());
  return SUCCESS;
}


Status RuntimeOpsTaskBuilder::FillCaseSwitchContext(const ge::OpDescPtr &op_desc,
                                                    domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                                                    FftsPlusCtxDefPtr &ctx_def_ptr,
                                                    vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def_ptr = ctx_def_ptr->mutable_case_switch_ctx();
  FFTS_CHECK_NOTNULL(case_switch_ctx_def_ptr);
  domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def = ffts_plus_ctx_def->mutable_case_switch_ctx();
  FFTS_CHECK_NOTNULL(case_switch_ctx_def);
  Status status = FillCaseSwitchContextData(case_switch_ctx_def_ptr, case_switch_ctx_def);
  if (status != SUCCESS) {
    FFTS_LOGE("FillCaseSwitchContextData failed. Op[%s, optype[%s]]",
              op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }
  case_switch_ctx_def->set_pred_cnt(sub_ffts_plus_context[0].pred_cnt);
  case_switch_ctx_def->set_pred_cnt_init(sub_ffts_plus_context[0].pred_cnt);
  case_switch_ctx_def->set_successor_num(sub_ffts_plus_context[0].successorNum);
  case_switch_ctx_def->set_start_label_id(0);
  case_switch_ctx_def->set_label_list_len(sub_ffts_plus_context[0].successorNum);
  case_switch_ctx_def->set_aten(0);

  (void)ge::AttrUtils::SetListInt(op_desc, kSuccList, sub_ffts_plus_context[0].succ_list);
  for (uint32_t i = 0; i < sub_ffts_plus_context[0].succ_list.size(); i++) {
    case_switch_ctx_def->add_successor_list(sub_ffts_plus_context[0].succ_list[i]);
  }
  return status;
}

Status RuntimeOpsTaskBuilder::FillCaseSwitchContextData(const domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def_ptr,
                                                        domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def) const {
  FFTS_CHECK_NOTNULL(case_switch_ctx_def_ptr);
  FFTS_CHECK_NOTNULL(case_switch_ctx_def);
  case_switch_ctx_def->set_aten(0);
  case_switch_ctx_def->set_atm(0);
  case_switch_ctx_def->set_thread_id(case_switch_ctx_def_ptr->thread_id());
  case_switch_ctx_def->set_thread_dim(case_switch_ctx_def_ptr->thread_dim());
  case_switch_ctx_def->set_ar_size(case_switch_ctx_def_ptr->ar_size());
  case_switch_ctx_def->set_snoop(case_switch_ctx_def_ptr->snoop());
  case_switch_ctx_def->set_ar_cache(case_switch_ctx_def_ptr->ar_cache());
  case_switch_ctx_def->set_ar_prot(case_switch_ctx_def_ptr->ar_prot());
  case_switch_ctx_def->set_va(case_switch_ctx_def_ptr->va());
  case_switch_ctx_def->set_load_addr0_base(case_switch_ctx_def_ptr->load_addr0_base());
  case_switch_ctx_def->set_ld0_en(case_switch_ctx_def_ptr->ld0_en());
  case_switch_ctx_def->set_load_addr0_offset(case_switch_ctx_def_ptr->load_addr0_offset());
  case_switch_ctx_def->set_load_addr1_base(case_switch_ctx_def_ptr->load_addr1_base());
  case_switch_ctx_def->set_ld1_en(case_switch_ctx_def_ptr->ld1_en());
  case_switch_ctx_def->set_load_addr1_offset(case_switch_ctx_def_ptr->load_addr1_offset());
  return SUCCESS;
}
}  // namespace ffts
