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

#include "graph/load/model_manager/task_info/ffts_task_info.h"

#include "common/math_util.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"

namespace {
constexpr uint64_t kAddrLen = sizeof(void *);
}
namespace ge {
FftsTaskInfo::~FftsTaskInfo() {
  GE_FREE_RT_LOG(args_);
}

Status FftsTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("FftsTaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  const domi::FftsTaskDef &ffts_task_def = task_def.ffts_task();
  const OpDescPtr op_desc = davinci_model_->GetOpByIndex(ffts_task_def.op_index());
  GE_CHECK_NOTNULL(op_desc);

  if ((ffts_task_def.sub_task_size() > static_cast<int32_t>(RT_FFTS_MAX_SUB_TASK_NUM)) ||
      (ffts_task_def.ticket_cache_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_NUM))) {
    GELOGE(INTERNAL_ERROR, "[Check][Param] failed. Node: %s, sub task desc size: %d, ticket cache size: %d",
           op_desc->GetName().c_str(), ffts_task_def.sub_task_size(), ffts_task_def.ticket_cache_size());
    return INTERNAL_ERROR;
  }
  args_size_ = kAddrLen * static_cast<uint64_t>(ffts_task_def.addr_size());
  GE_CHK_RT_RET(rtMalloc(&args_, args_size_, RT_MEMORY_HBM));
  (void)InitFftsDescInfo(ffts_task_def.ffts_desc(), sub_task_info_.fftsDesc);

  sub_task_info_.fftsType = static_cast<rtFftsType_t>(ffts_task_def.ffts_type());
  REQUIRE_COMPAT_UINT16(ffts_task_def.sub_task_size());
  sub_task_info_.subTaskNum = static_cast<uint16_t>(ffts_task_def.sub_task_size());
  for (int32_t idx = 0; idx < ffts_task_def.sub_task_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitSubTaskInfo(ffts_task_def.sub_task(idx), sub_task_info_.subTask[idx]));
  }
  REQUIRE_COMPAT_UINT16(ffts_task_def.ticket_cache_size());
  sub_task_info_.tickCacheNum = static_cast<uint16_t>(ffts_task_def.ticket_cache_size());
  for (int32_t idx = 0; idx < ffts_task_def.ticket_cache_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitTicketCache(ffts_task_def.ticket_cache(idx), sub_task_info_.ticketCache[idx]));
  }

  const auto data_size = kAddrLen * io_addrs_.size();
  GE_CHK_RT_RET(rtMemcpy(args_, args_size_, io_addrs_.data(), data_size, RT_MEMCPY_HOST_TO_DEVICE));
  GELOGI("FftsTaskInfo::Init Success. Node: %s, input/output size: %zu", op_desc->GetName().c_str(), io_addrs_.size());
  return SUCCESS;
}

Status FftsTaskInfo::InitFftsDescInfo(const domi::FftsDescInfoDef &ffts_desc_def, rtFftsDescInfo_t &ffts_desc) const {
  REQUIRE_COMPAT_UINT8(ffts_desc_def.tm());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.di());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.dw());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.df());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.data_split_unit());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.prefetch_ost_num());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.cache_maintain_ost_num());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.aic_prefetch_upper());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.aic_prefetch_lower());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.aiv_prefetch_upper());
  REQUIRE_COMPAT_UINT8(ffts_desc_def.aiv_prefetch_lower());
  ffts_desc.tm = static_cast<uint8_t>(ffts_desc_def.tm());
  ffts_desc.di = static_cast<uint8_t>(ffts_desc_def.di());
  ffts_desc.dw = static_cast<uint8_t>(ffts_desc_def.dw());
  ffts_desc.df = static_cast<uint8_t>(ffts_desc_def.df());
  ffts_desc.dataSplitUnit = static_cast<uint8_t>(ffts_desc_def.data_split_unit());
  ffts_desc.prefetchOstNum = static_cast<uint8_t>(ffts_desc_def.prefetch_ost_num());
  ffts_desc.cacheMaintainOstNum = static_cast<uint8_t>(ffts_desc_def.cache_maintain_ost_num());
  ffts_desc.aicPrefetchUpper = static_cast<uint8_t>(ffts_desc_def.aic_prefetch_upper());
  ffts_desc.aicPrefetchLower = static_cast<uint8_t>(ffts_desc_def.aic_prefetch_lower());
  ffts_desc.aivPrefetchUpper = static_cast<uint8_t>(ffts_desc_def.aiv_prefetch_upper());
  ffts_desc.aivPrefetchLower = static_cast<uint8_t>(ffts_desc_def.aiv_prefetch_lower());
  return SUCCESS;
}

Status FftsTaskInfo::InitSubTaskInfo(const domi::FftsSubTaskDef &sub_task_def, rtFftsSubTaskInfo_t &sub_task_desc) {
  if ((sub_task_def.dst_tick_cache_id_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK)) ||
      (sub_task_def.src_tick_cache_id_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK))) {
    GELOGE(FAILED, "[Check][Param] Invalid FftsSubTaskInfo, dst tick cache id size: %d, src tick cache id size: %d",
           sub_task_def.dst_tick_cache_id_size(), sub_task_def.src_tick_cache_id_size());
    return FAILED;
  }

  if (sub_task_def.has_auto_thread_aic_aiv() == sub_task_def.has_manual_thread_aic_aiv()) {
    GELOGE(FAILED, "[Check][Param] Invalid FftsSubTaskInfo, auto thread aic/aiv: %d, manual thread aic/aiv: %d",
           static_cast<int32_t>(sub_task_def.has_auto_thread_aic_aiv()),
           static_cast<int32_t>(sub_task_def.has_manual_thread_aic_aiv()));
    return FAILED;
  }

  thread_dim_ = sub_task_def.thread_dim();
  GE_CHK_BOOL_RET_STATUS(thread_dim_ != 0U, FAILED, "[Get][thread_dim] failed, Invalid thread dim: %u!", thread_dim_);

  sub_task_desc.subTaskType = static_cast<rtFftsSubTaskType_t>(sub_task_def.sub_task_type());
  REQUIRE_COMPAT_UINT16(sub_task_def.thread_dim());
  sub_task_desc.threadDim = static_cast<uint16_t>(sub_task_def.thread_dim());

  REQUIRE_COMPAT_UINT8(sub_task_def.dst_tick_cache_vld_bitmap());
  REQUIRE_COMPAT_UINT8(sub_task_def.src_tick_cache_vld_bitmap());
  REQUIRE_COMPAT_UINT8(sub_task_def.src_data_out_of_subgraph_bitmap());
  sub_task_desc.dstTickCacheVldBitmap = static_cast<uint8_t>(sub_task_def.dst_tick_cache_vld_bitmap());
  sub_task_desc.srcTickCacheVldBitmap = static_cast<uint8_t>(sub_task_def.src_tick_cache_vld_bitmap());
  sub_task_desc.srcDataOutOfSubGraphBitmap = static_cast<uint8_t>(sub_task_def.src_data_out_of_subgraph_bitmap());

  for (int32_t idx = 0; idx < sub_task_def.dst_tick_cache_id_size(); ++idx) {
    REQUIRE_COMPAT_UINT8(sub_task_def.dst_tick_cache_id(idx));
    sub_task_desc.dstTickCacheID[idx] = static_cast<uint8_t>(sub_task_def.dst_tick_cache_id(idx));
  }

  for (int32_t idx = 0; idx < sub_task_def.src_tick_cache_id_size(); ++idx) {
    REQUIRE_COMPAT_UINT8(sub_task_def.src_tick_cache_id(idx));
    sub_task_desc.srcTickCacheID[idx] = static_cast<uint8_t>(sub_task_def.src_tick_cache_id(idx));
  }

  if (sub_task_def.has_auto_thread_aic_aiv()) {
    GE_CHK_STATUS_RET_NOLOG(InitAutoAicAiv(sub_task_def.auto_thread_aic_aiv(), sub_task_desc.custom.autoThreadAicAiv));
  }

  if (sub_task_def.has_manual_thread_aic_aiv()) {
    GE_CHK_STATUS_RET_NOLOG(
        InitManualAicAiv(sub_task_def.manual_thread_aic_aiv(), sub_task_desc.custom.manualThreadAicAiv));
  }

  if (sub_task_def.has_manual_thread_nop()) {
    GE_CHK_STATUS_RET_NOLOG(InitManualNop(sub_task_def.manual_thread_nop(), sub_task_desc.custom.manualThreadNop));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitTicketCache(const domi::TicketCacheDef &ticket_cache_def,
                                     rtTicketCache_t &ticket_cache) const {
  if (ticket_cache_def.has_auto_thread_cache() == ticket_cache_def.has_manual_thread_cache()) {
    GELOGE(FAILED, "[Check][Param] Invalid TicketCacheDef, has auto thread cache: %d, has manual thread cache: %d",
           static_cast<int32_t>(ticket_cache_def.has_auto_thread_cache()),
           static_cast<int32_t>(ticket_cache_def.has_manual_thread_cache()));
    return FAILED;
  }

  ticket_cache.cacheOption = static_cast<rtCacheOp_t>(ticket_cache_def.cache_option());
  REQUIRE_COMPAT_UINT8(ticket_cache_def.ticket_cache_window());
  ticket_cache.ticketCacheWindow = static_cast<uint8_t>(ticket_cache_def.ticket_cache_window());

  if (ticket_cache_def.has_auto_thread_cache()) {
    GE_CHK_STATUS_RET_NOLOG(InitAutoCacheInfo(ticket_cache_def.auto_thread_cache(),
                                              ticket_cache.custom.autoThreadCache));
  }
  if (ticket_cache_def.has_manual_thread_cache()) {
    GE_CHK_STATUS_RET_NOLOG(
        InitManualCacheInfo(ticket_cache_def.manual_thread_cache(), ticket_cache.custom.manualThreadCache));
  }

  return SUCCESS;
}

// task_addr = {0,200,700,1000,2000, 3500}
// task_addr_offset = {20,40,2,100,200}
template <typename T>
Status FftsTaskInfo::InitIoAddrs(const RuntimeParam &rts_param, const T &aic_aiv_def, const uint32_t thread_dim,
                                 const uint32_t addr_count) {
  for (uint32_t i = 0U; i < addr_count; ++i) {
    const uintptr_t logic_addr = aic_aiv_def.task_addr(i)  + (thread_dim * aic_aiv_def.task_addr_offset(i));
    uint8_t *io_addr = nullptr;
    if (ModelUtils::GetRtAddress(rts_param, logic_addr, io_addr) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress]GetRtAddress failed.");
      return INTERNAL_ERROR;
    }
    GELOGD("aic_aiv_def task base addr is %ld, offset is %ld, thread is %u, logic addrs is 0x%lx, io addr is %p",
           aic_aiv_def.task_addr(i), aic_aiv_def.task_addr_offset(i), thread_dim, logic_addr, io_addr);
    io_addrs_.emplace_back(io_addr);
  }
  return SUCCESS;
}

Status FftsTaskInfo::InitAutoAicAiv(const domi::AutoThreadAicAivDef &aic_aiv_def, rtAutoThreadAicAivInfo_t &aic_aiv) {
  if (aic_aiv_def.src_prefetch_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK)) {
    GELOGE(FAILED, "[Check][Param] Invalid AutoThreadAicAivInfo, prefetch size: %d", aic_aiv_def.src_prefetch_size());
    return FAILED;
  }

  aic_aiv.taskParamAddr = PtrToValue(args_) + (kAddrLen * io_addrs_.size());
  GELOGD("AutoThreadAicAivDef: task param addr is %lu.", aic_aiv.taskParamAddr);
  const auto &rts_param = davinci_model_->GetRuntimeParam();
  for (uint32_t i = 0U; i < (thread_dim_ - 1U); ++i) {
    GE_CHK_STATUS_RET_NOLOG(InitIoAddrs(rts_param, aic_aiv_def, i,
                                        static_cast<uint32_t>(aic_aiv_def.task_addr_offset_size())));
  }
  GE_CHK_STATUS_RET_NOLOG(InitIoAddrs(rts_param, aic_aiv_def, thread_dim_ - 1U, aic_aiv_def.input_output_count()));
  const int32_t last_thread_workspace_size = aic_aiv_def.task_addr_size() - aic_aiv_def.task_addr_offset_size();
  for (int32_t i = 0; i < last_thread_workspace_size; ++i) {
    const uintptr_t logic_addr =
      static_cast<uintptr_t>(aic_aiv_def.task_addr(aic_aiv_def.task_addr_offset_size() + i));
    uint8_t *io_addr = nullptr;
    GE_CHK_STATUS_RET_NOLOG(ModelUtils::GetRtAddress(rts_param, logic_addr, io_addr));
    GELOGD("logic addr is 0x%lx, io addr is %p.", logic_addr, io_addr);
    io_addrs_.emplace_back(io_addr);
  }

  REQUIRE_COMPAT_UINT16(aic_aiv_def.task_param_offset());
  aic_aiv.taskParamOffset = static_cast<uint16_t>(aic_aiv_def.task_param_offset());
  GELOGD("args_: %p, io_addrs size: %zu, task param offset: %u.",
         args_, io_addrs_.size(), static_cast<uint32_t>(aic_aiv.taskParamOffset));

  REQUIRE_COMPAT_UINT8(aic_aiv_def.sat_mode());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.schedule_mode());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.cache_prefetch_cnt());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.prefetch_enable_bitmap());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.prefetch_once_bitmap());

  aic_aiv.satMode = static_cast<uint8_t>(aic_aiv_def.sat_mode());
  aic_aiv.scheduleMode = static_cast<uint8_t>(aic_aiv_def.schedule_mode());
  aic_aiv.iCachePrefetchCnt = static_cast<uint8_t>(aic_aiv_def.cache_prefetch_cnt());
  aic_aiv.prefetchEnableBitmap = static_cast<uint8_t>(aic_aiv_def.prefetch_enable_bitmap());
  aic_aiv.prefetchOnceBitmap = static_cast<uint8_t>(aic_aiv_def.prefetch_once_bitmap());

  REQUIRE_COMPAT_UINT16(aic_aiv_def.tail_blk_dim());
  REQUIRE_COMPAT_UINT16(aic_aiv_def.non_tail_blk_dim());

  aic_aiv.tailBlkDim = static_cast<uint16_t>(aic_aiv_def.tail_blk_dim());
  aic_aiv.nonTailBlkDim = static_cast<uint16_t>(aic_aiv_def.non_tail_blk_dim());

  aic_aiv.nonTailTaskFuncStub = davinci_model_->GetRegisterStub(aic_aiv_def.non_tail_task_func_stub(), "");
  aic_aiv.tailTaskFuncStub = davinci_model_->GetRegisterStub(aic_aiv_def.tail_task_func_stub(), "");

  GELOGI("Set func name[%s][%s] succ.", aic_aiv.nonTailTaskFuncStub, aic_aiv.tailTaskFuncStub);
  for (int32_t idx = 0; idx < aic_aiv_def.src_prefetch_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitAutoPrefetch(aic_aiv_def.src_prefetch(idx), aic_aiv.srcPrefetch[idx]));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitAutoCacheInfo(const domi::AutoThreadCacheDef &cache_def,
                                       rtAutoThreadCacheInfo_t &cache) const {
  const auto &rts_param = davinci_model_->GetRuntimeParam();
  const uintptr_t logic_addr = static_cast<uintptr_t>(cache_def.data_addr());
  uint8_t *data_addr = nullptr;
  GE_CHK_STATUS_RET(ModelUtils::GetRtAddress(rts_param, logic_addr, data_addr),
                    "[Check][GetRtAddress]GetRtAddress failed.");
  cache.dataAddr = PtrToValue(data_addr);
  cache.dataAddrOffset = cache_def.data_addr_offset();
  cache.nonTailDataLen = cache_def.non_tail_data_len();
  cache.tailDataLen = cache_def.tail_data_len();
  REQUIRE_COMPAT_UINT16(cache_def.ticket_cache_ref_cnt());
  cache.ticketCacheRefCnt = static_cast<uint16_t>(cache_def.ticket_cache_ref_cnt());
  return SUCCESS;
}

Status FftsTaskInfo::InitAutoPrefetch(const domi::AutoThreadPrefetchDef &prefetch_def,
                                      rtAutoThreadPrefetch_t &prefetch) const {
  const auto &rts_param = davinci_model_->GetRuntimeParam();
  const uintptr_t logic_addr = static_cast<uintptr_t>(prefetch_def.data_addr());
  uint8_t *data_addr = nullptr;
  GE_CHK_STATUS_RET(ModelUtils::GetRtAddress(rts_param, logic_addr, data_addr),
                    "[Check][GetRtAddress]GetRtAddress failed.");
  prefetch.dataAddr = PtrToValue(data_addr);
  prefetch.dataAddrOffset = prefetch_def.data_addr_offset();
  prefetch.nonTailDataLen = prefetch_def.non_tail_data_len();
  prefetch.tailDataLen = prefetch_def.tail_data_len();
  return SUCCESS;
}

Status FftsTaskInfo::InitManualAicAivCheck(const domi::ManualThreadAicAivDef &aic_aiv_def) const {
  if ((aic_aiv_def.thread_prefetch_dmu_idx_size() > static_cast<int32_t>(RT_FFTS_MAX_MANUAL_THREAD_NUM)) ||
      (aic_aiv_def.thread_blk_dim_size() > static_cast<int32_t>(RT_FFTS_MAX_MANUAL_THREAD_NUM)) ||
      (aic_aiv_def.thread_task_func_stub_size() > static_cast<int32_t>(RT_FFTS_MAX_MANUAL_THREAD_NUM)) ||
      (aic_aiv_def.src_dep_tbl_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK))) {
    GELOGE(FAILED, "[Check][Param] Invalid ManualThreadAicAivInfo, thread prefetch dmu desc size: %d, "
                   "thread blk dim size: %d, thread task func stub size: %d, src dep tbl size: %d",
           aic_aiv_def.thread_prefetch_dmu_idx_size(), aic_aiv_def.thread_blk_dim_size(),
           aic_aiv_def.thread_task_func_stub_size(), aic_aiv_def.src_dep_tbl_size());
    return FAILED;
  }
  return SUCCESS;
}

Status FftsTaskInfo::InitManualAicAiv(const domi::ManualThreadAicAivDef &aic_aiv_def,
                                      rtManualThreadAicAivInfo_t &aic_aiv) {
  GE_CHK_STATUS_RET_NOLOG(InitManualAicAivCheck(aic_aiv_def));
  aic_aiv.taskParamAddr = PtrToValue(args_) + (kAddrLen * io_addrs_.size());
  GELOGD("ManualThreadAicAivDef: task param addr is %lu.", aic_aiv.taskParamAddr);
  const auto &rts_param = davinci_model_->GetRuntimeParam();
  GE_CHK_STATUS_RET_NOLOG(InitIoAddrs(rts_param, aic_aiv_def, thread_dim_ - 1U, aic_aiv_def.input_output_count()));
  for (uint32_t i = 0U; i < (thread_dim_ - 1U); ++i) {
    GE_CHK_STATUS_RET_NOLOG(InitIoAddrs(rts_param, aic_aiv_def, i,
                                        static_cast<uint32_t>(aic_aiv_def.task_addr_offset_size())));
  }
  const int32_t last_thread_workspace_size = aic_aiv_def.task_addr_size() - aic_aiv_def.task_addr_offset_size();
  for (int32_t i = 0; i < last_thread_workspace_size; ++i) {
    const uintptr_t logic_addr =
      static_cast<uintptr_t>(aic_aiv_def.task_addr(aic_aiv_def.task_addr_offset_size() + i));
    uint8_t *io_addr = nullptr;
    GE_CHK_STATUS_RET_NOLOG(ModelUtils::GetRtAddress(rts_param, logic_addr, io_addr));
    io_addrs_.emplace_back(io_addr);
  }

  REQUIRE_COMPAT_UINT16(aic_aiv_def.task_param_offset());
  aic_aiv.taskParamOffset = static_cast<uint16_t>(aic_aiv_def.task_param_offset());

  REQUIRE_COMPAT_UINT8(aic_aiv_def.sat_mode());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.schedule_mode());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.cache_prefetch_cnt());
  aic_aiv.satMode = static_cast<uint8_t>(aic_aiv_def.sat_mode());
  aic_aiv.scheduleMode = static_cast<uint8_t>(aic_aiv_def.schedule_mode());
  aic_aiv.iCachePrefetchCnt = static_cast<uint8_t>(aic_aiv_def.cache_prefetch_cnt());

  REQUIRE_COMPAT_UINT8(aic_aiv_def.prefetch_enable_bitmap());
  REQUIRE_COMPAT_UINT8(aic_aiv_def.prefetch_once_bitmap());
  REQUIRE_COMPAT_UINT16(aic_aiv_def.prefetch_once_dmu_num());
  aic_aiv.prefetchEnableBitmap = static_cast<uint8_t>(aic_aiv_def.prefetch_enable_bitmap());    // 8 bit bitmap 1 0 1 0
  aic_aiv.prefetchOnceBitmap = static_cast<uint8_t>(aic_aiv_def.prefetch_once_bitmap());   // 8 bit bitmap 1 0 1 0
  aic_aiv.prefetchOnceDmuNum = static_cast<uint16_t>(aic_aiv_def.prefetch_once_dmu_num());

  for (int32_t idx = 0; idx < aic_aiv_def.thread_prefetch_dmu_idx_size(); ++idx) {
    REQUIRE_COMPAT_UINT16(aic_aiv_def.prefetch_once_dmu_num());
    aic_aiv.threadPrefetchDmuIdx[idx] = static_cast<uint16_t>(aic_aiv_def.thread_prefetch_dmu_idx(idx));
  }
  for (int32_t idx = 0; idx < aic_aiv_def.thread_blk_dim_size(); ++idx) {
    REQUIRE_COMPAT_UINT16(aic_aiv_def.thread_blk_dim(idx));
    aic_aiv.threadBlkDim[idx] = static_cast<uint16_t>(aic_aiv_def.thread_blk_dim(idx));
  }
  for (int32_t idx = 0; idx < aic_aiv_def.thread_task_func_stub_size(); ++idx) {
    aic_aiv.threadTaskFuncStub[idx] = aic_aiv_def.thread_task_func_stub(idx).c_str();
  }

  GE_CHK_STATUS_RET_NOLOG(InitManualDmuInfo(aic_aiv_def, aic_aiv.prefetchList));
  for (int32_t idx = 0; idx < aic_aiv_def.src_dep_tbl_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitManualDependency(aic_aiv_def.src_dep_tbl(idx), aic_aiv.srcDepTbl[idx]));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitManualCacheInfo(const domi::ManualThreadCacheDef &cache_def,
                                         rtManualThreadCacheInfo_t &cache_info) const {
  if ((cache_def.slice_dmu_idx_size() > static_cast<int32_t>(RT_FFTS_MAX_MANUAL_THREAD_NUM)) ||
      (cache_def.ticket_cache_ref_cnt_tbl_size() > static_cast<int32_t>(RT_FFTS_MAX_MANUAL_THREAD_NUM))) {
    GELOGE(FAILED, "[Check][Param] Invalid ManualThreadCacheInfo slice dum desc index %d, ticket cache ref cnt %d",
           cache_def.slice_dmu_idx_size(), cache_def.ticket_cache_ref_cnt_tbl_size());
    return FAILED;
  }

  GE_CHK_STATUS_RET_NOLOG(InitManualDmuInfo(cache_def, cache_info.dmuList));
  for (int32_t idx = 0; idx < cache_def.slice_dmu_idx_size(); ++idx) {
    REQUIRE_COMPAT_UINT16(cache_def.slice_dmu_idx(idx));
    cache_info.sliceDmuIdx[idx] = static_cast<uint16_t>(cache_def.slice_dmu_idx(idx));
  }

  for (int32_t idx = 0; idx < cache_def.ticket_cache_ref_cnt_tbl_size(); ++idx) {
    REQUIRE_COMPAT_UINT16(cache_def.ticket_cache_ref_cnt_tbl(idx));
    cache_info.ticketCacheRefCntTbl[idx] = static_cast<uint16_t>(cache_def.ticket_cache_ref_cnt_tbl(idx));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitManualDependency(const domi::ManualThreadDependencyDef &dependency_def,
                                          rtManualThreadDependency_t &dependency) const {
  if (dependency_def.dependency_size() > static_cast<int32_t>(RT_FFTS_MANUAL_SRC_DEPEND_TBL_LEN)) {
    GELOGE(FAILED, "[Check][Param] Invalid ManualThreadDependency size: %d", dependency_def.dependency_size());
    return FAILED;
  }

  for (int32_t idx = 0; idx < dependency_def.dependency_size(); ++idx) {
    REQUIRE_COMPAT_UINT8(dependency_def.dependency(idx));
    dependency.dependency[idx] = static_cast<uint8_t>(dependency_def.dependency(idx));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitManualNop(const domi::ManualThreadNopDef &nop_def, rtManualThreadNopInfo_t &nop_info) const {
  if (nop_def.src_dep_tbl_size() > static_cast<int32_t>(RT_FFTS_MAX_TICKET_CACHE_PER_SUBTASK)) {
    GELOGE(FAILED, "[Check][Param] Invalid ManualThreadNopInfo, src dep tbl size: %d", nop_def.src_dep_tbl_size());
    return FAILED;
  }

  for (int32_t idx = 0; idx < nop_def.src_dep_tbl_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitManualDependency(nop_def.src_dep_tbl(idx), nop_info.srcDepTbl[idx]));
  }

  return SUCCESS;
}

Status FftsTaskInfo::InitManualDmuInfo(const domi::ManualThreadAicAivDef &aic_aiv_def,
                                       rtManualThreadDmuInfo_t *&dmu) const {
  if (aic_aiv_def.prefetch_list().empty()) {
    return SUCCESS;
  }

  std::vector<uint8_t> buffer(sizeof(rtManualThreadDmuInfo_t) *
                              static_cast<uint64_t>(aic_aiv_def.prefetch_list_size()));
  dmu = PtrToPtr<uint8_t, rtManualThreadDmuInfo_t>(buffer.data());
  for (int32_t idx = 0; idx < aic_aiv_def.prefetch_list_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitManualDmuInfo(aic_aiv_def.prefetch_list(idx), dmu[idx]));
  }
  return SUCCESS;
}

Status FftsTaskInfo::InitManualDmuInfo(const domi::ManualThreadCacheDef &cache_def,
                                       rtManualThreadDmuInfo_t *&dmu) const {
  if (cache_def.dmu_list().empty()) {
    return SUCCESS;
  }

  std::vector<uint8_t> buffer(sizeof(rtManualThreadDmuInfo_t) * static_cast<uint64_t>(cache_def.dmu_list_size()));
  dmu = PtrToPtr<uint8_t, rtManualThreadDmuInfo_t>(buffer.data());
  for (int32_t idx = 0; idx < cache_def.dmu_list_size(); ++idx) {
    GE_CHK_STATUS_RET_NOLOG(InitManualDmuInfo(cache_def.dmu_list(idx), dmu[idx]));
  }
  return SUCCESS;
}

Status FftsTaskInfo::InitManualDmuInfo(const domi::ManualThreadDmuDef &dmu_def, rtManualThreadDmuInfo_t &dmu) const {
  REQUIRE_COMPAT_UINT16(dmu_def.num_outer());
  REQUIRE_COMPAT_UINT16(dmu_def.num_inner());
  dmu.dataAddr = dmu_def.data_addr();
  dmu.numOuter = static_cast<uint16_t>(dmu_def.num_outer());
  dmu.numInner = static_cast<uint16_t>(dmu_def.num_inner());
  dmu.strideOuter = dmu_def.stride_outer();
  dmu.lenInner = dmu_def.len_inner();
  dmu.strideInner = dmu_def.stride_inner();
  return SUCCESS;
}

Status FftsTaskInfo::CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  (void)task_def;
  (void)davinci_model;
  return SUCCESS;
}

Status FftsTaskInfo::UpdateArgs() {
  GE_CHECK_NOTNULL(davinci_model_);
  std::vector<void *> io_addrs = io_addrs_;
  GE_CHK_STATUS_RET_NOLOG(davinci_model_->UpdateRunAddr(io_addrs));
  const auto addr_size = kAddrLen * io_addrs.size();
  GE_CHK_RT_RET(rtMemcpy(args_, args_size_, io_addrs.data(), addr_size, RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status FftsTaskInfo::Distribute() {
  GELOGI("FftsTaskInfo Distribute Start.");
  const rtError_t rt_ret = rtFftsTaskLaunch(&sub_task_info_, stream_);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Check][RT_ret] Call rtFftsTaskLaunch failed, ret: 0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  GELOGI("FftsTaskInfo Distribute Success.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_FFTS_TASK, FftsTaskInfo);
}  // namespace ge
