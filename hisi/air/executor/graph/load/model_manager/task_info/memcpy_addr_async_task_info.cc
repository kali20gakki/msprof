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

#include "graph/load/model_manager/task_info/memcpy_addr_async_task_info.h"

#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"
#include "exec_runtime/execution_runtime.h"

namespace {
const uint32_t kAlignedBytes = 64U;
}

namespace ge {
Status MemcpyAddrAsyncTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("MemcpyAddrAsyncTaskInfo Init Start");
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  const auto &memcpy_async = task_def.memcpy_async();
  const OpDescPtr op_desc = davinci_model_->GetOpByIndex(memcpy_async.op_index());
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Can't get op_desc from davinci_model by index:%u", memcpy_async.op_index());
    GELOGE(INTERNAL_ERROR, "[Get][Op] Task op index:%u out of range", memcpy_async.op_index());
    return INTERNAL_ERROR;
  }

  op_index_ = memcpy_async.op_index();
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  auto ret = ModelUtils::GetRtAddress(rts_param, memcpy_async.src(), src_);
  if (ret != SUCCESS) {
    return ret;
  }

  ret = ModelUtils::GetRtAddress(rts_param, memcpy_async.dst(), dst_);
  if (ret != SUCCESS) {
    return ret;
  }

  // malloc args memory
  const std::vector<void *> io_addrs{src_, dst_};
  const size_t args_size = sizeof(void *) * io_addrs.size();
  const auto memory_type = rtGetTsMemType(MEM_REQUEST_FEATURE_DEFAULT, args_size + kAlignedBytes);
  GELOGI("memory_type: %u", memory_type);
  GE_CHK_RT_RET(rtMalloc(&args_, args_size + kAlignedBytes, memory_type));

  args_align_ = ValueToPtr(((PtrToValue(args_) / kAlignedBytes) + 1U) * kAlignedBytes);
  // copy orign src/dst
  GELOGI("src_args:%p, destMax:%zu, src_:%p, dst_args:%p, dst_:%p, count=%zu", args_align_, args_size, src_,
         PtrToPtr<void, uint8_t>(args_align_) + args_size, dst_, io_addrs.size());
  GE_CHK_RT_RET(rtMemcpy(args_align_, args_size, io_addrs.data(), args_size, RT_MEMCPY_HOST_TO_DEVICE));

  count_ = memcpy_async.count();
  kind_ = memcpy_async.kind();
  dst_max_ = memcpy_async.dst_max();
  GELOGI("InitMemcpyAddrAsyncTaskInfo, logic[0x%lx, 0x%lx], src:%p, dst:%p, max:%lu, count:%lu, args:%p, size:%zu",
         memcpy_async.src(), memcpy_async.dst(), src_, dst_, dst_max_, count_, args_align_, args_size);

  // aicpu scheduler cannot access TS memory, and cannot modify address
  bool loading_by_helper_executor = !ExecutionRuntime::IsHeterogeneousEnabled();
  if (loading_by_helper_executor && ((memory_type & RT_MEMORY_TS) != 0U)) {
    davinci_model_->DisableZeroCopy(src_);
    davinci_model_->DisableZeroCopy(dst_);
  } else {
    davinci_model_->SetZeroCopyAddr(op_desc, io_addrs, io_addrs.data(), PtrToValue(args_align_), args_size, 0U);
  }
  return SUCCESS;
}

Status MemcpyAddrAsyncTaskInfo::Distribute() {
  GELOGI("MemcpyAddrAsyncTaskInfo Distribute Start, dst_max:%lu, count:%lu, kind:%u", dst_max_, count_, kind_);

  SetTaskTag(davinci_model_->GetOpByIndex(op_index_)->GetName().c_str());
  const rtError_t rt_ret = rtMemcpyAsync(ValueToPtr((PtrToValue(args_align_)) + sizeof(void *)),
                                         dst_max_, args_align_, count_, static_cast<rtMemcpyKind_t>(kind_), stream_);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpyAsync failed, size:%lu, ret:0x%X", dst_max_, rt_ret);
    GELOGE(RT_FAILED, "[Call][RtMemcpyAsync] failed, size:%lu, ret:0x%X", dst_max_, rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_MEMCPY_ADDR_ASYNC, MemcpyAddrAsyncTaskInfo);
}  // namespace ge
