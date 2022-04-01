/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/load/model_manager/task_info/ffts_plus_proto_transfer.h"

#include "securec.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/model_utils.h"
#include "aicpu/common/aicpu_task_struct.h"

namespace {
constexpr int32_t kSrcSlotNum = 4;
constexpr int32_t kNotifyIdNum = 16;
constexpr int32_t kWriteValueNum = 4;

constexpr int32_t kRequiredUserDataNum = 6;
constexpr int32_t kArgsAddrLIndex = 2;
constexpr int32_t kArgsAddrHIndex = 3;
constexpr int32_t kSoNameAddrLIndex = 0;
constexpr int32_t kSoNameAddrHIndex = 1;
constexpr int32_t kKernelNameAddrLIndex = 4;
constexpr int32_t kKernelNameAddrHIndex = 5;

constexpr int32_t kManualIndex = 0;
constexpr int32_t kManualAicAivCtxPcNum = 1;
constexpr int32_t kAutoNonTailIndex = 0;
constexpr int32_t kAutoTailIndex = 1;
constexpr int32_t kAutoAicAivCtxPcNum = 2;
constexpr int32_t kManualAicCtxIndex = 0;
constexpr int32_t kManualAivCtxIndex = 1;
constexpr int32_t kManualMixAicAivCtxPcNum = 2;
constexpr int32_t kAutoNonTailAicCtxIndexVal0 = 0;
constexpr int32_t kAutoTailAicCtxIndex = 1;
constexpr int32_t kAutoNonTailAivCtxIndexVal2 = 2;
constexpr int32_t kAutoTailAivCtxIndex = 3;
constexpr int32_t kAutoMixAicAivCtxPcNum = 4;

constexpr uint32_t k1BitMask = 0x00000001U;    // 1  bit , 0000,0001
constexpr uint32_t k2BitsMask = 0x00000003U;   // 2  bits, 0000,0011
constexpr uint32_t k3BitsMask = 0x00000007U;   // 3  bits, 0000,0111
constexpr uint32_t k4BitsMask = 0x0000000FU;   // 4  bits, 0000,1111
constexpr uint32_t k5BitsMask = 0x0000001FU;   // 5  bits, 0001,1111
constexpr uint32_t k6BitsMask = 0x0000003FU;   // 6  bits, 0011,1111
constexpr uint32_t k7BitsMask = 0x0000007FU;   // 7  bits, 0111,1111
constexpr uint32_t k8BitsMask = 0x000000FFU;   // 8  bits, 1111,1111

constexpr uint32_t k12BitsMask = 0x00000FFFU;  // 12 bits, 0000,1111,1111,1111
constexpr uint32_t k16BitsMask = 0x0000FFFFU;  // 16 bits, 1111,1111,1111,1111

constexpr uint32_t k17BitsMask = 0x0001FFFFU;  // 17 bits, 0000,0000,0000,0001,1111,1111,1111,1111
constexpr uint32_t k32BitsMask = 0xFFFFFFFFU;  // 32 bits, 1111,1111,1111,1111,1111,1111,1111,1111

const std::string kAicpuAllshape = "_AllShape";
constexpr uint32_t kFwkAicpuKernelType = 1U;
constexpr uint32_t kCustomAicpuKernelType = 4U;

const std::set<rtFftsPlusContextType_t> kSaveArgsCtxType = {
    RT_CTX_TYPE_AICORE,
    RT_CTX_TYPE_AIV,
    RT_CTX_TYPE_MIX_AIC,
    RT_CTX_TYPE_MIX_AIV,
    RT_CTX_TYPE_AICPU
};
constexpr uint32_t kModeInArgsFirstFieldVal0 = 0U; // mode addr at args field
constexpr uint32_t kArgsSkipFirstField = 1U; // mix ctx args first addr is not input/output addr
constexpr uint32_t kSaveTaskAddr = 1U;
}

namespace ge {
void CleanRtFftsPlusTask(rtFftsPlusTaskInfo_t &ffts_plus_task_info) {
  if (ffts_plus_task_info.fftsPlusSqe != nullptr) {
    delete [] PtrToPtr<const rtFftsPlusSqe_t, const uint8_t>(ffts_plus_task_info.fftsPlusSqe);
  }
  ffts_plus_task_info.fftsPlusSqe = nullptr;
  ffts_plus_task_info.descBuf = nullptr;
}

std::map<rtFftsPlusContextType_t, FftsPlusProtoTransfer::CtxHandle> FftsPlusProtoTransfer::init_ctx_fun_ {
  { RT_CTX_TYPE_AICORE, &FftsPlusProtoTransfer::InitAicAivCtx },
  { RT_CTX_TYPE_AIV, &FftsPlusProtoTransfer::InitAicAivCtx },
  { RT_CTX_TYPE_PERSISTENT_CACHE, &FftsPlusProtoTransfer::InitPersistentCacheCtx },
  { RT_CTX_TYPE_NOTIFY_WAIT, &FftsPlusProtoTransfer::InitNotifyCtx },
  { RT_CTX_TYPE_NOTIFY_RECORD, &FftsPlusProtoTransfer::InitNotifyCtx },
  { RT_CTX_TYPE_WRITE_VALUE, &FftsPlusProtoTransfer::InitWriteValueCtx },
  { RT_CTX_TYPE_MIX_AIC, &FftsPlusProtoTransfer::InitMixAicAivCtx },
  { RT_CTX_TYPE_MIX_AIV, &FftsPlusProtoTransfer::InitMixAicAivCtx },
  { RT_CTX_TYPE_SDMA, &FftsPlusProtoTransfer::InitSdmaCtx },
  { RT_CTX_TYPE_FLUSH_DATA, &FftsPlusProtoTransfer::InitDataCtx },
  { RT_CTX_TYPE_INVALIDATE_DATA, &FftsPlusProtoTransfer::InitDataCtx },
  { RT_CTX_TYPE_WRITEBACK_DATA, &FftsPlusProtoTransfer::InitDataCtx },
  { RT_CTX_TYPE_AICPU, &FftsPlusProtoTransfer::InitAicpuCtx },
  { RT_CTX_TYPE_COND_SWITCH, &FftsPlusProtoTransfer::InitCondSwitchCtx },
  { RT_CTX_TYPE_CASE_SWITCH, &FftsPlusProtoTransfer::InitCaseCtx },
  { RT_CTX_TYPE_AT_START, &FftsPlusProtoTransfer::InitAtStartCtx },
  { RT_CTX_TYPE_AT_END, &FftsPlusProtoTransfer::InitAtEndCtx },
  { RT_CTX_TYPE_LABEL, &FftsPlusProtoTransfer::InitLabelCtx }
};

Status FftsPlusProtoTransfer::Transfer(const OpDescPtr &op_desc, const domi::FftsPlusTaskDef &ffts_plus_task_def,
                                       rtFftsPlusTaskInfo_t &ffts_plus_task_info) {
  GE_CHECK_NOTNULL(find_node_handle_);
  GE_CHECK_NOTNULL(op_desc);
  logic_stream_id_ = static_cast<uint32_t>(op_desc->GetStreamId());

  const int32_t ctx_num = ffts_plus_task_def.ffts_plus_ctx_size();
  ffts_plus_task_info.descBufLen = sizeof(rtFftsPlusComCtx_t) * static_cast<size_t>(ctx_num);
  auto *const sqe_ctx = new (std::nothrow) uint8_t[sizeof(rtFftsPlusSqe_t) + ffts_plus_task_info.descBufLen]{};
  GE_CHECK_NOTNULL(sqe_ctx);

  auto *const ffts_plus_sqe = PtrToPtr<uint8_t, rtFftsPlusSqe_t>(sqe_ctx);
  ffts_plus_task_info.fftsPlusSqe = ffts_plus_sqe;
  InitFftsPlusSqe(ffts_plus_task_def.ffts_plus_sqe(), ffts_plus_sqe);

  ffts_plus_task_info.descBuf = &sqe_ctx[sizeof(rtFftsPlusSqe_t)];
  GELOGI("Init ctx begin, node %s, args_base=0x%lx, ctx_num=%d", op_desc->GetName().c_str(), args_base_, ctx_num);
  GE_CHK_STATUS_RET_NOLOG(InitFftsPlusCtx(ffts_plus_task_def, &sqe_ctx[sizeof(rtFftsPlusSqe_t)], ctx_num));

  return SUCCESS;
}

void FftsPlusProtoTransfer::InitFftsPlusSqe(const domi::FftsPlusSqeDef &sqe_def, rtFftsPlusSqe_t * const sqe) const {
  InitFftsPlusSqeHeader(sqe_def.sqe_header(), sqe->sqeHeader);

  sqe->wrrRatio = static_cast<uint16_t>(sqe_def.wrr_ratio() & k4BitsMask);
  sqe->sqeIndex = static_cast<uint16_t>(sqe_def.sqe_index());

  sqe->totalContextNum = static_cast<uint16_t>(sqe_def.total_context_num());
  sqe->readyContextNum = static_cast<uint16_t>(sqe_def.ready_context_num());
  sqe->preloadContextNum = static_cast<uint16_t>(sqe_def.preload_context_num());

  sqe->prefetchOstNum = static_cast<uint16_t>(sqe_def.prefetch_ost_num() & k5BitsMask);
  sqe->cmaintOstNum = static_cast<uint16_t>(sqe_def.cmaint_ost_num() & k5BitsMask);

  sqe->aicPrefetchLower = static_cast<uint16_t>(sqe_def.aic_prefetch_lower() & k5BitsMask);
  sqe->aicPrefetchUpper = static_cast<uint16_t>(sqe_def.aic_prefetch_upper() & k5BitsMask);
  sqe->aivPrefetchLower = static_cast<uint16_t>(sqe_def.aiv_prefetch_lower() & k5BitsMask);
  sqe->aivPrefetchUpper = static_cast<uint16_t>(sqe_def.aiv_prefetch_upper() & k5BitsMask);
}

void FftsPlusProtoTransfer::InitFftsPlusSqeHeader(const domi::StarsSqeHeaderDef &sqe_header_def,
                                                  rtStarsSqeHeader_t &sqe_header) const {
  sqe_header.l1Lock = static_cast<uint8_t>(sqe_header_def.l1_lock());
  sqe_header.l1Unlock = static_cast<uint8_t>(sqe_header_def.l1_unlock());
  sqe_header.blockDim = static_cast<uint16_t>(sqe_header_def.block_dim());
}

Status FftsPlusProtoTransfer::InitFftsPlusCtx(const domi::FftsPlusTaskDef &task_def,
                                              uint8_t *const ctx, const int32_t num) {
  InitAdditionalData(task_def);
  rtFftsPlusComCtx_t *const com_ctx = PtrToPtr<uint8_t, rtFftsPlusComCtx_t>(ctx);
  GE_CHECK_NOTNULL(com_ctx);
  for (auto i = 0; i < num; ++i) {
    const domi::FftsPlusCtxDef &ctx_def = task_def.ffts_plus_ctx(i);
    const auto ctx_type = static_cast<rtFftsPlusContextType_t>(ctx_def.context_type());
    const auto op_desc = find_node_handle_(ctx_def.op_index());
    if (op_desc != nullptr) {
      std::vector<std::string> orig_names;
      if (AttrUtils::GetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, orig_names) && (!orig_names.empty())) {
        FusionOpInfo fusion_op_info;
        fusion_op_info.stream_id = logic_stream_id_;
        fusion_op_info.op_index = ctx_def.op_index();
        fusion_op_info.original_op_names = orig_names;
        fusion_op_info.op_name = op_desc->GetName();
        fusion_op_info_.emplace_back(fusion_op_info);
      }

      if ((save_ctx_args_handle_ != nullptr) && (kSaveArgsCtxType.count(ctx_type) > 0U)) {
        size_t dump_args_offset = io_addrs_.size();
        const auto it = ctx_additional_data_.find(kModeInArgsFirstFieldVal0);
        if ((it != ctx_additional_data_.cend()) && (it->second.count(ctx_def.context_id()) > 0U)) {
          dump_args_offset += kArgsSkipFirstField;
        }
        GELOGD("save ctx args, op idx:%u, ctx type:%u, ctx id:%u", ctx_def.op_index(),
               ctx_def.context_type(), ctx_def.context_id());
        save_ctx_args_handle_(op_desc, dump_args_offset);
      }
    }

    GELOGI("Init ctx %d in FftsPlusTask, context_type=%u, op_index=%u", i, ctx_type, ctx_def.op_index());
    const std::map<rtFftsPlusContextType_t, CtxHandle>::const_iterator it = init_ctx_fun_.find(ctx_type);
    if (it != init_ctx_fun_.cend()) {
      auto *const temp_ctx = PtrAdd<rtFftsPlusComCtx_t>(com_ctx, static_cast<size_t>(num), static_cast<size_t>(i));
      temp_ctx->contextType = ctx_type;
      GE_CHK_STATUS_RET(it->second(this, ctx_def, temp_ctx), "Init fftsplusCtx failed, ctx_index:%d", i);
    } else {
      REPORT_INNER_ERROR("E19999", "Unsupported ctx type %u", ctx_type);
      GELOGE(FAILED, "[Check][CtxType] Unsupported ctx type %u", ctx_type);
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitPersistentCacheCtx(const domi::FftsPlusCtxDef &task_def,
                                                     rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusPersistentCacheCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusPersistentCacheCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusCachePersistCtxDef &ctx_def = task_def.cache_persist_ctx();
  ctx->contextType = static_cast<rtFftsPlusContextType_t>(task_def.context_type());
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999",
                       "Size of successor_list in FftsPlusPersistentCacheCtxDef should not > %d, it is %d actually",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED,
           "[Check][Param] Size of successor_list in FftsPlusPersistentCacheCtxDef should not > %d, it is %d actually",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->persistentEnable = static_cast<uint8_t>(ctx_def.persistent_en() & k1BitMask);
  ctx->persistentSize = static_cast<uint16_t>(ctx_def.persistent_size());
  ctx->persistentId = static_cast<uint32_t>(ctx_def.persistent_id());
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicAivCtx(const domi::FftsPlusCtxDef &task_def,
                                            rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusAicAivCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusAicAivCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusAicAivCtxDef &ctx_def = task_def.aic_aiv_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->prefetchConfig = static_cast<uint8_t>(ctx_def.prefetch_config());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusAicAivCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusAicAivCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->schem = static_cast<uint16_t>(ctx_def.schem() & k2BitsMask);
  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->prefetchEnableBitmap = static_cast<uint16_t>(ctx_def.prefetch_enable_bitmap() & k4BitsMask);
  ctx->prefetchOnceBitmap = static_cast<uint16_t>(ctx_def.prefetch_once_bitmap() & k4BitsMask);

  ctx->pmg = static_cast<uint16_t>(ctx_def.pmg() & k2BitsMask);
  ctx->ns = static_cast<uint16_t>(ctx_def.ns() & k1BitMask);
  ctx->partId = static_cast<uint16_t>(ctx_def.part_id() & k8BitsMask);
  ctx->qos = static_cast<uint16_t>(ctx_def.qos() & k4BitsMask);

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());
  ctx->nonTailBlockdim = static_cast<uint16_t>(ctx_def.non_tail_block_dim());
  ctx->tailBlockdim = static_cast<uint16_t>(ctx_def.tail_block_dim());

  if (ctx_def.src_slot_size() > kSrcSlotNum) {
    REPORT_INNER_ERROR("E19999", "Size of src_slot in FftsPlusAicAivCtxDef should not > %d, but %d exactly",
                       kSrcSlotNum, ctx_def.src_slot_size());
    GELOGE(FAILED, "[Check][Param] Size of src_slot in FftsPlusAicAivCtxDef should not > %d, but %d exactly",
           kSrcSlotNum, ctx_def.src_slot_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.src_slot_size(); ++i) {
    ctx->srcSlot[i] = static_cast<uint16_t>(ctx_def.src_slot(i));
  }

  const uint64_t task_param_ptr_base = args_base_ + (sizeof(void *) * io_addrs_.size());
  GELOGD("FftsPlusAicAivCtxDef: task param addr is %lu.", task_param_ptr_base);
  ctx->taskParamPtrBaseL = static_cast<uint32_t>(task_param_ptr_base & k32BitsMask);
  ctx->taskParamPtrBaseH = static_cast<uint16_t>((task_param_ptr_base >> 32U) & k16BitsMask);
  ctx->taskParamPtrOffset = static_cast<uint16_t>(ctx_def.task_param_ptr_offset());

  if (ctx->threadDim == 0U) {
    GELOGD("Context thread dim is zero, Dynamic shape mode.");
    return SUCCESS;
  }

  return (ctx->atm == 0U) ? InitManualAicAivCtx(ctx_def, *ctx) : InitAutoAicAivCtx(ctx_def, *ctx);
}

Status FftsPlusProtoTransfer::InitManualAicAivCtx(const domi::FftsPlusAicAivCtxDef &ctx_def,
                                                  rtFftsPlusAicAivCtx_t &ctx) const {
  for (auto i = 0; i < ctx_def.task_addr_size(); ++i) {
    GELOGD("index %d, task addr is 0x%lx", i, ctx_def.task_addr(i));
    io_addrs_.emplace_back(ctx_def.task_addr(i));
  }

  // PcL for low 32 bits of pc, PcH for high 16 bits of pc
  if (ctx_def.kernel_name_size() != kManualAicAivCtxPcNum) {
    REPORT_INNER_ERROR("E19999", "Size of kernel_name in FftsPlusAicAivCtxDef should be %d, but %d exactly",
                       kManualAicAivCtxPcNum, ctx_def.kernel_name_size());
    GELOGE(FAILED, "[Check][Param] Size of kernel_name in FftsPlusAicAivCtxDef should be %d, but %d exactly",
           kManualAicAivCtxPcNum, ctx_def.kernel_name_size());
    return FAILED;
  }
  uint32_t i_cache_prefetch_cnt = 0U;
  void *task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kManualIndex), task_start_pc, i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s", ctx_def.kernel_name(kManualIndex).c_str());
  }
  ctx.nonTailTaskStartPcL = static_cast<uint32_t>(PtrToValue(task_start_pc) & k32BitsMask);
  ctx.nonTailTaskStartPcH = static_cast<uint16_t>((PtrToValue(task_start_pc) >> 32U) & k16BitsMask);
  ctx.tailTaskStartPcL = static_cast<uint32_t>(PtrToValue(task_start_pc) & k32BitsMask);
  ctx.tailTaskStartPcH = static_cast<uint16_t>((PtrToValue(task_start_pc) >> 32U) & k16BitsMask);
  ctx.icachePrefetchCnt = static_cast<uint16_t>(i_cache_prefetch_cnt & k5BitsMask);

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAutoAicAivCtx(const domi::FftsPlusAicAivCtxDef &ctx_def,
                                                rtFftsPlusAicAivCtx_t &ctx) const {
  if (ctx_def.save_task_addr() == kSaveTaskAddr) {
    for (uint16_t i = 0U; i < (ctx.threadDim - 1U); ++i) {
      InitThrdIoAddrs(ctx_def, i, ctx_def.task_addr_offset_size());
    }
    InitThrdIoAddrs(ctx_def, ctx.threadDim - 1U, static_cast<int32_t>(ctx_def.input_output_count()));
    for (auto k = 0; k < (ctx_def.task_addr_size() - ctx_def.task_addr_offset_size()); ++k) {
      auto logic_addr = static_cast<uintptr_t>(ctx_def.task_addr(ctx_def.task_addr_offset_size() + k));
      io_addrs_.emplace_back(logic_addr);
    }
  }

  // PcL for low 32 bits of pc, PcH for high 16 bits of pc
  if (ctx_def.kernel_name_size() != kAutoAicAivCtxPcNum) {
    REPORT_INNER_ERROR("E19999", "Size of kernel_name in FftsPlusAicAivCtxDef should be %d, but %d exactly",
                       kAutoAicAivCtxPcNum, ctx_def.kernel_name_size());
    GELOGE(FAILED, "[Check][Param] Size of kernel_name in FftsPlusAicAivCtxDef should be %d, but %d exactly",
           kAutoAicAivCtxPcNum, ctx_def.kernel_name_size());
    return FAILED;
  }
  uint32_t non_tail_i_cache_prefetch_cnt = 0U;
  void *non_tail_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kAutoNonTailIndex), non_tail_task_start_pc,
                                        non_tail_i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s", ctx_def.kernel_name(kAutoNonTailIndex).c_str());
  }
  ctx.nonTailTaskStartPcL = static_cast<uint32_t>(PtrToValue(non_tail_task_start_pc) & k32BitsMask);
  ctx.nonTailTaskStartPcH = static_cast<uint16_t>((PtrToValue(non_tail_task_start_pc) >> 32U) & k16BitsMask);
  uint32_t tail_i_cache_prefetch_cnt = 0U;
  void *tail_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(
        addr_pref_handle_(ctx_def.kernel_name(kAutoTailIndex), tail_task_start_pc, tail_i_cache_prefetch_cnt),
        "Get addr and pref cnt failed, kernel_name=%s", ctx_def.kernel_name(kAutoTailIndex).c_str());
  }

  ctx.tailTaskStartPcL = static_cast<uint32_t>(PtrToValue(tail_task_start_pc) & k32BitsMask);
  ctx.tailTaskStartPcH = static_cast<uint16_t>((PtrToValue(tail_task_start_pc) >> 32U) & k16BitsMask);
  const uint32_t i_cache_prefetch_cnt = std::min(non_tail_i_cache_prefetch_cnt, tail_i_cache_prefetch_cnt);
  ctx.icachePrefetchCnt = static_cast<uint16_t>(i_cache_prefetch_cnt & k5BitsMask);

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitNotifyCtx(const domi::FftsPlusCtxDef &task_def,
                                            rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusNotifyCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusNotifyCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusNotifyCtxDef &ctx_def = task_def.notify_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusNotifyCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusNotifyCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->satm = static_cast<uint16_t>(ctx_def.satm() & k1BitMask);
  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());
  ctx->notifyIdBase = static_cast<uint16_t>(ctx_def.notify_id_base());
  ctx->autoWindow = static_cast<uint8_t>(ctx_def.auto_window());

  if (ctx_def.notify_id_size() > kNotifyIdNum) {
    REPORT_INNER_ERROR("E19999", "Size of notify_id in FftsPlusNotifyCtxDef should not > %d, but %d exactly",
                       kNotifyIdNum, ctx_def.notify_id_size());
    GELOGE(FAILED, "[Check][Param] Size of notify_id in FftsPlusNotifyCtxDef should not > %d, but %d exactly",
           kNotifyIdNum, ctx_def.notify_id_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.notify_id_size(); i++) {
    ctx->notifyId[i] = static_cast<uint16_t>(ctx_def.notify_id(i));
  }

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitWriteValueCtx(const domi::FftsPlusCtxDef &task_def,
                                                rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusWriteValueCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusWriteValueCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusWriteValueCtxDef &ctx_def = task_def.write_value_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusWriteValueCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusWriteValueCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->awSize = static_cast<uint8_t>(ctx_def.aw_size() & k3BitsMask);
  ctx->awSnoop = static_cast<uint8_t>(ctx_def.aw_snoop() & k1BitMask);
  ctx->awCache = static_cast<uint8_t>(ctx_def.aw_cache() & k4BitsMask);
  ctx->awProt = static_cast<uint8_t>(ctx_def.aw_prot() & k3BitsMask);
  ctx->awVa = static_cast<uint8_t>(ctx_def.aw_va() & k1BitMask);

  ctx->arSize = static_cast<uint8_t>(ctx_def.ar_size() & k3BitsMask);
  ctx->arSnoop = static_cast<uint8_t>(ctx_def.ar_snoop() & k1BitMask);
  ctx->arCache = static_cast<uint8_t>(ctx_def.ar_cache() & k4BitsMask);
  ctx->arProt = static_cast<uint8_t>(ctx_def.ar_prot() & k3BitsMask);
  ctx->arVa = static_cast<uint8_t>(ctx_def.ar_va() & k1BitMask);

  uint8_t *write_addr_base = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.write_addr_base(), write_addr_base) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic write addr base is 0x%lx.", ctx_def.write_addr_base());
    return INTERNAL_ERROR;
  }
  ctx->writeAddressBaseL = static_cast<uint32_t>(PtrToValue(write_addr_base) & k32BitsMask);
  ctx->writeAddressBaseH = static_cast<uint32_t>((PtrToValue(write_addr_base) >> 32U) & k17BitsMask);
  ctx->writeAddressOffset = ctx_def.write_addr_offset();

  if (ctx_def.write_value_size() > kWriteValueNum) {
    REPORT_INNER_ERROR("E19999", "Size of write_value in FftsPlusWriteValueCtxDef should not > %d, but %d exactly",
                       kWriteValueNum, ctx_def.write_value_size());
    GELOGE(FAILED, "[Check][Param] Size of write_value in FftsPlusWriteValueCtxDef should not > %d, but %d exactly",
           kWriteValueNum, ctx_def.write_value_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.write_value_size(); i++) {
    ctx->writeValue[i] = static_cast<uint16_t>(ctx_def.write_value(i));
  }

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitMixAicAivCtx(const domi::FftsPlusCtxDef &task_def,
                                               rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusMixAicAivCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusMixAicAivCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusMixAicAivCtxDef &ctx_def = task_def.mix_aic_aiv_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->prefetchConfig = static_cast<uint8_t>(ctx_def.prefetch_config());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusMixAicAivCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusMixAicAivCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->schem = static_cast<uint16_t>(ctx_def.schem() & k2BitsMask);
  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->prefetchEnableBitmap = static_cast<uint16_t>(ctx_def.prefetch_enable_bitmap() & k4BitsMask);
  ctx->prefetchOnceBitmap = static_cast<uint16_t>(ctx_def.prefetch_once_bitmap() & k4BitsMask);

  ctx->pmg = static_cast<uint16_t>(ctx_def.pmg() & k2BitsMask);
  ctx->ns = static_cast<uint16_t>(ctx_def.ns() & k1BitMask);
  ctx->partId = static_cast<uint16_t>(ctx_def.part_id() & k8BitsMask);
  ctx->qos = static_cast<uint16_t>(ctx_def.qos() & k4BitsMask);

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->nonTailBlockRatioN = static_cast<uint8_t>(ctx_def.non_tail_block_ratio_n());
  ctx->tailBlockRatioN = static_cast<uint8_t>(ctx_def.tail_block_ratio_n());

  ctx->nonTailBlockdim = static_cast<uint16_t>(ctx_def.non_tail_block_dim());
  ctx->tailBlockdim = static_cast<uint16_t>(ctx_def.tail_block_dim());

  const uint64_t task_param_ptr_base = args_base_ + (sizeof(void *) * io_addrs_.size());
  GELOGD("FftsPlusMixAicAivCtxDef: task param addr is %lu.", task_param_ptr_base);
  ctx->aicTaskParamPtrL = static_cast<uint32_t>(task_param_ptr_base & k32BitsMask);
  ctx->aicTaskParamPtrH = static_cast<uint16_t>((task_param_ptr_base >> 32U) & k16BitsMask);
  ctx->aivTaskParamPtrL = static_cast<uint32_t>(task_param_ptr_base & k32BitsMask);
  ctx->aivTaskParamPtrH = static_cast<uint16_t>((task_param_ptr_base >> 32U) & k16BitsMask);
  ctx->aicTaskParamPtrOffset = static_cast<uint16_t>(ctx_def.aic_task_param_ptr_offset());
  ctx->aivTaskParamPtrOffset = static_cast<uint16_t>(ctx_def.aiv_task_param_ptr_offset());
  int32_t start_addr = 0;
  const auto it = ctx_additional_data_.find(kModeInArgsFirstFieldVal0);
  if ((it != ctx_additional_data_.cend()) && (it->second.count(task_def.context_id()) > 0U)) {
    uint64_t mode_addr = 0U;
    uint32_t len = 0U;
    start_addr = 1;
    (void)mode_addr_idx_.emplace(io_addrs_.size());
    GE_CHK_RT_RET(rtGetC2cCtrlAddr(&mode_addr, &len));
    io_addrs_.emplace_back(static_cast<uintptr_t>(mode_addr));
    GELOGD("save mode addr:0x%lx to mix_aic/mix_aiv args.", mode_addr);
  }

  if (ctx_def.src_slot_size() > kSrcSlotNum) {
    REPORT_INNER_ERROR("E19999", "Size of src_slot in FftsPlusMixAicAivCtxDef should not > %d, but %d exactly",
                       kSrcSlotNum, ctx_def.src_slot_size());
    GELOGE(FAILED, "[Check][Param] Size of src_slot in FftsPlusMixAicAivCtxDef should not > %d, but %d exactly",
           kSrcSlotNum, ctx_def.src_slot_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.src_slot_size(); i++) {
    ctx->srcSlot[i] = static_cast<uint16_t>(ctx_def.src_slot(i));
  }

  if (ctx->threadDim == 0U) {
    GELOGD("Context thread dim is zero, Dynamic shape mode.");
    return SUCCESS;
  }

  if (ctx->atm == 0U) {
    GE_CHK_STATUS_RET(InitManualMixAicAivCtx(ctx_def, *ctx, start_addr), "Init MixAicAivCtx in manual mode failed");
  } else {
    GE_CHK_STATUS_RET(InitAutoMixAicAivCtx(ctx_def, *ctx, start_addr), "Init MixAicAivCtx in auto mode failed");
  }

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitManualMixAicAivCtx(const domi::FftsPlusMixAicAivCtxDef &ctx_def,
                                                     rtFftsPlusMixAicAivCtx_t &ctx, const int32_t start_idx) const {
  for (int32_t i = start_idx; i < ctx_def.task_addr_size(); ++i) {
    GELOGD("index %u, task addr is 0x%lx", i, ctx_def.task_addr(i));
    io_addrs_.emplace_back(ctx_def.task_addr(i));
  }

  // PcL for low 32 bits of pc, PcH for high 16 bits of pc
  if (ctx_def.kernel_name_size() != kManualMixAicAivCtxPcNum) {
    REPORT_INNER_ERROR("E19999", "Size of kernel_name in FftsPlusMixAicAivCtxDef should be %d, but %d exactly",
                       kManualMixAicAivCtxPcNum, ctx_def.kernel_name_size());
    GELOGE(FAILED, "[Check][Param] Size of kernel_name in FftsPlusMixAicAivCtxDef should be %d, but %d exactly",
           kManualMixAicAivCtxPcNum, ctx_def.kernel_name_size());
    return FAILED;
  }
  uint32_t aic_i_cache_prefetch_cnt = 0U;
  void *aic_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(
        addr_pref_handle_(ctx_def.kernel_name(kManualAicCtxIndex), aic_task_start_pc, aic_i_cache_prefetch_cnt),
        "Get addr and pref cnt failed, kernel_name=%s", ctx_def.kernel_name(kManualAicCtxIndex).c_str());
  }

  ctx.nonTailAicTaskStartPcL = static_cast<uint32_t>(PtrToValue(aic_task_start_pc) & k32BitsMask);
  ctx.nonTailAicTaskStartPcH = static_cast<uint16_t>((PtrToValue(aic_task_start_pc) >> 32U) & k16BitsMask);
  ctx.tailAicTaskStartPcL = static_cast<uint32_t>(PtrToValue(aic_task_start_pc) & k32BitsMask);
  ctx.tailAicTaskStartPcH = static_cast<uint16_t>((PtrToValue(aic_task_start_pc) >> 32U) & k16BitsMask);
  ctx.aicIcachePrefetchCnt = static_cast<uint16_t>(aic_i_cache_prefetch_cnt & k5BitsMask);

  uint32_t aiv_i_cache_prefetch_cnt = 0U;
  void *aiv_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(
        addr_pref_handle_(ctx_def.kernel_name(kManualAivCtxIndex), aiv_task_start_pc, aiv_i_cache_prefetch_cnt),
        "Get addr and pref cnt failed, kernel_name=%s", ctx_def.kernel_name(kManualAivCtxIndex).c_str());
  }

  ctx.nonTailAivTaskStartPcL = static_cast<uint32_t>(PtrToValue(aiv_task_start_pc) & k32BitsMask);
  ctx.nonTailAivTaskStartPcH = static_cast<uint16_t>((PtrToValue(aiv_task_start_pc) >> 32U) & k16BitsMask);
  ctx.tailAivTaskStartPcL = static_cast<uint32_t>(PtrToValue(aiv_task_start_pc) & k32BitsMask);
  ctx.tailAivTaskStartPcH = static_cast<uint16_t>((PtrToValue(aiv_task_start_pc) >> 32U) & k16BitsMask);
  ctx.aivIcachePrefetchCnt = static_cast<uint16_t>(aiv_i_cache_prefetch_cnt & k5BitsMask);

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAutoMixAicAivCtx(const domi::FftsPlusMixAicAivCtxDef &ctx_def,
                                                   rtFftsPlusMixAicAivCtx_t &ctx, const int32_t start_idx) const {
  if (ctx_def.save_task_addr() == kSaveTaskAddr) {
    for (uint16_t i = 0U; i < (ctx.threadDim - 1U); ++i) {
      InitThrdIoAddrs(ctx_def, i, ctx_def.task_addr_offset_size(), start_idx);
    }
    InitThrdIoAddrs(ctx_def, ctx.threadDim - 1U, static_cast<int32_t>(ctx_def.input_output_count()), start_idx);
    const int32_t last_thread_workspace_size = ctx_def.task_addr_size() - ctx_def.task_addr_offset_size() - start_idx;
    for (auto k = 0; k < last_thread_workspace_size; ++k) {
      uintptr_t logic_addr = ctx_def.task_addr(ctx_def.task_addr_offset_size() + k);
      io_addrs_.emplace_back(logic_addr);
    }
  }

  // PcL for low 32 bits of pc, PcH for high 16 bits of pc
  if (ctx_def.kernel_name_size() != kAutoMixAicAivCtxPcNum) {
    REPORT_INNER_ERROR("E19999", "Size of kernel_name in FftsPlusMixAicAivCtxDef should be %d, but %d exactly",
                       kAutoMixAicAivCtxPcNum, ctx_def.kernel_name_size());
    GELOGE(FAILED, "[Check][Param] Size of kernel_name in FftsPlusMixAicAivCtxDef should be %d, but %d exactly",
           kAutoMixAicAivCtxPcNum, ctx_def.kernel_name_size());
    return FAILED;
  }
  uint32_t non_tail_aic_i_cache_prefetch_cnt = 0U;
  void *non_tail_aic_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kAutoNonTailAicCtxIndexVal0), non_tail_aic_task_start_pc,
                                        non_tail_aic_i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s",
                      ctx_def.kernel_name(kAutoNonTailAicCtxIndexVal0).c_str());
  }

  ctx.nonTailAicTaskStartPcL = static_cast<uint32_t>(PtrToValue(non_tail_aic_task_start_pc) & k32BitsMask);
  ctx.nonTailAicTaskStartPcH = static_cast<uint16_t>((PtrToValue(non_tail_aic_task_start_pc) >> 32U) & k16BitsMask);
  uint32_t tail_aic_i_cache_prefetch_cnt = 0U;
  void *tail_aic_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kAutoTailAicCtxIndex), tail_aic_task_start_pc,
                                        tail_aic_i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s",
                      ctx_def.kernel_name(kAutoTailAicCtxIndex).c_str());
  }

  ctx.tailAicTaskStartPcL = static_cast<uint32_t>(PtrToValue(tail_aic_task_start_pc) & k32BitsMask);
  ctx.tailAicTaskStartPcH = static_cast<uint16_t>((PtrToValue(tail_aic_task_start_pc) >> 32U) & k16BitsMask);
  const uint32_t aic_i_cache_prefetch_cnt = std::min(non_tail_aic_i_cache_prefetch_cnt, tail_aic_i_cache_prefetch_cnt);
  ctx.aicIcachePrefetchCnt = static_cast<uint16_t>(aic_i_cache_prefetch_cnt & k5BitsMask);

  uint32_t non_tail_aiv_i_cache_prefetch_cnt = 0U;
  void *non_tail_aiv_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kAutoNonTailAivCtxIndexVal2), non_tail_aiv_task_start_pc,
                                        non_tail_aiv_i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s",
                      ctx_def.kernel_name(kAutoNonTailAivCtxIndexVal2).c_str());
  }

  ctx.nonTailAivTaskStartPcL = static_cast<uint32_t>(PtrToValue(non_tail_aiv_task_start_pc) & k32BitsMask);
  ctx.nonTailAivTaskStartPcH = static_cast<uint16_t>((PtrToValue(non_tail_aiv_task_start_pc) >> 32U) & k16BitsMask);
  uint32_t tail_aiv_i_cache_prefetch_cnt = 0U;
  void *tail_aiv_task_start_pc = nullptr;
  if (addr_pref_handle_ != nullptr) {
    GE_CHK_STATUS_RET(addr_pref_handle_(ctx_def.kernel_name(kAutoTailAivCtxIndex), tail_aiv_task_start_pc,
                                        tail_aiv_i_cache_prefetch_cnt),
                      "Get addr and pref cnt failed, kernel_name=%s",
                      ctx_def.kernel_name(kAutoTailAivCtxIndex).c_str());
  }

  ctx.tailAivTaskStartPcL = static_cast<uint32_t>(PtrToValue(tail_aiv_task_start_pc) & k32BitsMask);
  ctx.tailAivTaskStartPcH = static_cast<uint16_t>((PtrToValue(tail_aiv_task_start_pc) >> 32U) & k16BitsMask);
  const uint32_t aiv_i_cache_prefetch_cnt = std::min(non_tail_aiv_i_cache_prefetch_cnt, tail_aiv_i_cache_prefetch_cnt);
  ctx.aivIcachePrefetchCnt = static_cast<uint16_t>(aiv_i_cache_prefetch_cnt & k5BitsMask);

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitSdmaCtx(const domi::FftsPlusCtxDef &task_def,
                                          rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusSdmaCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusSdmaCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusSdmaCtxDef &ctx_def = task_def.sdma_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusSdmaCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusSdmaCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->atm = static_cast<uint8_t>(ctx_def.atm() & k1BitMask);
  ctx->pmg = static_cast<uint16_t>(ctx_def.pmg() & k2BitsMask);
  ctx->ns = static_cast<uint16_t>(ctx_def.ns() & k1BitMask);
  ctx->partId = static_cast<uint16_t>(ctx_def.part_id() & k8BitsMask);
  ctx->qos = static_cast<uint16_t>(ctx_def.qos() & k4BitsMask);

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->sdmaSqeHeader = ctx_def.sdma_sqe_header();

  ctx->sourceStreamId = static_cast<uint16_t>(ctx_def.src_stream_id());
  ctx->sourceSubstreamId = static_cast<uint16_t>(ctx_def.src_sub_stream_id());

  ctx->destinationStreamId = static_cast<uint16_t>(ctx_def.dst_stream_id());
  ctx->destinationSubstreamId = static_cast<uint16_t>(ctx_def.dst_sub_stream_id());

  uint8_t *src_addr_base = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.src_addr_base(), src_addr_base) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic src addr is 0x%lx.", ctx_def.src_addr_base());
    return INTERNAL_ERROR;
  }
  ctx->sourceAddressBaseL = static_cast<uint32_t>(PtrToValue(src_addr_base) & k32BitsMask);
  ctx->sourceAddressBaseH = static_cast<uint32_t>(PtrToValue(src_addr_base) >> 32U);
  ctx->sourceAddressOffset = ctx_def.src_addr_offset();

  uint8_t *dst_addr_base = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.dst_addr_base(), dst_addr_base) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic dst addr is 0x%lx.", ctx_def.dst_addr_base());
    return INTERNAL_ERROR;
  }
  ctx->destinationAddressBaseL = static_cast<uint32_t>(PtrToValue(dst_addr_base) & k32BitsMask);
  ctx->destinationAddressBaseH = static_cast<uint32_t>(PtrToValue(dst_addr_base) >> 32U);
  ctx->destinationAddressOffset = ctx_def.dst_addr_offset();

  ctx->nonTailDataLength = ctx_def.non_tail_data_len();
  ctx->tailDataLength = ctx_def.tail_data_len();

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitDataCtx(const domi::FftsPlusCtxDef &task_def,
                                          rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusDataCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusDataCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusDataCtxDef &ctx_def = task_def.data_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->cntInit = static_cast<uint8_t>(ctx_def.cnt_init());
  ctx->cnt = static_cast<uint8_t>(ctx_def.cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusDataCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusDataCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->atm = static_cast<uint8_t>(ctx_def.atm() & k1BitMask);
  ctx->pmg = static_cast<uint16_t>(ctx_def.pmg() & k2BitsMask);
  ctx->ns = static_cast<uint16_t>(ctx_def.ns() & k1BitMask);
  ctx->partId = static_cast<uint16_t>(ctx_def.part_id() & k8BitsMask);
  ctx->qos = static_cast<uint16_t>(ctx_def.qos() & k4BitsMask);

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());
  ctx->origConsumerCounter = static_cast<uint16_t>(ctx_def.orig_consumer_counter());
  ctx->runConsumerCounter = static_cast<uint16_t>(ctx_def.run_consumer_counter());

  uint8_t *addr_base = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.addr_base(), addr_base) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic addr base is 0x%lx.", ctx_def.addr_base());
    return INTERNAL_ERROR;
  }
  ctx->addressBaseL = static_cast<uint32_t>(PtrToValue(addr_base) & k32BitsMask);
  ctx->addressBaseH = static_cast<uint32_t>(PtrToValue(addr_base) >> 32U);
  ctx->addressOffset = ctx_def.addr_offset();

  ctx->nonTailNumOutter = static_cast<uint16_t>(ctx_def.non_tail_num_outter());
  ctx->nonTailNumInner = static_cast<uint16_t>(ctx_def.non_tail_num_inner());
  ctx->nonTailLengthInner = ctx_def.non_tail_len_inner();
  ctx->nonTailStrideOutter = ctx_def.non_tail_stride_outter();
  ctx->nonTailStrideInner = ctx_def.non_tail_stride_inner();

  ctx->tailNumOutter = static_cast<uint16_t>(ctx_def.tail_num_outter());
  ctx->tailNumInner = static_cast<uint16_t>(ctx_def.tail_num_inner());
  ctx->tailLengthInner = ctx_def.tail_len_inner();
  ctx->tailStrideOutter = ctx_def.tail_stride_outter();
  ctx->tailStrideInner = ctx_def.tail_stride_inner();

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuCtx(const domi::FftsPlusCtxDef &task_def,
                                           rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusAiCpuCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusAiCpuCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusAicpuCtxDef &ctx_def = task_def.aicpu_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusAicpuCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusAicpuCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorContextID[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->sqeIndex = static_cast<uint16_t>(ctx_def.sqe_index());
  ctx->kernelType = static_cast<uint8_t>(ctx_def.kernel_type() & k7BitsMask);
  ctx->bm = static_cast<uint8_t>(ctx_def.bm() & k1BitMask);
  ctx->topicType = static_cast<uint8_t>(ctx_def.topic_type() & k4BitsMask);
  ctx->qos = static_cast<uint8_t>(ctx_def.qos() & k3BitsMask);

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());
  ctx->nonTailBlockdim = static_cast<uint16_t>(ctx_def.non_tail_block_dim());
  ctx->tailBlockdim = static_cast<uint16_t>(ctx_def.tail_block_dim());

  ctx->subtopicId = static_cast<uint32_t>(ctx_def.sub_topic_id() & k12BitsMask);
  ctx->topicId = static_cast<uint32_t>(ctx_def.topic_id() & k6BitsMask);
  ctx->groupId = static_cast<uint32_t>(ctx_def.group_id() & k6BitsMask);
  ctx->taskParamOffset = ctx_def.task_param_offset();
  const auto op_desc = find_node_handle_(task_def.op_index());
  GE_CHK_STATUS_RET(InitAicpuCtxUserData(op_desc, ctx_def, *ctx), "Init user data for aicpu ctx failed");

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuCtxUserData(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def,
                                                   rtFftsPlusAiCpuCtx_t &ctx) const {
  GE_CHECK_NOTNULL(op_desc);
  const auto user_data_len = sizeof(ctx.usrData) / sizeof(uint32_t);
  if (user_data_len < static_cast<uint64_t>(kRequiredUserDataNum)) {
    REPORT_INNER_ERROR("E19999", "Length of user_data in rtFftsPlusAiCpuCtx_t should not < %d, but %lu exactly",
                       kRequiredUserDataNum, user_data_len);
    GELOGE(FAILED, "[Check][Param] Length of user_data in rtFftsPlusAiCpuCtx_t should not < %d, but %lu exactly",
           kRequiredUserDataNum, user_data_len);
    return FAILED;
  }
  ctx.usrDataLength = static_cast<uint32_t>(kRequiredUserDataNum);
  const auto &kernel = ctx_def.kernel();

  // copy so_name
  const auto &so_name = kernel.so_name();
  void *so_name_addr = nullptr;
  GE_CHK_RT_RET(rtMalloc(&so_name_addr, so_name.size(), RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(so_name_addr);
  GE_CHK_RT_RET(rtMemcpy(so_name_addr, so_name.size(), so_name.data(), so_name.size(), RT_MEMCPY_HOST_TO_DEVICE));
  ctx.usrData[kSoNameAddrLIndex] = static_cast<uint32_t>(PtrToValue(so_name_addr) & k32BitsMask);
  ctx.usrData[kSoNameAddrHIndex] = static_cast<uint32_t>(PtrToValue(so_name_addr) >> 32U);

  // get args addr
  void *args_addr = nullptr;
  GE_CHK_STATUS_RET(InitAicpuInfo(op_desc, ctx_def, args_addr));
  ctx.usrData[kArgsAddrLIndex] = static_cast<uint32_t>(PtrToValue(args_addr) & k32BitsMask);
  ctx.usrData[kArgsAddrHIndex] = static_cast<uint32_t>(PtrToValue(args_addr) >> 32U);

  // copy kernel_name
  const auto &kernel_name = kernel.kernel_name();
  void *kernel_name_addr = nullptr;
  GE_CHK_RT_RET(rtMalloc(&kernel_name_addr, kernel_name.size(), RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(kernel_name_addr);
  GE_CHK_RT_RET(rtMemcpy(kernel_name_addr, kernel_name.size(), kernel_name.data(), kernel_name.size(),
                         RT_MEMCPY_HOST_TO_DEVICE));
  ctx.usrData[kKernelNameAddrLIndex] = static_cast<uint32_t>(PtrToValue(kernel_name_addr) & k32BitsMask);
  ctx.usrData[kKernelNameAddrHIndex] = static_cast<uint32_t>(PtrToValue(kernel_name_addr) >> 32U);
  GELOGD("Init aicpu ctx user data success, node is [%s], type is [%s], so name is [%s], kernel name is [%s]",
         op_desc->GetName().c_str(), op_desc->GetType().c_str(), so_name.c_str(), kernel_name.c_str());
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def,
                                            void *&addr) const {
  // aicpu fwk op
  if (ctx_def.kernel_type() == kFwkAicpuKernelType) {
    return InitAicpuFwkExtInfo(op_desc, ctx_def, addr);
  }
  if (ctx_def.kernel_type() == kCustomAicpuKernelType) {
    load_cust_aicpu_so_(op_desc, ctx_def);
  }
  return InitAicpuExtInfo(op_desc, ctx_def, addr);
}

Status FftsPlusProtoTransfer::InitAicpuFwkAddrInfo(const OpDescPtr &op_desc, uint8_t *const ori_args_addr,
                                                   const size_t args_size) const {
  const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(runtime_param_, op_desc);
  if (workspace_data_addrs.empty()) {
    GELOGI("[Check][Param] workspace_data_addrs is empty in op:%s(%s), take for dynamic shape.",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return SUCCESS;
  }

  const auto fwk_op_kernel = PtrToPtr<uint8_t, STR_FWK_OP_KERNEL>(ori_args_addr);
  const auto task_info_addr = PtrAdd(ori_args_addr, args_size, sizeof(STR_FWK_OP_KERNEL));
  const size_t task_info_addr_size = args_size - sizeof(STR_FWK_OP_KERNEL);
  const auto ret = CopyTaskInfoToWorkspace(op_desc, static_cast<const void *>(task_info_addr), task_info_addr_size);
  if (ret != SUCCESS) {
    GELOGE(ret, "[copy][TaskInfo] to workspace failed, op:%s.", op_desc->GetName().c_str());
    return ret;
  }

  const std::vector<void *> input_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  const std::vector<void *> output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  std::vector<void *> io_addrs;
  (void)io_addrs.insert(io_addrs.cend(), input_addrs.cbegin(), input_addrs.cend());
  (void)io_addrs.insert(io_addrs.cend(), output_addrs.cbegin(), output_addrs.cend());

  const auto addrs_size = sizeof(uint64_t) * (io_addrs.size());
  void *input_output_addr = nullptr;
  if (addrs_size > 0U) {
    GE_CHK_RT_RET(rtMalloc(&input_output_addr, addrs_size, RT_MEMORY_HBM));
    ext_info_addrs_.emplace_back(input_output_addr);
    GE_CHK_RT_RET(rtMemcpy(input_output_addr, addrs_size, io_addrs.data(), addrs_size, RT_MEMCPY_HOST_TO_DEVICE));
  }

  fwk_op_kernel->fwkKernelBase.fwk_kernel.workspaceBaseAddr = PtrToValue(workspace_data_addrs[0U]);
  fwk_op_kernel->fwkKernelBase.fwk_kernel.inputOutputAddr = PtrToValue(input_output_addr);
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuFwkExtInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def,
                                                  void *&addr) const {
  GELOGI("Begin to init aicpu fwk op ext info.");
  const auto &kernel = ctx_def.kernel();

  // create host args
  const size_t args_size = kernel.args().size();
  std::vector<uint8_t> ori_args_addr(kernel.args().data(), &kernel.args()[args_size]);

  // 1 get loop cond variable for tensor array
  const auto fwk_op_kernel = PtrToPtr<unsigned char, STR_FWK_OP_KERNEL>(ori_args_addr.data());
  GE_CHK_STATUS_RET_NOLOG(create_aicpu_session_(*fwk_op_kernel));

  const auto &ext_info = kernel.kernel_ext_info();
  std::shared_ptr<hybrid::AicpuExtInfoHandler> ext_handle;
  GE_CHK_STATUS_RET_NOLOG(InitAicpuTaskExtInfo(op_desc, ext_info, ext_handle));
  if (ext_handle == nullptr) {
    return SUCCESS; // ext_info is empty.
  }

  void *aicpu_ext_info_addr = nullptr;
  GE_CHK_RT_RET(rtMalloc(&aicpu_ext_info_addr, ext_handle->GetExtInfoLen(), RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(aicpu_ext_info_addr);
  GE_CHK_RT_RET(rtMemcpy(aicpu_ext_info_addr, ext_handle->GetExtInfoLen(), ext_handle->GetExtInfo(),
                         ext_handle->GetExtInfoLen(), RT_MEMCPY_HOST_TO_DEVICE));

  GELOGI("Node[%s] type[%s] kernel_ext_info size=%zu, aicpu_ext_info_addr=%p", op_desc->GetName().c_str(),
         op_desc->GetType().c_str(), ext_info.size(), aicpu_ext_info_addr);

  // 4 set workspace addr and input output data addr
  fwk_op_kernel->fwkKernelBase.fwk_kernel.extInfoLen = ext_info.size();
  fwk_op_kernel->fwkKernelBase.fwk_kernel.extInfoAddr = PtrToValue(aicpu_ext_info_addr);
  GE_CHK_STATUS_RET(InitAicpuFwkAddrInfo(op_desc, ori_args_addr.data(), args_size),
                    "[Init][InitAicpuFwkAddrInfo] failed, ext info size is %zu, op is %s", args_size,
                    op_desc->GetName().c_str());

  // 5. Return result
  GE_CHK_RT_RET(rtMalloc(&addr, sizeof(STR_FWK_OP_KERNEL), RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(addr);
  GE_CHK_RT_RET(rtMemcpy(addr, sizeof(STR_FWK_OP_KERNEL), static_cast<void *>(fwk_op_kernel),
                         sizeof(STR_FWK_OP_KERNEL), RT_MEMCPY_HOST_TO_DEVICE));
  GELOGI("Init aicpu fwk op context info Success. session id: %lu", fwk_op_kernel->fwkKernelBase.fwk_kernel.sessionID);
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuExtInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def,
                                               void *&addr) const {
  GELOGI("Begin to init aicpu op ext info.");
  if (ctx_def.thread_dim() == 0U) {
    return save_aicpu_ctx_handle_(op_desc, ctx_def.kernel());
  }

  const auto &kernel = ctx_def.kernel();
  // copy args to new host addr
  const size_t args_size = kernel.args().size();
  std::vector<uint8_t> ori_args_addr(kernel.args().data(), &kernel.args()[args_size]);

  // ctx_def.atm() == 0 is aicpu manual mode(only once looping), else is auto mode
  const auto thread_num = ctx_def.thread_dim();
  const auto task_param_offset = static_cast<size_t>(ctx_def.task_param_offset());
  GELOGI("[Init ffts plus aicpu ext info] thread num is %u, atm is %u, task_param_offset is %lu", thread_num,
         ctx_def.atm(), task_param_offset);
  const auto &ext_info = kernel.kernel_ext_info();
  std::shared_ptr<hybrid::AicpuExtInfoHandler> ext_handle;
  GE_CHK_STATUS_RET_NOLOG(InitAicpuTaskExtInfo(op_desc, ext_info, ext_handle));
  if (ext_handle == nullptr) {
    return SUCCESS; // ext_info is empty.
  }

  void *aicpu_ext_info_addr = nullptr;
  GE_CHK_RT_RET(rtMalloc(&aicpu_ext_info_addr, ext_handle->GetExtInfoLen() * thread_num, RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(aicpu_ext_info_addr);

  // Dynamic shape mode will skip for thread dim is zero.
  for (size_t i = 0U; i < thread_num; ++i) {
    void *const ctx_ext_info_addr = ValueToPtr(PtrToValue(aicpu_ext_info_addr) + (ext_handle->GetExtInfoLen() * i));
    GE_CHK_RT_RET(rtMemcpy(ctx_ext_info_addr, ext_handle->GetExtInfoLen(), ext_handle->GetExtInfo(),
                           ext_handle->GetExtInfoLen(), RT_MEMCPY_HOST_TO_DEVICE));
    GELOGI("Node[%s] type[%s] kernel_ext_info size=%zu, current thread aicpu ext_info_addr=%p",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), ext_info.size(), ctx_ext_info_addr);
    const auto aicpu_param_head = PtrToPtr<uint8_t, aicpu::AicpuParamHead>(&ori_args_addr[i * task_param_offset]);
    aicpu_param_head->extInfoAddr = PtrToValue(ctx_ext_info_addr);
    aicpu_param_head->extInfoLength = static_cast<uint32_t>(ext_info.size());
    const uintptr_t io_addr = PtrToValue(&ori_args_addr[(i * task_param_offset) + sizeof(aicpu::AicpuParamHead)]);
    GE_CHK_STATUS_RET(InitAicpuIoAddrs(op_desc, io_addr), "Init aicpu[%s] io addrs failed", op_desc->GetName().c_str());
  }

  // malloc device memory for args
  GE_CHK_RT_RET(rtMalloc(&addr, args_size, RT_MEMORY_HBM));
  ext_info_addrs_.emplace_back(addr);
  // copy args to device
  GE_CHK_RT_RET(rtMemcpy(addr, args_size, static_cast<void *>(ori_args_addr.data()), args_size,
                         RT_MEMCPY_HOST_TO_DEVICE));
  GELOGI("Init aicpu op context info success");
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuTaskExtInfo(const OpDescPtr &op_desc, const std::string &ext_info,
                                                   std::shared_ptr<hybrid::AicpuExtInfoHandler> &ext_handle) const {
  if (ext_info.empty()) {
    return SUCCESS;
  }

  int32_t unknown_shape_type_val = 0;
  (void)AttrUtils::GetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type_val);
  const UnknowShapeOpType unknown_type = static_cast<UnknowShapeOpType>(unknown_shape_type_val);
  const uint32_t num_inputs = static_cast<uint32_t>(op_desc->GetInputsSize());
  const uint32_t num_outputs = static_cast<uint32_t>(op_desc->GetOutputsSize());
  ext_handle = MakeShared<hybrid::AicpuExtInfoHandler>(op_desc->GetName(), num_inputs, num_outputs, unknown_type);
  GE_CHK_BOOL_RET_STATUS(ext_handle != nullptr, FAILED, "[Malloc][Memory] for aicpu_ext_handle failed!");
  GE_CHK_STATUS_RET(ext_handle->Parse(ext_info),
                    "[Parse][KernelExtInfo] failed, kernel_ext_info_size=%zu, op:%s.",
                    ext_info.size(), op_desc->GetName().c_str());
  GE_CHK_STATUS_RET(ext_handle->UpdateSessionInfoId(aicpu_get_session_id_()),
                    "[Update][SessionInfoSessionId] failed, op:%s", op_desc->GetName().c_str());
  GELOGD("Update aicpu_task ext_info session_info session_id is %lu", aicpu_get_session_id_());
  GE_CHK_STATUS_RET(ext_handle->UpdateExecuteMode(true),
                    "[Update][ExecuteMode] failed, op:%s", op_desc->GetName().c_str());
  GELOGD("Update aicpu_task ext_info bit_map execute mode to 1.");
  bool all_shape = false;
  (void)AttrUtils::GetBool(op_desc, kAicpuAllshape, all_shape);
  if (all_shape) {
    GELOGD("Aicpu all_shape kernel need to update io shape.");
    for (uint32_t i = 0U; i < num_inputs; i++) {
      const auto input_desc = op_desc->MutableInputDesc(i);
      GE_CHECK_NOTNULL(input_desc);
      GE_CHK_STATUS_RET(ext_handle->UpdateInputShapeAndType(i, *input_desc),
                        "[Call][UpdateInputShapeAndType] Input[%u] update input shape failed, op:%s.",
                        i, op_desc->GetName().c_str());
    }
    for (uint32_t j = 0U; j < num_outputs; j++) {
      const auto output_desc = op_desc->MutableOutputDesc(j);
      GE_CHECK_NOTNULL(output_desc);
      GE_CHK_STATUS_RET(ext_handle->UpdateOutputShapeAndType(j, *output_desc),
                        "[Call][UpdateOutputShapeAndType] Output[%u] update output shape failed, op:%s.",
                        j, op_desc->GetName().c_str());
    }
  }

  return SUCCESS;
}

Status FftsPlusProtoTransfer::CopyTaskInfoToWorkspace(const OpDescPtr &op_desc, const void *const task_info_addr,
                                                      const size_t task_info_addr_size) const {
  // Userspace copy need virtual address.
  const std::vector<int64_t> workspace_data_sizes = ModelUtils::GetWorkspaceSize(op_desc);
  const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(runtime_param_, op_desc);
  if (workspace_data_addrs.empty() || workspace_data_sizes.empty()) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) workspace addr:%zu or size:%zu empty, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       workspace_data_addrs.size(), workspace_data_sizes.size());
    GELOGE(FAILED, "[Check][Param] Node:%s invalid workspace, addrs is %zu, size is %zu.", op_desc->GetName().c_str(),
           workspace_data_addrs.size(), workspace_data_sizes.size());
    return FAILED;
  }

  if (workspace_data_addrs[0U] == nullptr) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) workspace addr is nullptr, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] Node:%s workspace addrs is null.", op_desc->GetName().c_str());
    return FAILED;
  }

  if ((workspace_data_sizes[0U] < static_cast<int64_t>(task_info_addr_size)) ||
      (workspace_data_sizes[0U] > static_cast<int64_t>(runtime_param_.mem_size))) {
    REPORT_INNER_ERROR("E19999",
                       "Node:%s(%s) workspace size:%ld < task info size:%zu or workspace size > total mem "
                       "size %lu, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), workspace_data_sizes[0U],
                       task_info_addr_size, runtime_param_.mem_size);
    GELOGE(FAILED,
           "[Check][Param] Node:%s workspace size is %ld, task info size is %zu or workspace size > total mem "
           "size %lu, check invalid",
           op_desc->GetName().c_str(), workspace_data_sizes[0U], task_info_addr_size, runtime_param_.mem_size);
    return FAILED;
  }

  GE_CHK_RT_RET(rtMemcpy(workspace_data_addrs[0U], static_cast<uint64_t>(workspace_data_sizes[0U]),
                         task_info_addr, static_cast<uint64_t>(task_info_addr_size), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAicpuIoAddrs(const OpDescPtr &op_desc, const uintptr_t &io_addr) const {
  const std::vector<void *> input_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  const std::vector<void *> output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  const vector<int64_t> input_size = ModelUtils::GetInputSize(op_desc);
  const vector<int64_t> output_size = ModelUtils::GetOutputSize(op_desc);
  if (input_addrs.size() != input_size.size()) {
    GELOGE(FAILED, "input addrs size[%zu] is not equal to input size[%zu]", input_addrs.size(), input_size.size());
    return FAILED;
  }
  if (output_addrs.size() != output_size.size()) {
    GELOGE(FAILED, "output addrs size[%zu] is not equal to output size[%zu]", output_addrs.size(), output_size.size());
    return FAILED;
  }
  std::vector<void *> io_addrs;
  const uint64_t *const input_offset = PtrToPtr<void, uint64_t>(ValueToPtr(io_addr));
  for (size_t i = 0U; i < input_addrs.size(); ++i) {
    const auto step = input_offset[i];
    io_addrs.emplace_back(ValueToPtr(PtrToValue(input_addrs[i]) + step));
    GELOGI("Node[%s] type[%s] index[%zu] size=%zu, input size[%lu], input addr=%p", op_desc->GetName().c_str(),
           op_desc->GetType().c_str(), i, step, input_offset[i], ValueToPtr(PtrToValue(input_addrs[i]) + step));
  }

  const uint64_t *const output_offset =
    PtrToPtr<void, uint64_t>(ValueToPtr(io_addr + (input_addrs.size() * sizeof(uint64_t))));
  for (size_t i = 0U; i < output_addrs.size(); ++i) {
    const auto step = output_offset[i];
    io_addrs.emplace_back(ValueToPtr(PtrToValue(output_addrs[i]) + step));
    GELOGI("Node[%s] type[%s] index[%zu] size=%zu, output size[%lu], output addr=%p", op_desc->GetName().c_str(),
           op_desc->GetType().c_str(), i, step, output_offset[i], ValueToPtr(PtrToValue(output_addrs[i]) + step));
  }

  GELOGI("Node[%s] type[%s] io_addrs size is [%zu]", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
         io_addrs.size());
  if (!io_addrs.empty()) {
    const auto addrs_size = sizeof(uint64_t) * io_addrs.size();
    const errno_t sec_ret = memcpy_s(ValueToPtr(io_addr), addrs_size, io_addrs.data(), addrs_size);
    if (sec_ret != EOK) {
      REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%lu, ret:0x%X", addrs_size, sec_ret);
      GELOGE(FAILED, "[Call][Memcpy] failed, size:%lu, ret:0x%X", addrs_size, sec_ret);
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitCondSwitchCtx(const domi::FftsPlusCtxDef &task_def,
                                                rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusCondSwitchCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusCondSwitchCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusCondSwitchCtxDef &ctx_def = task_def.cond_switch_ctx();
  ctx->trueSuccessorNum = static_cast<uint8_t>(ctx_def.true_successor_num());
  ctx->falseSuccessorNum = static_cast<uint8_t>(ctx_def.false_successor_num() & k7BitsMask);
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);

  if (ctx_def.condition() == RT_COND_TYPE_MAX) {
    REPORT_INNER_ERROR("E19999", "Unsupported cond type %u", ctx_def.condition());
    GELOGE(FAILED, "[Check][CtxType] Unsupported cond type %u", ctx_def.condition());
    return FAILED;
  }
  ctx->condition = static_cast<rtFftsPlusCondType_t>(ctx_def.condition());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.true_successor_list_size() > RT_CTX_TRUE_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999",
                       "Size of true_successor_list in FftsPlusCondSwitchCtxDef should not > %d, but %d exactly",
                       RT_CTX_TRUE_SUCCESSOR_NUM, ctx_def.true_successor_list_size());
    GELOGE(FAILED,
           "[Check][Param] Size of true_successor_list in FftsPlusCondSwitchCtxDef should not > %d, but %d exactly",
           RT_CTX_TRUE_SUCCESSOR_NUM, ctx_def.true_successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.true_successor_list_size(); i++) {
    ctx->trueSuccessorList[i] = static_cast<uint16_t>(ctx_def.true_successor_list(i));
  }

  if (ctx_def.false_successor_list_size() > RT_CTX_FALSE_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999",
                       "Size of false_successor_list in FftsPlusCondSwitchCtxDef should not > %d, but %d exactly",
                       RT_CTX_FALSE_SUCCESSOR_NUM, ctx_def.false_successor_list_size());
    GELOGE(FAILED,
           "[Check][Param] Size of false_successor_list in FftsPlusCondSwitchCtxDef should not > %d, but %d exactly",
           RT_CTX_FALSE_SUCCESSOR_NUM, ctx_def.false_successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.false_successor_list_size(); i++) {
    ctx->falseSuccessorList[i] = static_cast<uint16_t>(ctx_def.false_successor_list(i));
  }

  ctx->atm = static_cast<uint16_t>(ctx_def.atm() & k1BitMask);
  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->arSize = static_cast<uint8_t>(ctx_def.ar_size() & k3BitsMask);
  ctx->snoop = static_cast<uint8_t>(ctx_def.snoop() & k1BitMask);
  ctx->arCache = static_cast<uint8_t>(ctx_def.ar_cache() & k4BitsMask);
  ctx->arProt = static_cast<uint8_t>(ctx_def.ar_prot() & k3BitsMask);
  ctx->va = static_cast<uint8_t>(ctx_def.va() & k1BitMask);

  uint8_t *addr_base_0 = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.load_addr0_base(), addr_base_0) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic load addr0 base is 0x%lx.", ctx_def.load_addr0_base());
    return INTERNAL_ERROR;
  }
  ctx->loadAddress0BaseL = static_cast<uint32_t>(PtrToValue(addr_base_0) & k32BitsMask);
  ctx->loadAddress0BaseH = static_cast<uint32_t>((PtrToValue(addr_base_0) >> 32U) & k17BitsMask);
  ctx->ld0En = static_cast<uint32_t>(ctx_def.ld0_en() & k1BitMask);
  ctx->loadAddress0Offset = ctx_def.load_addr0_offset();

  uint8_t *addr_base_1 = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.load_addr1_base(), addr_base_1) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic load addr1 base is 0x%lx.", ctx_def.load_addr1_base());
    return INTERNAL_ERROR;
  }
  ctx->loadAddress1BaseL = static_cast<uint32_t>(PtrToValue(addr_base_1) & k32BitsMask);
  ctx->loadAddress1BaseH = static_cast<uint32_t>((PtrToValue(addr_base_1) >> 32U) & k17BitsMask);
  ctx->ld1En = static_cast<uint32_t>(ctx_def.ld1_en() & k1BitMask);
  ctx->loadAddress1Offset = ctx_def.load_addr1_offset();

  ctx->cmpValue1 = ctx_def.cmp_value_1();
  ctx->cmpValue2 = ctx_def.cmp_value_2();

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitCaseSwitchCtx(const domi::FftsPlusCtxDef &task_def,
                                                rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusCaseSwitchCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusCaseSwitchCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusCaseSwitchCtxDef &ctx_def = task_def.case_switch_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);

  ctx->startLabelId = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->labelListLen = static_cast<uint8_t>(ctx_def.label_list_len());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusCaseDefaultCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusCaseDefaultCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->atm = static_cast<uint8_t>(ctx_def.atm() & k1BitMask);
  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->arSize = static_cast<uint8_t>(ctx_def.ar_size() & k3BitsMask);
  ctx->snoop = static_cast<uint8_t>(ctx_def.snoop() & k1BitMask);
  ctx->arCache = static_cast<uint8_t>(ctx_def.ar_cache() & k4BitsMask);
  ctx->arProt = static_cast<uint8_t>(ctx_def.ar_prot() & k3BitsMask);
  ctx->va = static_cast<uint8_t>(ctx_def.va() & k1BitMask);

  uint8_t *addr_base_0 = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.load_addr0_base(), addr_base_0) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic load addr0 base is 0x%lx.", ctx_def.load_addr0_base());
    return INTERNAL_ERROR;
  }
  ctx->loadAddress0BaseL = static_cast<uint32_t>(PtrToValue(addr_base_0) & k32BitsMask);
  ctx->loadAddress0BaseH = static_cast<uint32_t>((PtrToValue(addr_base_0) >> 32U) & k17BitsMask);
  ctx->ld0En = static_cast<uint32_t>(ctx_def.ld0_en() & k1BitMask);
  ctx->loadAddress0Offset = ctx_def.load_addr0_offset();

  uint8_t *addr_base_1 = nullptr;
  if ((run_addr_handle_ != nullptr) && (run_addr_handle_(ctx_def.load_addr1_base(), addr_base_1) != SUCCESS)) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic load addr1 base is 0x%lx.", ctx_def.load_addr1_base());
    return INTERNAL_ERROR;
  }
  ctx->loadAddress1BaseL = static_cast<uint32_t>(PtrToValue(addr_base_1) & k32BitsMask);
  ctx->loadAddress1BaseH = static_cast<uint32_t>((PtrToValue(addr_base_1) >> 32U) & k17BitsMask);
  ctx->ld1En = static_cast<uint32_t>(ctx_def.ld1_en() & k1BitMask);
  ctx->loadAddress1Offset = ctx_def.load_addr1_offset();

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitCaseDefaultCtx(const domi::FftsPlusCtxDef &task_def,
                                                 rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusCaseDefCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusCaseDefCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusCaseDefaultCtxDef &ctx_def = task_def.case_default_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);

  ctx->startLabelId = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->labelListLen = static_cast<uint8_t>(ctx_def.label_list_len());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusCaseDefaultCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusCaseDefaultCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitCaseCtx(const domi::FftsPlusCtxDef &task_def,
                                          rtFftsPlusComCtx_t *const com_ctx) const {
  if (static_cast<int32_t>(task_def.has_case_switch_ctx()) == static_cast<int32_t>(task_def.has_case_default_ctx())) {
    REPORT_INNER_ERROR("E19999", "case_switch_ctx %s and case_default_ctx %s when software ctx type is case",
                       task_def.has_case_switch_ctx() ? "exist" : "not exist",
                       task_def.has_case_default_ctx() ? "exist" : "not exist");
    GELOGE(FAILED, "[Check][Ctx] case_switch_ctx %s and case_default_ctx %s when software ctx type is case",
           task_def.has_case_switch_ctx() ? "exist" : "not exist",
           task_def.has_case_default_ctx() ? "exist" : "not exist");
    return FAILED;
  }

  if (task_def.has_case_switch_ctx()) {
    GE_CHK_STATUS_RET(InitCaseSwitchCtx(task_def, com_ctx), "Init CaseSwitchCtx failed");
  }
  if (task_def.has_case_default_ctx()) {
    GE_CHK_STATUS_RET(InitCaseDefaultCtx(task_def, com_ctx), "Init CaseDefaultCtx failed");
  }
  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAtStartCtx(const domi::FftsPlusCtxDef &task_def,
                                             rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusAtStartCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusAtStartCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusAtStartCtxDef &ctx_def = task_def.at_start_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusAtStartCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusAtStartCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());
  ctx->threadDim = static_cast<uint16_t>(ctx_def.thread_dim());

  ctx->threadIdInit = static_cast<uint16_t>(ctx_def.thread_id_init());
  ctx->threadWindowSize = static_cast<uint16_t>(ctx_def.thread_window_size());

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitAtEndCtx(const domi::FftsPlusCtxDef &task_def,
                                           rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusAtEndCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusAtEndCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusAtEndCtxDef &ctx_def = task_def.at_end_ctx();
  ctx->atStartSlotNumber = static_cast<uint8_t>(ctx_def.at_start_slot_num());
  ctx->outLabelSlotNumber = static_cast<uint8_t>(ctx_def.out_label_slot_num() & k7BitsMask);

  ctx->aten = static_cast<uint8_t>(ctx_def.aten() & k1BitMask);
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.succ_at_start_slot_size() > RT_CTX_SUCC_AT_START_SLOT_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of succ_at_start_slot in FftsPlusAtEndCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCC_AT_START_SLOT_NUM, ctx_def.succ_at_start_slot_size());
    GELOGE(FAILED, "[Check][Param] Size of succ_at_start_slot in FftsPlusAtStartCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCC_AT_START_SLOT_NUM, ctx_def.succ_at_start_slot_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.succ_at_start_slot_size(); i++) {
    ctx->succAtStartSlot[i] = static_cast<uint16_t>(ctx_def.succ_at_start_slot(i));
  }

  if (ctx_def.succ_out_label_slot_size() > RT_CTX_SUCC_OUT_LABEL_SLOT_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of succ_out_label_slot in FftsPlusAtEndCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCC_OUT_LABEL_SLOT_NUM, ctx_def.succ_out_label_slot_size());
    GELOGE(FAILED,
           "[Check][Param] Size of succ_out_label_slot in FftsPlusAtStartCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCC_OUT_LABEL_SLOT_NUM, ctx_def.succ_out_label_slot_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.succ_out_label_slot_size(); i++) {
    ctx->succOutLabelSlot[i] = static_cast<uint16_t>(ctx_def.succ_out_label_slot(i));
  }

  ctx->threadId = static_cast<uint16_t>(ctx_def.thread_id());

  return SUCCESS;
}

Status FftsPlusProtoTransfer::InitLabelCtx(const domi::FftsPlusCtxDef &task_def,
                                           rtFftsPlusComCtx_t *const com_ctx) const {
  rtFftsPlusLabelCtx_t *const ctx = PtrToPtr<rtFftsPlusComCtx_t, rtFftsPlusLabelCtx_t>(com_ctx);
  GE_CHECK_NOTNULL(ctx);
  const domi::FftsPlusLabelCtxDef &ctx_def = task_def.label_ctx();
  ctx->successorNum = static_cast<uint8_t>(ctx_def.successor_num());
  ctx->predCntInit = static_cast<uint8_t>(ctx_def.pred_cnt_init());
  ctx->predCnt = static_cast<uint8_t>(ctx_def.pred_cnt());

  if (ctx_def.successor_list_size() > RT_CTX_SUCCESSOR_NUM) {
    REPORT_INNER_ERROR("E19999", "Size of successor_list in FftsPlusLabelCtxDef should not > %d, but %d exactly",
                       RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    GELOGE(FAILED, "[Check][Param] Size of successor_list in FftsPlusLabelCtxDef should not > %d, but %d exactly",
           RT_CTX_SUCCESSOR_NUM, ctx_def.successor_list_size());
    return FAILED;
  }
  for (auto i = 0; i < ctx_def.successor_list_size(); i++) {
    ctx->successorList[i] = static_cast<uint16_t>(ctx_def.successor_list(i));
  }

  return SUCCESS;
}

void FftsPlusProtoTransfer::InitAdditionalData(const domi::FftsPlusTaskDef &task_def) {
  GELOGD("init additional data start, size:%d", task_def.additional_data_size());
  for (int32_t i = 0; i < task_def.additional_data_size(); ++i) {
    const domi::AdditionalDataDef &additional_data = task_def.additional_data(i);
    auto &additional_context = ctx_additional_data_[additional_data.data_type()];
    for (int32_t j = 0; j < additional_data.context_id_size(); ++j) {
      (void)additional_context.emplace(additional_data.context_id(j));
      GELOGD("additional data type:%u, context id:%u", additional_data.data_type(), additional_data.context_id(j));
    }
  }
}

template <typename T>
void FftsPlusProtoTransfer::InitThrdIoAddrs(const T &ctx_def, const uint16_t thread_id, const int32_t addr_count,
                                            const int32_t start_idx) const {
  for (int32_t i = 0; i < addr_count; ++i) {
    uintptr_t logic_addr = ctx_def.task_addr(start_idx + i) + (thread_id * ctx_def.task_addr_offset(i));
    GELOGD("task base addr is %lu, offset is %lu, thread id is %u, logic addr is 0x%lx",
           ctx_def.task_addr(i), ctx_def.task_addr_offset(i), static_cast<uint32_t>(thread_id), logic_addr);
    io_addrs_.emplace_back(logic_addr);
  }
}
}  // namespace ge
