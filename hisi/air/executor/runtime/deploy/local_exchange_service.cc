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
#include "runtime/deploy/local_exchange_service.h"
#include "common/debug/log.h"
#include "common/ge_inner_error_codes.h"
#include "runtime/rt_mem_queue.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/mem_utils.h"

namespace ge {
namespace {
constexpr int32_t kQueueOpTimeout = 10 * 60 * 1000;  // 10 minutes
constexpr uint32_t kBufferCount = 1U;
constexpr size_t kContextLen = 0U;
constexpr size_t kAlignmentVal64 = 64U;
}
Status LocalExchangeService::CreateQueue(const int32_t device_id,
                                         const std::string &name,
                                         const uint32_t depth,
                                         const uint32_t work_mode,
                                         uint32_t &queue_id) {
  GELOGD("[CreateQueue] start, device id = %d, queue name = %s, depth = %u, work_mode = %u",
         device_id,
         name.c_str(),
         depth,
         work_mode);
  if (name.size() >= static_cast<size_t>(RT_MQ_MAX_NAME_LEN - 1)) {
    GELOGE(PARAM_INVALID,
           "[CreateQueue] [CheckParam] Length of queue name out of range, name = %s, length = %zu",
           name.c_str(),
           name.size());
    return PARAM_INVALID;
  }
  GE_CHK_STATUS_RET(EnsureInitialized(device_id), "[CreateQueue] [Init] failed, queue name = %s", name.c_str());
  rtMemQueueAttr_t queue_attr;
  queue_attr.depth = depth;
  queue_attr.workMode = work_mode;
  queue_attr.flowCtrlFlag = false;
  queue_attr.flowCtrlDropTime = static_cast<uint32_t>(kQueueOpTimeout);
  queue_attr.overWriteFlag = false;
  // actually this won't fail, length was checked
  if (strcpy_s(queue_attr.name, sizeof(queue_attr.name), name.c_str()) != EOK) {
    GELOGE(FAILED, "[CreateQueue] [CopyName] Failed");
    return FAILED;
  }

  const auto ret = rtMemQueueCreate(device_id, &queue_attr, &queue_id);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueueCreate fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[CreateQueue] failed, rt_err = %d, device id = %d, queue name = %s, depth = %u",
           ret,
           device_id,
           name.c_str(),
           depth);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[CreateQueue] ended successfully, device id = %d, queue name = %s, depth = %u, queue_id = %u",
         device_id,
         name.c_str(),
         depth,
         queue_id);
  return SUCCESS;
}

Status LocalExchangeService::DestroyQueue(const int32_t device_id, const uint32_t queue_id) {
  GELOGD("[DestroyQueue] start, device id = %d, queue_id = %u", device_id, queue_id);
  const auto ret = rtMemQueueDestroy(device_id, queue_id);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueueDestroy fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[DestroyQueue] failed, rt_err = %d, device id = %d, queue id = %u",
           ret,
           device_id,
           queue_id);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[DestroyQueue] ended successfully, device id = %d, queue_id = %u", device_id, queue_id);
  return SUCCESS;
}

Status LocalExchangeService::Enqueue(const int32_t device_id, const uint32_t queue_id,
                                     const void *const data, const size_t size) {
  GELOGD("[Enqueue] start, device id = %d, queue_id = %u, size = %zu", device_id, queue_id, size);
  rtMemQueueBuffInfo buffer_info;
  buffer_info.addr = const_cast<void *>(data);
  buffer_info.len = size;
  rtMemQueueBuff_t buffer;
  buffer.contextAddr = nullptr;
  buffer.contextLen = kContextLen;
  buffer.buffCount = kBufferCount;
  buffer.buffInfo = &buffer_info;
  const auto ret = rtMemQueueEnQueueBuff(device_id, queue_id, &buffer, kQueueOpTimeout);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueueEnQueueBuff fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED,
           "[Enqueue] [EnqueueBuffer] failed, device id = %d, queue_id = %u, size = %zu",
           device_id,
           queue_id,
           size);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[Enqueue] ended successfully, device id = %d, queue_id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status LocalExchangeService::Enqueue(const int32_t device_id,
                                     const uint32_t queue_id,
                                     const size_t size,
                                     const ExchangeService::FillFunc &fill_func) {
  std::vector<uint8_t> buffer(size);
  GE_CHK_STATUS_RET(fill_func(buffer.data(), size),
                    "[Enqueue] failed to fill buffer, device id = %d, queue_id = %u, size = %zu",
                    device_id,
                    queue_id,
                    size);
  return Enqueue(device_id, queue_id, buffer.data(), buffer.size());
}

Status LocalExchangeService::Peek(const int32_t device_id, const uint32_t queue_id, size_t &size) {
  GELOGD("[Peek] start, device id = %d, queue id = %u", device_id, queue_id);
  const auto ret = rtMemQueuePeek(device_id, queue_id, &size, kQueueOpTimeout);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueuePeek fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[Peek] failed, rt_err = %d, device id = %d, queue id = %u", ret, device_id, queue_id);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[Peek] ended successfully, device id = %d, queue id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status LocalExchangeService::DequeueBuf(const int32_t device_id, const uint32_t queue_id, void *const data,
                                        const size_t size) const {
  GELOGD("[Dequeue] start, device id = %d, queue id = %u, size = %zu", device_id, queue_id, size);
  rtMemQueueBuffInfo buffer_info;
  buffer_info.addr = data;
  buffer_info.len = size;
  rtMemQueueBuff_t buffer;
  buffer.contextAddr = nullptr;
  buffer.contextLen = kContextLen;
  buffer.buffCount = kBufferCount;
  buffer.buffInfo = &buffer_info;
  const auto ret = rtMemQueueDeQueueBuff(device_id, queue_id, &buffer, kQueueOpTimeout);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueueDeQueueBuff fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[Dequeue] [DequeueBuffer] Failed, ret = %d, device id = %d, queue id = %u, size = %zu",
           ret,
           device_id,
           queue_id,
           size);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[Dequeue] ended successfully, device id = %d, queue id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status LocalExchangeService::Dequeue(const int32_t device_id, const uint32_t queue_id, void *const data,
                                     const size_t size, ControlInfo &control_Info) {
  GELOGD("[Dequeue] start, device id = %d, queue id = %u, size = %zu", device_id, queue_id, size);
  control_Info.end_of_sequence_flag = false;
  // 1. get dequeue size
  size_t dequeue_size = 0U;
  GE_CHK_STATUS_RET(Peek(device_id, queue_id, dequeue_size), "[Dequeue] [Peek] Failed");
  if (dequeue_size > size) {
    GELOGE(INTERNAL_ERROR,
           "[Dequeue] [ValidateSize] Failed to validate size, expected size = %zu, but got %zu, queue id = %u", size,
           dequeue_size, queue_id);
    return INTERNAL_ERROR;
  }
  GE_CHK_STATUS_RET_NOLOG(DequeueBuf(device_id, queue_id, data, dequeue_size));
  GELOGD("[Dequeue] ended successfully, device id = %d, queue id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status LocalExchangeService::DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                                           ControlInfo &control_Info) {
  GELOGD("[DequeueTensor] start, device id = %d, queue id = %u", device_id, queue_id);
  control_Info.end_of_sequence_flag = false;
  // 1. get dequeue size
  size_t dequeue_size = 0U;
  GE_CHK_STATUS_RET(Peek(device_id, queue_id, dequeue_size), "[Dequeue] [Peek] Failed");
  GELOGD("MakeShared start size=%zu", dequeue_size);
  const auto buff_aligned_ptr = MakeShared<AlignedPtr>(dequeue_size, kAlignmentVal64);
  GELOGD("MakeShared end size=%zu", dequeue_size);
  GE_CHECK_NOTNULL(buff_aligned_ptr);
  GE_CHK_STATUS_RET_NOLOG(DequeueBuf(device_id, queue_id, buff_aligned_ptr->MutableGet(), dequeue_size));
  auto &output_tensor_desc = tensor.MutableTensorDesc();
  RuntimeTensorDesc *const mbuf_tensor_desc = PtrToPtr<uint8_t, RuntimeTensorDesc>(buff_aligned_ptr->MutableGet());
  std::vector<int64_t> output_shape;
  for (size_t j = 1U; j <= static_cast<size_t>(mbuf_tensor_desc->shape[0U]); ++j) {
    output_shape.push_back(mbuf_tensor_desc->shape[j]);
  }
  output_tensor_desc.SetShape(GeShape(output_shape));
  int64_t tensor_raw_size = -1;
  GE_CHK_GRAPH_STATUS_RET(TensorUtils::CalcTensorMemSize(output_tensor_desc.GetShape(), output_tensor_desc.GetFormat(),
                                                         output_tensor_desc.GetDataType(), tensor_raw_size),
                          "Failed to DequeueTensor");
  GELOGD("MakeShared start size=%ld", tensor_raw_size);
  const auto output_aligned_ptr = MakeShared<AlignedPtr>(tensor_raw_size, kAlignmentVal64);
  GELOGD("MakeShared end size=%ld", tensor_raw_size);
  GE_CHECK_NOTNULL(output_aligned_ptr);
  const uint8_t *const data_addr =
      PtrAdd<uint8_t>(buff_aligned_ptr->MutableGet(), dequeue_size, sizeof(RuntimeTensorDesc));
  if (memcpy_s(output_aligned_ptr->MutableGet(), static_cast<size_t>(tensor_raw_size), data_addr,
               (dequeue_size - sizeof(RuntimeTensorDesc))) != EOK) {
    GELOGE(FAILED, "Failed to copy output tensor data copy size = %ld", tensor_raw_size);
    return FAILED;
  }
  tensor.SetData(output_aligned_ptr, static_cast<uint64_t>(tensor_raw_size));
  GELOGD("[Dequeue] ended successfully, device id = %d, queue id = %u, size = %ld", device_id, queue_id,
         tensor_raw_size);
  return SUCCESS;
}

Status LocalExchangeService::EnsureInitialized(const int32_t device_id) {
  const std::lock_guard<std::mutex> lk(mu_);
  if (initialized_devices_.find(device_id) == initialized_devices_.end()) {
    GELOGI("[InitQueue] start, device id = %d", device_id);
    const auto ret = rtMemQueueInit(device_id);
    if (ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtMemQueueInit fail, ret: 0x%X", ret);
      GELOGE(RT_FAILED, "[InitQueue] failed, rt_err = %d, device id = %d", ret, device_id);
      return RT_ERROR_TO_GE_STATUS(ret);
    }
    GELOGI("[InitQueue] ended successfully, device id = %d", device_id);
    (void)initialized_devices_.emplace(device_id);
  }
  return SUCCESS;
}
}  // namespace ge
