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
#include "common/data_flow/queue/helper_exchange_service.h"
#include <thread>
#include <chrono>
#include "framework/common/debug/log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "runtime/rt_mem_queue.h"
#include "common/utils/bind_cpu_utils.h"
#include "external/runtime/rt_error_codes.h"
#include "common/utils/rts_api_utils.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/mem_utils.h"

namespace ge {
namespace {
constexpr int32_t kQueueOpTimeout = 10 * 60 * 1000;  // 10 min
constexpr uint32_t kBufferCount = 1U;
constexpr size_t kContextLen = 0U;
constexpr size_t kAlignmentVal64 = 64U;
constexpr uint32_t kMbufHeadMaxSize = 256U;
constexpr uint32_t kMbufHeadEndOfSequencePos = 128U;
constexpr uint8_t kEndOfSequenceFlag = 0x5A;
constexpr size_t kTransIdOffset = 64U;
constexpr uint32_t kEventGroupId = 0U;
constexpr int32_t kWaitEventTimeout = 1000; // 1s
constexpr int32_t kRtQueueTypeSingle = 2;
constexpr int32_t kDequeueInterval = 1; // 1s
constexpr int32_t kDequeueRetryMaxTimes = 60 * 5;
constexpr int32_t kEnqueueInterval = 100; // 100ms
constexpr int32_t kEnqueueRetryMaxTimes = 10 * 60 * 5;
}  // namespace

HelperExchangeService::~HelperExchangeService() {
  (void)Finalize();
}

Status HelperExchangeService::Initialize() {
  return SUCCESS;
}

Status HelperExchangeService::Finalize() {
  waiting_.store(false);
  for (auto &th : events_threads_) {
    if (th.joinable()) {
      th.join();
    }
  }
  {
    std::lock_guard<std::mutex> lk(mu_);
    initialized_devices_.clear();
  }
  {
    std::lock_guard<std::mutex> lk(dequeue_mu_);
    subscribed_dequeues_.clear();
  }
  {
    std::lock_guard<std::mutex> lk(enqueue_mu_);
    subscribed_enqueues_.clear();
  }
  return SUCCESS;
}

Status HelperExchangeService::EnsureInitialized(int32_t device_id) {
  std::lock_guard<std::mutex> lk(mu_);
  if (initialized_devices_.find(device_id) == initialized_devices_.end()) {
    GELOGI("[InitQueue] start, device id = %d", device_id);
    auto ret = rtMemQueueInit(device_id);
    if (ret != RT_ERROR_NONE && ret != ACL_ERROR_RT_REPEATED_INIT) {
      REPORT_CALL_ERROR("E19999", "Call rtMemQueueInit fail, ret: 0x%X", ret);
      GELOGE(RT_FAILED, "[InitQueue] failed, rt_err = %d, device id = %d", ret, device_id);
      return RT_ERROR_TO_GE_STATUS(ret);
    }
    GELOGI("[InitQueue] ended successfully, device id = %d", device_id);
    initialized_devices_.emplace(device_id);
  }
  return SUCCESS;
}

void HelperExchangeService::ProcessEmptyToNotEmptyEvent(const uint32_t queue_id) {
  std::lock_guard<std::mutex> lk(dequeue_mu_);
  auto it = subscribed_dequeues_.find(queue_id);
  GELOGI("[ReceiveEvent] receive queue id[%u] event, find result[%d]",
         queue_id, static_cast<int32_t>(it != subscribed_dequeues_.end()));
  if (it != subscribed_dequeues_.end()) {
    GELOGI("[ReceiveEvent] received queue id[%u] enqueue, notify dequeue", queue_id);
    it->second = true;
    dequeue_cv_.notify_all();
  }
}

void HelperExchangeService::ProcessF2NFEvent(const uint32_t queue_id) {
  std::lock_guard<std::mutex> lk(enqueue_mu_);
  auto it = subscribed_enqueues_.find(queue_id);
  GELOGI("[ReceiveEvent] receive queue id[%u] event, find result[%d]",
         queue_id, static_cast<int32_t>(it != subscribed_enqueues_.end()));
  if (it != subscribed_enqueues_.end()) {
    GELOGI("[ReceiveEvent] received queue id[%u] not full, notify enqueue", queue_id);
    it->second = true;
    enqueue_cv_.notify_all();
  }
}

void HelperExchangeService::WaitEvents(const int32_t device_id) {
  // host device_id is 0, bind cpu 0
  // device each device has 8 cpu, bind 0/8 cpu when device_id is 0/1
  BindCpuUtils::BindCore(device_id * 8);
  uint64_t mask = (1ULL << static_cast<uint32_t>(RT_EVENT_QUEUE_EMPTY_TO_NOT_EMPTY)) |
                  (1ULL << static_cast<uint32_t>(RT_EVENT_QUEUE_FULL_TO_NOT_FULL));
  auto result = RtsApiUtils::EschedSubscribeEvent(device_id, kEventGroupId, 0, mask);
  if (result != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "Failed to invoke EschedSubscribeEvent, ret = 0x%X", result);
    return;
  }
  GELOGI("Event thread start successfully, device id = %d", device_id);
  while (waiting_) {
    rtEschedEventSummary_t in_event;
    auto ret = rtEschedWaitEvent(device_id, kEventGroupId, 0, kWaitEventTimeout, &in_event);
    if (ret == RT_ERROR_NONE) {
      if (in_event.eventId == RT_EVENT_QUEUE_EMPTY_TO_NOT_EMPTY) {
        ProcessEmptyToNotEmptyEvent(in_event.subeventId);
      } else if (in_event.eventId == RT_EVENT_QUEUE_FULL_TO_NOT_FULL) {
        ProcessF2NFEvent(in_event.subeventId);
      } else {
        GELOGW("[WaitEvents] receive unsupported event[%d]", in_event.eventId);
      }
    } else {
      GELOGW("Failed to invoke rtEschedWaitEvent, ret = 0x%X", ret);
    }
  }
  GELOGI("Event thread exist successfully, device id = %d", device_id);
}

Status HelperExchangeService::EnsureEnqueueSubscribed(const int32_t device_id, const uint32_t queue_id) {
  auto it = subscribed_enqueues_.find(queue_id);
  if (it == subscribed_enqueues_.end()) {
    GELOGI("[EnqueueSubscribe] start, queue id = %u", queue_id);
    auto ret = rtQueueSubF2NFEvent(device_id, queue_id, kEventGroupId);
    if (ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtQueueSubF2NFEvent fail, ret: 0x%X", ret);
      GELOGE(RT_FAILED, "[EnqueueSubscribe] failed, rt_err = %d, queue id = %d", ret, queue_id);
      return RT_ERROR_TO_GE_STATUS(ret);
    }
    GELOGI("[EnqueueSubscribe] ended successfully, queue id = %u", queue_id);
  }
  subscribed_enqueues_[queue_id] = false;
  return SUCCESS;
}

Status HelperExchangeService::EnsureDequeueSubscribed(const int32_t device_id, const uint32_t queue_id) {
  auto it = subscribed_dequeues_.find(queue_id);
  if (it == subscribed_dequeues_.end()) {
    GELOGI("[DequeueSubscribe] start, queue id = %u", queue_id);
    auto ret = rtQueueSubscribe(device_id, queue_id, kEventGroupId, kRtQueueTypeSingle);
    if (ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtQueueSubscribe fail, ret: 0x%X", ret);
      GELOGE(RT_FAILED, "[DequeueSubscribe] failed, rt_err = %d, queue id = %d", ret, queue_id);
      return RT_ERROR_TO_GE_STATUS(ret);
    }
    GELOGI("[DequeueSubscribe] ended successfully, queue id = %u", queue_id);
  }
  subscribed_dequeues_[queue_id] = false;
  return SUCCESS;
}

Status HelperExchangeService::InitializeEvents(const int32_t device_id) {
  std::lock_guard<std::mutex> lk(events_mu_);
  if (events_devices_.find(device_id) == events_devices_.end()) {
    GELOGI("[InitEvents] start, device id = %d", device_id);
    GE_CHK_STATUS_RET(RtsApiUtils::EschedAttachDevice(device_id));
    GE_CHK_STATUS_RET(RtsApiUtils::EschedCreateGroup(device_id, kEventGroupId, RT_GRP_TYPE_BIND_CP_CPU));
    events_threads_.emplace_back(&HelperExchangeService::WaitEvents, this, device_id);
    GELOGI("[InitEvents] ended successfully, device id = %d", device_id);
    events_devices_.emplace(device_id);
  }
  return SUCCESS;
}

void HelperExchangeService::InitializeTransInfo(const int32_t device_id, const uint32_t queue_id) {
  std::lock_guard<std::mutex> lk(trans_mu_);
  TransInfoContext context{device_id, queue_id};
  auto iter = trans_ids_.find(context);
  if (iter == trans_ids_.end()) {
    trans_ids_[context] = 0;
    GELOGD("Init trans info successfully, device id = %d, queue id = %u", device_id, queue_id);
  }
}

void HelperExchangeService::DestroyTransInfo(const int32_t device_id, const uint32_t queue_id) {
  std::lock_guard<std::mutex> lk(trans_mu_);
  TransInfoContext context{device_id, queue_id};
  auto iter = trans_ids_.find(context);
  if (iter != trans_ids_.end()) {
    trans_ids_.erase(iter);
    GELOGI("Destroy trans info successfully, device id = %d, queue id = %u", device_id, queue_id);
  }
}

Status HelperExchangeService::GetTransId(const int32_t device_id, const uint32_t queue_id, uint64_t &trans_id) {
  std::lock_guard<std::mutex> lk(trans_mu_);
  TransInfoContext context{device_id, queue_id};
  auto iter = trans_ids_.find(context);
  if (iter != trans_ids_.end()) {
    trans_id = ++trans_ids_[context];
    GELOGI("Get trans id[%lu] successfully, device id = %d, queue id = %u", trans_id, device_id, queue_id);
    return SUCCESS;
  }
  GELOGE(PARAM_INVALID, "Get trans id failed, device id = %d, queue id = %u", device_id, queue_id);
  return PARAM_INVALID;
}

Status HelperExchangeService::SetTransId(const int32_t device_id, const uint32_t queue_id, rtMbufPtr_t mbuf) {
  uint64_t trans_id = 0U;
  GE_CHK_STATUS_RET(GetTransId(device_id, queue_id, trans_id), "Get trans id failed, device id = %d, queue id = %u",
      device_id, queue_id);
  void *head_buf = nullptr;
  uint64_t head_size = 0U;
  auto ret = rtMbufGetPrivInfo(mbuf, &head_buf, &head_size);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMbufGetPrivInfo fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "Set trans id failed, rt_err = %d, device id = %d, queue id = %u", ret, device_id, queue_id);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  uint64_t *mbuf_trans_id = reinterpret_cast<uint64_t *>(static_cast<char_t *>(head_buf) + head_size - kTransIdOffset);
  *mbuf_trans_id = trans_id;
  return SUCCESS;
}

Status HelperExchangeService::CreateQueue(int32_t device_id,
                                          const std::string &name,
                                          uint32_t depth,
                                          uint32_t work_mode,
                                          uint32_t &queue_id) {
  if (name.size() >= static_cast<size_t>(RT_MQ_MAX_NAME_LEN - 1)) {
    GELOGE(PARAM_INVALID,
           "[CreateQueue] [CheckParam] Length of queue name out of range, name = %s, length = %zu",
           name.c_str(),
           name.size());
    return PARAM_INVALID;
  }
  GELOGD("[CreateQueue] start, device id = %d, queue name = %s, depth = %u, work_mode = %u",
         device_id,
         name.c_str(),
         depth,
         work_mode);
  GE_CHK_STATUS_RET(EnsureInitialized(device_id), "[CreateQueue] [Init] failed, queue name = %s", name.c_str());
  rtMemQueueAttr_t attr;
  attr.depth = depth;
  attr.workMode = work_mode;
  attr.flowCtrlFlag = false;
  attr.flowCtrlDropTime = static_cast<uint32_t>(kQueueOpTimeout);
  attr.overWriteFlag = false;
  // actually this won't fail, length was checked
  if (strcpy_s(attr.name, sizeof(attr.name), name.c_str()) != EOK) {
    GELOGE(FAILED, "[CreateQueue] [CopyName] Failed");
    return FAILED;
  }

  auto ret = rtMemQueueCreate(device_id, &attr, &queue_id);
  if (ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemQueueCreate fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[CreateQueue] failed, rt_err = %d, device id = %d, queue name = %s, depth = %u",
           ret,
           device_id,
           name.c_str(),
           depth);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  InitializeTransInfo(device_id, queue_id);
  GELOGD("[CreateQueue] ended successfully, device id = %d, queue name = %s, depth = %u, queue_id = %u",
         device_id,
         name.c_str(),
         depth,
         queue_id);
  return SUCCESS;
}

Status HelperExchangeService::DestroyQueue(int32_t device_id, uint32_t queue_id) {
  GELOGD("[DestroyQueue] start, device id = %d, queue_id = %u", device_id, queue_id);
  auto ret = rtMemQueueDestroy(device_id, queue_id);
  if (ret != RT_ERROR_NONE) {
        REPORT_CALL_ERROR("E19999", "Call rtMemQueueDestroy fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[DestroyQueue] failed, rt_err = %d, device id = %d, queue id = %u",
           ret,
           device_id,
           queue_id);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  DestroyTransInfo(device_id, queue_id);
  GELOGD("[DestroyQueue] ended successfully, device id = %d, queue_id = %u", device_id, queue_id);
  return SUCCESS;
}

Status HelperExchangeService::Enqueue(int32_t device_id, uint32_t queue_id, const void *data, size_t size) {
  FillFunc fill_func = [data, size](void *buffer, size_t buffer_size) {
    if (memcpy_s(buffer, buffer_size, data, size) == EOK) {
      return SUCCESS;
    }
    return FAILED;
  };
  return Enqueue(device_id, queue_id, size, fill_func);
}

Status HelperExchangeService::Enqueue(int32_t device_id,
                                      uint32_t queue_id,
                                      size_t size,
                                      const ExchangeService::FillFunc &fill_func) {
  rtMbufPtr_t m_buf = nullptr;
  void *buffer = nullptr;
  GE_CHK_RT_RET(rtMbufAlloc(&m_buf, size));
  GE_CHK_RT_RET(rtMbufGetBuffAddr(m_buf, &buffer));

  // clear end of sequence flag
  void *priv_info = nullptr;
  uint64_t priv_size = 0UL;
  GE_CHK_RT_RET(rtMbufGetPrivInfo(m_buf, &priv_info, &priv_size));
  if (priv_size > kMbufHeadEndOfSequencePos) {
    *(static_cast<uint8_t *>(priv_info) + kMbufHeadEndOfSequencePos) = 0U;
  }

  auto ret = fill_func(buffer, size);
  if (ret != SUCCESS) {
    GELOGE(ret, "[CopyTo][Mbuf] failed, addr = %p, size = %zu", buffer, size);
    (void) rtMbufFree(m_buf);
    return ret;
  }

  ret = EnqueueMbuf(device_id, queue_id, m_buf);
  if (ret != SUCCESS) {
    (void) rtMbufFree(m_buf);
    return ret;
  }
  // mbuf will be freed by consumer
  GELOGD("[Enqueue][Mbuf] succeeded, device_id = %d, queue_id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status HelperExchangeService::Dequeue(int32_t device_id, uint32_t queue_id, void *data, size_t size,
                                      ControlInfo &control_info) {
  rtMbufPtr_t m_buf = nullptr;
  GE_CHK_STATUS_RET_NOLOG(DequeueMbuf(device_id, queue_id, &m_buf));
  GE_MAKE_GUARD(m_buf, [m_buf]() {
    GE_CHK_RT(rtMbufFree(m_buf));
  });

  size_t head_size = 0;
  void *head_buf = nullptr;
  GE_CHK_STATUS(CopyMbufHeadTo(m_buf, head_buf, head_size));
  if ((head_buf != nullptr) && (head_size > kMbufHeadEndOfSequencePos)) {
    uint64_t value = PtrToValue(head_buf);
    uint8_t end_of_sequence = *(reinterpret_cast<uint8_t *>(value + kMbufHeadEndOfSequencePos));
    if (end_of_sequence == kEndOfSequenceFlag) {
      control_info.end_of_sequence_flag = true;
      GELOGI("[Dequeue] End of sequence is coming.");
      return SUCCESS;
    }
  }

  GE_CHK_STATUS(CopyMbufTo(m_buf, data, size));
  GELOGD("[Dequeue] succeeded, device_id = %d, queue_id = %u, size = %zu", device_id, queue_id, size);
  return SUCCESS;
}

Status HelperExchangeService::DequeueTensor(int32_t device_id, uint32_t queue_id, GeTensor &tensor,
                                            ControlInfo &control_info) {
  rtMbufPtr_t m_buf = nullptr;
  GE_CHK_STATUS_RET_NOLOG(DequeueMbuf(device_id, queue_id, &m_buf));
  GE_MAKE_GUARD(m_buf, [m_buf]() { GE_CHK_RT(rtMbufFree(m_buf)); });

  size_t head_size = 0;
  void *head_buf = nullptr;
  GE_CHK_STATUS(CopyMbufHeadTo(m_buf, head_buf, head_size));
  if ((head_buf != nullptr) && (head_size >= kMbufHeadEndOfSequencePos + sizeof(uint8_t))) {
    uint64_t value = PtrToValue(head_buf);
    uint8_t end_of_sequence = *(reinterpret_cast<uint8_t *>(value + kMbufHeadEndOfSequencePos));
    if (end_of_sequence == kEndOfSequenceFlag) {
      control_info.end_of_sequence_flag = true;
      GELOGI("[Dequeue] End of sequence is coming.");
      return SUCCESS;
    }
  }

  auto &output_tensor_desc = tensor.MutableTensorDesc();
  void *data_buffer = nullptr;
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetBufferAddr(m_buf, &data_buffer));
  GELOGD("Got buffer address = %p", data_buffer);
  RuntimeTensorDesc *const mbuf_tensor_desc = PtrToPtr<void, RuntimeTensorDesc>(data_buffer);
  std::vector<int64_t> output_shape;
  for (size_t j = 1U; j <= static_cast<size_t>(mbuf_tensor_desc->shape[0U]); ++j) {
    output_shape.push_back(mbuf_tensor_desc->shape[j]);
  }
  output_tensor_desc.SetShape(GeShape(output_shape));
  int64_t tensor_raw_size = -1;
  GE_CHK_GRAPH_STATUS_RET(TensorUtils::CalcTensorMemSize(output_tensor_desc.GetShape(), output_tensor_desc.GetFormat(),
                                                         output_tensor_desc.GetDataType(), tensor_raw_size),
                          "Failed to DequeueTensor");
  GELOGD("Tensor size = %zu", tensor_raw_size);
  if (tensor_raw_size > 0) {
    auto output_aligned_ptr = MakeShared<AlignedPtr>(tensor_raw_size, kAlignmentVal64);
    GELOGD("Tensor buffer allocated, size = %zu", tensor_raw_size);
    GE_CHECK_NOTNULL(output_aligned_ptr);
    const void *data_addr = PtrAdd<void>(data_buffer, (sizeof(RuntimeTensorDesc) + 1UL), sizeof(RuntimeTensorDesc));
    if (memcpy_s(output_aligned_ptr->MutableGet(), static_cast<size_t>(tensor_raw_size), data_addr,
                 static_cast<size_t>(tensor_raw_size)) != EOK) {
      GELOGE(FAILED, "Failed to copy output tensor data copy size = %ld", tensor_raw_size);
      return FAILED;
    }
    tensor.SetData(output_aligned_ptr, static_cast<uint64_t>(tensor_raw_size));
  }
  GELOGD("[Dequeue] succeeded, device_id = %d, queue_id = %u, size = %zu", device_id, queue_id, tensor_raw_size);
  return SUCCESS;
}

Status HelperExchangeService::Peek(int32_t device_id, uint32_t queue_id, size_t &size) {
  GELOGE(UNSUPPORTED, "[Peek] operation not supported.");
  return UNSUPPORTED;
}


Status HelperExchangeService::EnqueueMbuf(int32_t device_id, uint32_t queue_id, rtMbufPtr_t m_buf) {
  GE_CHK_STATUS_RET(InitializeEvents(device_id), "[Enqueue] [Init] failed, device id = %d", device_id);
  GE_CHK_STATUS_RET(SetTransId(device_id, queue_id, m_buf),
                    "Set mbuf trans id failed, device_id = %d, queue_id = %u",
                    device_id, queue_id);
  std::unique_lock<std::mutex> lk(enqueue_mu_);
  GE_CHK_STATUS_RET(EnsureEnqueueSubscribed(device_id, queue_id),
                    "[Enqueue] [Init] failed, queue id = %u", queue_id);
  auto ret = RT_ERROR_NONE;
  int32_t retry_cnt = 0;
  while (true) {
    ret = rtMemQueueEnQueue(device_id, queue_id, m_buf);
    if (ret == RT_ERROR_NONE) {
      return SUCCESS;
    }
    if (ret == ACL_ERROR_RT_QUEUE_FULL) {
      GELOGD("[Enqueue][MBuf] failed, queue is full, device_id = %d, queue_id = %u", device_id, queue_id);
      if (enqueue_cv_.wait_for(lk, std::chrono::milliseconds(kEnqueueInterval), [this, queue_id] {
            return subscribed_enqueues_[queue_id];
          })) {
        GELOGI("[Enqueue] receive f2nf event");
        continue;
      } else {
        retry_cnt++;
        if (retry_cnt < kEnqueueRetryMaxTimes) {
          continue;
        }
      }
    }

    GELOGE(RT_FAILED, "[Enqueue][MBuf] failed, device_id = %d, queue_id = %u, retry cnt = %d, rt_error_code = %d",
           device_id, queue_id, retry_cnt, ret);
    ret = RT_ERROR_TO_GE_STATUS(ret);
    break;
  }
  return ret;
}

Status HelperExchangeService::DequeueMbuf(int32_t device_id, uint32_t queue_id, rtMbufPtr_t *m_buf) {
  GE_CHK_STATUS_RET(InitializeEvents(device_id), "[Dequeue] [Init] failed, device id = %d", device_id);
  std::unique_lock<std::mutex> lk(dequeue_mu_);
  GE_CHK_STATUS_RET(EnsureDequeueSubscribed(device_id, queue_id),
                    "[Dequeue] [Init] failed, queue id = %u", queue_id);
  auto ret = RT_ERROR_NONE;
  int32_t retry_cnt = 0;
  while (true) {
    ret = rtMemQueueDeQueue(device_id, queue_id, m_buf);
    if (ret == RT_ERROR_NONE) {
      return SUCCESS;
    }
    if (ret == ACL_ERROR_RT_QUEUE_EMPTY) {
      GELOGD("[Dequeue][MBuf] failed, queue is empty, device_id = %d, queue_id = %u", device_id, queue_id);
      if (dequeue_cv_.wait_for(lk, std::chrono::seconds(kDequeueInterval), [this, queue_id] {
            return subscribed_dequeues_[queue_id];
          })) {
        GELOGI("[Dequeue] receive enqueue event");
        continue;
      } else {
        retry_cnt++;
        if (retry_cnt < kDequeueRetryMaxTimes) {
          continue;
        }
      }
    }

    GELOGE(RT_FAILED, "[Dequeue][MBuf] failed, device_id = %d, queue_id = %u, retry cnt = %d, rt_error_code = %d",
           device_id, queue_id, retry_cnt, ret);
    ret = RT_ERROR_TO_GE_STATUS(ret);
    break;
  }
  return ret;
}

Status HelperExchangeService::CopyMbufHeadTo(void *m_buf, void *&control_data, size_t &head_size) {
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetPrivData(m_buf, &control_data, &head_size));
  return SUCCESS;
}

Status HelperExchangeService::CopyMbufTo(void *m_buf, void *data, size_t size) {
  uint64_t buffer_size = 0;
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetBufferSize(m_buf, buffer_size));
  void *data_buffer = nullptr;
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetBufferAddr(m_buf, &data_buffer));
  GELOGD("Got buffer address = %p", data_buffer);
  if (memcpy_s(data, size, data_buffer, buffer_size) != EOK) {
    GELOGE(FAILED, "Failed to copy buffer to user, dst = %p, src = %p, dst size = %zu, copy size = %lu",
           data, data_buffer, size, buffer_size);
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge
