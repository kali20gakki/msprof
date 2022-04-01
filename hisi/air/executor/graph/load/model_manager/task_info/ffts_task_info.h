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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_FFTS_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_FFTS_TASK_INFO_H_

#include "graph/op_desc.h"
#include "graph/load/model_manager/task_info/task_info.h"

namespace ge {
class FftsTaskInfo : public TaskInfo {
 public:
  FftsTaskInfo() = default;
  ~FftsTaskInfo() override;

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Distribute() override;

  Status UpdateArgs() override;

  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

 private:
  Status InitFftsDescInfo(const domi::FftsDescInfoDef &ffts_desc_def, rtFftsDescInfo_t &ffts_desc) const;
  Status InitSubTaskInfo(const domi::FftsSubTaskDef &sub_task_def, rtFftsSubTaskInfo_t &sub_task_desc);
  Status InitTicketCache(const domi::TicketCacheDef &ticket_cache_def, rtTicketCache_t &ticket_cache) const;

  Status InitAutoAicAiv(const domi::AutoThreadAicAivDef &aic_aiv_def, rtAutoThreadAicAivInfo_t &aic_aiv);
  Status InitAutoCacheInfo(const domi::AutoThreadCacheDef &cache_def, rtAutoThreadCacheInfo_t &cache) const;
  Status InitAutoPrefetch(const domi::AutoThreadPrefetchDef &prefetch_def, rtAutoThreadPrefetch_t &prefetch) const;

  Status InitManualAicAivCheck(const domi::ManualThreadAicAivDef &aic_aiv_def) const;
  Status InitManualAicAiv(const domi::ManualThreadAicAivDef &aic_aiv_def, rtManualThreadAicAivInfo_t &aic_aiv);
  Status InitManualCacheInfo(const domi::ManualThreadCacheDef &cache_def, rtManualThreadCacheInfo_t &cache_info) const;
  Status InitManualDependency(const domi::ManualThreadDependencyDef &dependency_def,
                              rtManualThreadDependency_t &dependency) const;
  Status InitManualNop(const domi::ManualThreadNopDef &nop_def, rtManualThreadNopInfo_t &nop_info) const;

  Status InitManualDmuInfo(const domi::ManualThreadDmuDef &dmu_def, rtManualThreadDmuInfo_t &dmu) const;
  Status InitManualDmuInfo(const domi::ManualThreadCacheDef &cache_def, rtManualThreadDmuInfo_t *&dmu) const;
  Status InitManualDmuInfo(const domi::ManualThreadAicAivDef &aic_aiv_def, rtManualThreadDmuInfo_t *&dmu) const;

  template<typename T>
  Status InitIoAddrs(const RuntimeParam &rts_param, const T &aic_aiv_def,
                     const uint32_t thread_dim, const uint32_t addr_count);

  DavinciModel *davinci_model_{nullptr};
  rtFftsTaskInfo_t sub_task_info_{};
  std::vector<void *> io_addrs_;
  uint32_t thread_dim_{0U};
  void *args_{nullptr};    // runtime args memory
  uint64_t args_size_{0UL};    // runtime args memory length
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_FFTS_TASK_INFO_H_
