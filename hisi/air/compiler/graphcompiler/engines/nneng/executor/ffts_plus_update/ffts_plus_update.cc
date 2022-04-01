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
#include "ffts_plus_update.h"
#include "common/aicore_util_attr_define.h"
#include "common/aicore_util_constants.h"
#include "graph/utils/node_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/op_desc_utils.h"
namespace fe {
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIC, FFTSPlusFeUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIV, FFTSPlusFeUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeMixAIC, FFTSPlusFeUpdate);
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeMixAIV, FFTSPlusFeUpdate);
FFTSPlusFeUpdate::FFTSPlusFeUpdate() {}
FFTSPlusFeUpdate::~FFTSPlusFeUpdate() {}

Status FFTSPlusFeUpdate::GetAutoThreadParam(const ge::NodePtr &node,
                                            const vector<optiling::utils::OpRunInfo> &op_run_info,
                                            AutoThreadParam &auto_thread_param)
{
  FE_CHECK_NOTNULL(node);
  (void)op_run_info;
  (void)auto_thread_param;
  return SUCCESS;
}

inline void SetLow32FromSrc(uint32_t &dst, const uint64_t &src)
{
  dst = static_cast<uint32_t>(src & 0xFFFFFFFF);
  return;
}
inline void SetHigh32FromSrc(uint32_t &dst, const uint64_t &src)
{
  dst = static_cast<uint32_t>((src >> 32) & 0xFFFFFFFF);
  return;
}
inline void SetHigh16FromSrc(uint16_t &dst, const uint64_t &src)
{
  dst = static_cast<uint16_t>((src >> 32) & 0xFFFF);
  return;
}

Status FFTSPlusFeUpdate::UpdateMixL2AicAivCtx(const rtFftsPlusTaskInfo_t &task_info,
                                              const AutoThreadSubTaskFlush &flush_data, const ge::NodePtr node) const
{
  void *ctx_buf = const_cast<void*>(task_info.descBuf);
  rtFftsPlusComCtx_t *context_head = reinterpret_cast<rtFftsPlusComCtx_t*>(ctx_buf);
  rtFftsPlusMixAicAivCtx_t *ctx = nullptr;
  uint32_t contextId;
  (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kContextId, contextId);
  if (contextId > task_info.fftsPlusSqe->totalContextNum) {
    REPORT_FE_ERROR("[Executor][FFTS_plus_update][UpdateMixL2AicAivCtx]Context Id(%d) over err.", contextId);
    return FAILED;
  }
  ctx = reinterpret_cast<rtFftsPlusMixAicAivCtx_t*>(context_head + contextId);
  FE_CHECK_NOTNULL(ctx);
  uint64_t paraBase = reinterpret_cast<uintptr_t>(flush_data.args_base);
  SetLow32FromSrc(ctx->aicTaskParamPtrL, paraBase);
  SetHigh16FromSrc(ctx->aicTaskParamPtrH, paraBase);
  ctx->aicTaskParamPtrOffset = 0;
  ctx->aivTaskParamPtrOffset = 0;
  ctx->aivTaskParamPtrH = ctx->aicTaskParamPtrH;
  ctx->aivTaskParamPtrL = ctx->aicTaskParamPtrL;
  if (node->GetOpDesc()->HasAttr("_mix_aic" + ATTR_NAME_KERNEL_LIST_FIRST_NAME) ||
      node->GetOpDesc()->HasAttr("_mix_aiv" + ATTR_NAME_KERNEL_LIST_FIRST_NAME)) {
    if (flush_data.op_run_info.empty()) {
      REPORT_FE_ERROR("[Executor][FFTS_plus_update][UpdateMixL2AicAivCtx]MixAicAivCtx runinfo size(%zu) err.",
                      flush_data.op_run_info.size());
      return FAILED;
    }
    ctx->nonTailBlockdim = flush_data.op_run_info[0].GetBlockDim();
    ctx->tailBlockdim = flush_data.op_run_info[0].GetBlockDim();
    FE_LOGD("MixL2 nonTail_Block_dim:%d, tail_Block_dim:%d", ctx->nonTailBlockdim, ctx->tailBlockdim);
    ctx->aicIcachePrefetchCnt = flush_data.aic_icache_prefetch_cnt;
    SetLow32FromSrc(ctx->nonTailAicTaskStartPcL, flush_data.aic_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->nonTailAicTaskStartPcH, flush_data.aic_non_tail_task_start_pc);
    SetLow32FromSrc(ctx->tailAicTaskStartPcL, flush_data.aic_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->tailAicTaskStartPcH, flush_data.aic_non_tail_task_start_pc);
    ctx->aivIcachePrefetchCnt = flush_data.aiv_icache_prefetch_cnt;
    SetLow32FromSrc(ctx->nonTailAivTaskStartPcL, flush_data.aiv_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->nonTailAivTaskStartPcH, flush_data.aiv_non_tail_task_start_pc);
    SetLow32FromSrc(ctx->tailAivTaskStartPcL, flush_data.aiv_non_tail_task_start_pc);
    SetHigh16FromSrc(ctx->tailAivTaskStartPcH, flush_data.aiv_non_tail_task_start_pc);
  }
  FE_LOGD("MixL2 context update success");
  return SUCCESS;
}

Status FFTSPlusFeUpdate::UpdateSubTaskAndCache(const ge::NodePtr &node, const AutoThreadSubTaskFlush &flush_data,
                                               rtFftsPlusTaskInfo_t &task_info)
{
  FE_CHECK_NOTNULL(node);
  ge::OpDescPtr op_desc = node->GetOpDesc();
  if (op_desc->HasAttr(ATTR_NAME_ALIAS_ENGINE_NAME)) {
    return UpdateMixL2AicAivCtx(task_info, flush_data, node);
  }
  FE_LOGD("FFTS+ node context update success.");
  return SUCCESS;
}
}