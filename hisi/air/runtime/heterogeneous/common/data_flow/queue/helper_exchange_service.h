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
#ifndef RUNTIME_DEPLOY_HELPER_EXCHANGE_SERVICE_H_
#define RUNTIME_DEPLOY_HELPER_EXCHANGE_SERVICE_H_

#include <mutex>
#include <set>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "runtime/rt_mem_queue.h"
#include "exec_runtime/deploy/exchange_service.h"

namespace ge {
class HelperExchangeService : public ExchangeService {
 public:
  static HelperExchangeService &GetInstance() {
    static HelperExchangeService instance;
    return instance;
  }

  HelperExchangeService() = default;
  virtual ~HelperExchangeService();

  struct TransInfoContext {
    int32_t device_id;
    uint32_t queue_id;
    bool operator < (const TransInfoContext &other) const {
      if (device_id != other.device_id) {
        return device_id < other.device_id;
      } else {
        return queue_id < other.queue_id;
      }
    }
  };

  Status Initialize();

  Status Finalize();

  Status CreateQueue(int32_t device_id,
                     const std::string &name,
                     uint32_t depth,
                     uint32_t work_mode,
                     uint32_t &queue_id) override;
  Status DestroyQueue(int32_t device_id, uint32_t queue_id) override;
  Status Enqueue(int32_t device_id, uint32_t queue_id, const void *data, size_t size) override;
  Status Enqueue(int32_t device_id, uint32_t queue_id, size_t size, const FillFunc &fill_func) override;
  Status Peek(int32_t device_id, uint32_t queue_id, size_t &size) override;
  Status Dequeue(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_Info) override;
  Status DequeueTensor(int32_t device_id, uint32_t queue_id, GeTensor &tensor, ControlInfo &control_Info) override;
  Status EnsureInitialized(int32_t device_id);
  void InitializeTransInfo(const int32_t device_id, const uint32_t queue_id);
  void DestroyTransInfo(const int32_t device_id, const uint32_t queue_id);
  Status GetTransId(const int32_t device_id, const uint32_t queue_id, uint64_t &trans_id);
  Status SetTransId(const int32_t device_id, const uint32_t queue_id, rtMbufPtr_t mbuf);

 private:
  Status EnqueueMbuf(int32_t device_id, uint32_t queue_id, rtMbufPtr_t m_buf);
  Status DequeueMbuf(int32_t device_id, uint32_t queue_id, rtMbufPtr_t *m_buf);
  static Status CopyMbufTo(void *m_buf, void *data, size_t size);
  static Status CopyMbufHeadTo(void *m_buf, void *&Control_data, size_t &head_size);
  void ProcessEmptyToNotEmptyEvent(const uint32_t queue_id);
  void ProcessF2NFEvent(const uint32_t queue_id);
  void WaitEvents(const int32_t device_id);
  Status InitializeEvents(const int32_t device_id);
  Status EnsureEnqueueSubscribed(const int32_t device_id, const uint32_t queue_id);
  Status EnsureDequeueSubscribed(const int32_t device_id, const uint32_t queue_id);

  std::mutex mu_;
  std::set<int32_t> initialized_devices_;

  std::mutex trans_mu_;
  std::map<TransInfoContext, uint64_t> trans_ids_;

  std::mutex dequeue_mu_;
  std::condition_variable dequeue_cv_;
  std::map<uint32_t, bool> subscribed_dequeues_; // key is qid, true means not empty

  std::mutex enqueue_mu_;
  std::condition_variable enqueue_cv_;
  std::map<uint32_t, bool> subscribed_enqueues_; // key is qid, true means not full

  std::atomic<bool> waiting_{true};
  std::vector<std::thread> events_threads_;
  std::mutex events_mu_;
  std::set<int32_t> events_devices_;
};
}  // namespace ge

#endif  // RUNTIME_DEPLOY_HELPER_EXCHANGE_SERVICE_H_
