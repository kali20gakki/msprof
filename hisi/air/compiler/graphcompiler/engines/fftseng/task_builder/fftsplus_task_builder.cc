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

#include "fftsplus_task_builder.h"
#include <securec.h>
#include <string>
#include "inc/ffts_log.h"
#include "inc/ffts_type.h"
#include "inc/graph/utils/node_utils.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "runtime/rt_error_codes.h"
#include "runtime/rt_model.h"
#include "runtime/mem.h"


namespace ffts {
FFTSPlusTaskBuilder::FFTSPlusTaskBuilder() {}

FFTSPlusTaskBuilder::~FFTSPlusTaskBuilder() {}

Status FFTSPlusTaskBuilder::GetFirstAvailableLabel(domi::FftsPlusTaskDef *ffts_plus_task_def,
                                                   domi::FftsPlusLabelCtxDef *pred_label_ctx,
                                                   domi::FftsPlusLabelCtxDef **avl_label_context,
                                                   uint32_t &recursion_count) {
  recursion_count++;
  if (recursion_count > kGetFirstAvailableLabel) {
    FFTS_LOGE("The count of GetFirstAvailableLabel recursion has reached %d, now stop recursion.", recursion_count);
    return FAILED;
  }
  FFTS_LOGD("pred_label_ctx successor_num is %u", pred_label_ctx->successor_num());
  if (pred_label_ctx->successor_num() == RT_CTX_SUCCESSOR_NUM) {
    uint32_t last_succ_id = pred_label_ctx->successor_list(RT_CTX_SUCCESSOR_NUM - 1);
    FFTS_LOGD("The pred label ctx is full, it's last successor id is %u", last_succ_id);
    uint32_t ctx_size = ffts_plus_task_def->ffts_plus_ctx_size();
    if (last_succ_id >= ctx_size) {
      REPORT_FFTS_ERROR("last_succ_id %u, ctx_size:%u", last_succ_id, ctx_size);
      return FAILED;
    }
    domi::FftsPlusCtxDef* last_succ_ctx = ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(last_succ_id));
    FFTS_CHECK_NOTNULL(last_succ_ctx);
    if (last_succ_ctx->context_type() == RT_CTX_TYPE_LABEL) {
      FFTS_LOGD("The last successor is a label context, keep searching.");
      GetFirstAvailableLabel(ffts_plus_task_def, last_succ_ctx->mutable_label_ctx(),
                             avl_label_context, recursion_count);
    } else {
      FFTS_LOGD("The last successor is a not label, stop searching and generate a new label.");
      Status ret = GenerateNewLabelCtx(ffts_plus_task_def, last_succ_id, pred_label_ctx, avl_label_context);
      if (ret != SUCCESS) {
        return ret;
      }
    }
  } else {
    *avl_label_context = pred_label_ctx;
    FFTS_LOGD("Stop searching for label context.");
    return SUCCESS;
  }

  return SUCCESS;
}

Status FFTSPlusTaskBuilder::UpdateSuccList(uint32_t succ_id, uint32_t curr_id,
                                           domi::FftsPlusTaskDef *ffts_plus_task_def) const {
  domi::FftsPlusCtxDef* ffts_plus_ctx = ffts_plus_task_def->mutable_ffts_plus_ctx(curr_id);
  FFTS_CHECK_NOTNULL(ffts_plus_ctx);
  uint32_t type = ffts_plus_ctx->context_type();
  FFTS_LOGI("current type:%u.", type);
  switch (type) {
    case RT_CTX_TYPE_AT_START:
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_at_start_ctx());
      break;
    case RT_CTX_TYPE_AICORE:
    case RT_CTX_TYPE_AIV:
      FFTS_LOGD("Update succlist for aic/aiv context %u, succ id %u", curr_id, succ_id);
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_aic_aiv_ctx());
      break;
    case RT_CTX_TYPE_MIX_AIC:
    case RT_CTX_TYPE_MIX_AIV:
      FFTS_LOGD("Update succlist for mix aic/aiv context %u, succ id %u", curr_id, succ_id);
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_mix_aic_aiv_ctx());
      break;
    case RT_CTX_TYPE_AICPU:
      FFTS_LOGD("Update succlist for aicpu context %u, succ id %u", curr_id, succ_id);
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_aicpu_ctx());
      break;
    case RT_CTX_TYPE_SDMA:
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_sdma_ctx());
      break;
    case RT_CTX_TYPE_NOTIFY_WAIT:
    case RT_CTX_TYPE_NOTIFY_RECORD:
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_notify_ctx());
      break;
    case RT_CTX_TYPE_WRITE_VALUE:
      AddOneId(ffts_plus_task_def, succ_id, ffts_plus_ctx->mutable_write_value_ctx());
      break;
    default:
      FFTS_LOGI("type %u does not need to update its successor list.", type);
      return FAILED;
  }
  return SUCCESS;
}

Status FFTSPlusTaskBuilder::GenerateTaskDef(const ge::NodePtr &node,
                                            domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("TaskBuilder::GenerateTask begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());

  ge::OpDescPtr op_desc = node->GetOpDesc();
  Status status = GenContextDef(node, ffts_plus_task_def);
  if (status != SUCCESS) {
    FFTS_LOGE("GenSubFftsTaskCommonInfo failed. Op[%s, optype[%s]]",
              op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }
  return SUCCESS;
}

Status FFTSPlusTaskBuilder::FillProducersInfo(const ge::NodePtr &node, FftsPlusComCtx_t &ffts_plus_context) const {
  uint8_t pred_cnt = 0;
  for (const auto &up_node : node->GetInAllNodes()) {
    ge::OpDescPtr up_op_desc = up_node->GetOpDesc();
    FFTS_LOGD("Up node name:%s, node type:%s.", up_op_desc->GetName().c_str(), up_op_desc->GetType().c_str());
    if (kHCCLOpType.count(up_op_desc->GetType()) > 0) {
      int32_t hccl_output_degree_0_num = 0;
      (void)ge::AttrUtils::GetInt(up_op_desc, kHcclOutDegree0Num, hccl_output_degree_0_num);
      pred_cnt += static_cast<uint8_t>(hccl_output_degree_0_num);
      FFTS_LOGD("FFTSPlusTaskBuilder node name:%s, node type:%s, hccl_output_degree_0_num:%d success",
                up_op_desc->GetName().c_str(), up_op_desc->GetType().c_str(), hccl_output_degree_0_num);
    } else if ((up_op_desc->HasAttr(kTypeFFTSPlus) && up_op_desc->GetType() != kTypePhonyConcat) ||
        up_op_desc->GetType() == kAtomicAddrClean) {
      if (!up_op_desc->HasAttr(kContextId) && !up_op_desc->HasAttr(kAutoCtxIdList)) {
        continue;
      }
      pred_cnt++;
    } else if (up_op_desc->GetType() == kTypePhonyConcat) {
      for (const auto &in_node : up_node->GetInDataNodes()) {
        auto in_node_desc = in_node->GetOpDesc();
        FFTS_CHECK_NOTNULL(in_node_desc);
        if (!in_node_desc->HasAttr(kContextId) && !up_op_desc->HasAttr(kAutoCtxIdList)) {
          continue;
        }
        if (in_node_desc->HasAttr(kTypeFFTSPlus)) {
          pred_cnt++;
        }
      }
    }
  }

  ffts_plus_context.pred_cnt = pred_cnt;
  vector<uint32_t> at_start_ctx_id_list;
  (void)ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoAtStartCtxIdList, at_start_ctx_id_list);
  if (!at_start_ctx_id_list.empty()) {
    ffts_plus_context.pred_cnt = 1;
  }

  FFTS_LOGD("FFTSPlusTaskBuilder node name:%s, node type:%s, predCnt:%d success",
            node->GetName().c_str(), node->GetType().c_str(), ffts_plus_context.pred_cnt);
  return SUCCESS;
}

static bool IsSubGraphBoundaryExt(const ge::NodePtr &phony_concat, uint32_t curr_id) {
  if (phony_concat->GetType() != kTypePhonyConcat) {
    return false;
  }

  FFTS_LOGD("Check whether phony concat %s is a boundary op.", phony_concat->GetName().c_str());
  for (const auto &peer_node : phony_concat->GetOutDataNodes()) {
    uint32_t peer_id = 0;
    if (ge::AttrUtils::GetInt(peer_node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, peer_id) &&
        peer_id == curr_id) {
      return false;
    }
  }
  FFTS_LOGD("Phony concat %s is a boundary op.", phony_concat->GetName().c_str());
  return true;
}

void FillComCtxForPhonyConcat(uint32_t &context_id, const ge::NodePtr &up_node,
                              FftsPlusComCtx_t &sub_ffts_plus_context_elem) {
  for (const auto &peer_node : up_node->GetOutDataNodes()) {
    ge::OpDescPtr peer_node_desc = peer_node->GetOpDesc();
    if (peer_node_desc == nullptr || !peer_node_desc->HasAttr(kContextId)) {
      continue;
    }
    if (kHCCLOpType.count(peer_node_desc->GetType()) > 0) {
      continue;
    }
    (void)ge::AttrUtils::GetInt(peer_node_desc, kContextId, context_id);
    FFTS_LOGD("Node successor add :%u, jump PhonyConcat.", context_id);
    sub_ffts_plus_context_elem.succ_list.emplace_back(context_id);
  }
}

void FFTSPlusTaskBuilder::FillManualCustomersInfo(const ge::NodePtr &node,
                                                  FftsPlusComCtx_t &sub_ffts_plus_context_elem) const {
  uint32_t context_id = 0;
  uint32_t curr_id = 0;
  (void)ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, curr_id);

  for (const auto &up_node : node->GetOutAllNodes()) {
    ge::OpDescPtr up_op_desc = up_node->GetOpDesc();
    if (IsSubGraphBoundaryExt(up_node, curr_id)) {
      FFTS_LOGD("Node name:%s is Boundary.", up_op_desc->GetName().c_str());
      continue;
    } else if (!up_node->GetOpDesc()->HasAttr(kTypeFFTSPlus)) {
      FFTS_LOGD("Node name:%s has not _ffts_plus attr.", up_op_desc->GetName().c_str());
      continue;
    }
    if (up_op_desc->GetType() == kTypePhonyConcat) {
      FillComCtxForPhonyConcat(context_id, up_node, sub_ffts_plus_context_elem);
    } else {
      if (!up_op_desc->HasAttr(kContextId)) {
        continue;
      }
      if (kHCCLOpType.count(up_op_desc->GetType()) > 0) {
        continue;
      }
      (void)ge::AttrUtils::GetInt(up_op_desc, kContextId, context_id);
      FFTS_LOGD("Out node name:%s, node type:%s, context_id:%u", up_op_desc->GetName().c_str(),
                up_op_desc->GetType().c_str(), context_id);
      sub_ffts_plus_context_elem.succ_list.push_back(context_id);
    }
  }
  return;
}

Status FFTSPlusTaskBuilder::FillCustomersInfo(const ge::NodePtr &node, FftsPlusComCtx_t &sub_ffts_plus_context_elem,
                                              vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  FFTS_CHECK_NOTNULL(node);

  // Confirm whether it is automatic or manual
  bool thread_manual_mode = true;
  ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  if (slice_info_ptr && slice_info_ptr->thread_mode == 1) {
    thread_manual_mode = false;
  }

  if (thread_manual_mode) {
    // manual
    FillManualCustomersInfo(node, sub_ffts_plus_context_elem);
    sub_ffts_plus_context_elem.successorNum = sub_ffts_plus_context_elem.succ_list.size();
    FFTS_LOGD("[TaskBuilder][Fftsplus][CommonTask] node name:%s, node type:%s, successorNum: %u.",
              node->GetName().c_str(), node->GetType().c_str(), sub_ffts_plus_context_elem.successorNum);
    sub_ffts_plus_context.push_back(sub_ffts_plus_context_elem);
  } else {
    // auto thread
    vector<uint32_t> context_id_list;
    (void)ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list);
    for (size_t i = 0; i < context_id_list.size(); i++) {
      sub_ffts_plus_context.emplace_back(sub_ffts_plus_context_elem);
    }
  }
  return SUCCESS;
}


Status FFTSPlusTaskBuilder::GenFftsPlusDependencyInfo(const ge::NodePtr &node,
                                                      vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  FftsPlusComCtx_t sub_ffts_plus_context_elem = {0};
  FillProducersInfo(node, sub_ffts_plus_context_elem);
  FillCustomersInfo(node, sub_ffts_plus_context_elem, sub_ffts_plus_context);
  return SUCCESS;
}

Status FFTSPlusTaskBuilder::GenFftsPlusTaskCommonInfo(const ge::NodePtr &node,
                                                      vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const {
  auto status = GenFftsPlusDependencyInfo(node, sub_ffts_plus_context);
  return status;
}

void FFTSPlusTaskBuilder::FillContextData(const domi::FftsPlusAicAivCtxDef *aicore_ctx_def,
                                          domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def) const {
  aic_aiv_ctx_def->set_aten(aicore_ctx_def->aten());

  aic_aiv_ctx_def->set_prefetch_once_bitmap(aicore_ctx_def->prefetch_once_bitmap());
  aic_aiv_ctx_def->set_prefetch_enable_bitmap(aicore_ctx_def->prefetch_enable_bitmap());
  aic_aiv_ctx_def->set_atm(aicore_ctx_def->atm());
  aic_aiv_ctx_def->set_thread_dim(aicore_ctx_def->thread_dim());
  aic_aiv_ctx_def->set_non_tail_block_dim(aicore_ctx_def->non_tail_block_dim());
  aic_aiv_ctx_def->set_tail_block_dim(aicore_ctx_def->tail_block_dim());
  aic_aiv_ctx_def->set_input_output_count(aicore_ctx_def->input_output_count());
  for (auto task_addr : aicore_ctx_def->task_addr()) {
    aic_aiv_ctx_def->add_task_addr(task_addr);
  }
  for (auto task_addr_offset : aicore_ctx_def->task_addr_offset()) {
    aic_aiv_ctx_def->add_task_addr_offset(task_addr_offset);
  }
  for (auto kernel_name : aicore_ctx_def->kernel_name()) {
    aic_aiv_ctx_def->add_kernel_name(kernel_name);
  }
  aic_aiv_ctx_def->set_task_param_ptr_offset(aicore_ctx_def->task_param_ptr_offset());
}

void FFTSPlusTaskBuilder::FillContextData(const domi::FftsPlusMixAicAivCtxDef *aicore_ctx_def,
                                          domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def) const {
  mix_aic_aiv_ctx_def->set_aten(aicore_ctx_def->aten());

  mix_aic_aiv_ctx_def->set_prefetch_once_bitmap(aicore_ctx_def->prefetch_once_bitmap());
  mix_aic_aiv_ctx_def->set_prefetch_enable_bitmap(aicore_ctx_def->prefetch_enable_bitmap());
  mix_aic_aiv_ctx_def->set_atm(aicore_ctx_def->atm());
  mix_aic_aiv_ctx_def->set_thread_dim(aicore_ctx_def->thread_dim());
  mix_aic_aiv_ctx_def->set_non_tail_block_dim(aicore_ctx_def->non_tail_block_dim());
  mix_aic_aiv_ctx_def->set_tail_block_dim(aicore_ctx_def->tail_block_dim());
  mix_aic_aiv_ctx_def->set_input_output_count(aicore_ctx_def->input_output_count());
  for (auto task_addr : aicore_ctx_def->task_addr()) {
    mix_aic_aiv_ctx_def->add_task_addr(task_addr);
  }
  for (auto kernel_name : aicore_ctx_def->kernel_name()) {
    mix_aic_aiv_ctx_def->add_kernel_name(kernel_name);
  }

  mix_aic_aiv_ctx_def->set_non_tail_block_ratio_n(aicore_ctx_def->non_tail_block_ratio_n());
  mix_aic_aiv_ctx_def->set_tail_block_ratio_n(aicore_ctx_def->tail_block_ratio_n());
}

Status FFTSPlusTaskBuilder::FillContextData(const domi::FftsPlusAicpuCtxDef *aicpu_ctx_def_ptr,
                                            domi::FftsPlusAicpuCtxDef *aicpu_ctx_def) const {
  aicpu_ctx_def->set_atm(aicpu_ctx_def_ptr->atm());
  aicpu_ctx_def->set_kernel_type(aicpu_ctx_def_ptr->kernel_type());
  aicpu_ctx_def->set_bm(aicpu_ctx_def_ptr->bm());
  aicpu_ctx_def->set_topic_type(aicpu_ctx_def_ptr->topic_type());
  aicpu_ctx_def->set_thread_id(aicpu_ctx_def_ptr->thread_id());
  aicpu_ctx_def->set_thread_dim(aicpu_ctx_def_ptr->thread_dim());
  aicpu_ctx_def->set_non_tail_block_dim(aicpu_ctx_def_ptr->non_tail_block_dim());
  aicpu_ctx_def->set_tail_block_dim(aicpu_ctx_def_ptr->tail_block_dim());
  aicpu_ctx_def->set_sub_topic_id(aicpu_ctx_def_ptr->sub_topic_id());
  aicpu_ctx_def->set_topic_id(aicpu_ctx_def_ptr->topic_id());
  aicpu_ctx_def->set_group_id(aicpu_ctx_def_ptr->group_id());
  aicpu_ctx_def->set_task_param_offset(aicpu_ctx_def_ptr->task_param_offset());
  domi::aicpuKernelDef *kernel_def = aicpu_ctx_def->mutable_kernel();
  FFTS_CHECK_NOTNULL(kernel_def);
  kernel_def->set_args_size(aicpu_ctx_def_ptr->kernel().args_size());
  kernel_def->set_args(aicpu_ctx_def_ptr->kernel().args());
  kernel_def->set_so_name(aicpu_ctx_def_ptr->kernel().so_name());
  kernel_def->set_kernel_name(aicpu_ctx_def_ptr->kernel().kernel_name());
  kernel_def->set_kernel_ext_info(aicpu_ctx_def_ptr->kernel().kernel_ext_info());
  kernel_def->set_kernel_ext_info_size(aicpu_ctx_def_ptr->kernel().kernel_ext_info_size());
  return SUCCESS;
}
}  // namespace ffts
