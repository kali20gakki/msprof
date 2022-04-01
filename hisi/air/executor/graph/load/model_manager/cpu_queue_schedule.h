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
#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_CPU_QUEUE_SCHEDULE_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_CPU_QUEUE_SCHEDULE_H_

#include <cstdint>

#include "graph/load/model_manager/task_info/task_info.h"
#include "graph/load/model_manager/zero_copy_offset.h"

namespace ge {
// For AICPU task "modelDequeue" / "modelEnqueue"
struct MbufQueueInfo {
  uint32_t queue_id;        // Op queue id
  uintptr_t in_mbuf;        // addr for input mbuf
};

// For AICPU task "modelProcessOutput"
enum class ProcessStage {
  kPrepare,
  kPostDynamic,
  kPostStatic
};

struct ProcessOutputInfo {
  uint32_t data_size;       // output Tensor size
  uintptr_t data_addr;      // output Tensor addr
  uintptr_t in_mbuf;        // input mbuf, for fill output mbuf header
  uintptr_t out_mbuf;       // output mbuf addr
};

// For AICPU task "modelZeroCopy"
enum class ZeroCpyType {
  kAllStatic,
  kAllDynamic,
  kMixedCpy
};

struct ZeroCpyArgs {
  ZeroCpyType cpy_type;
  bool has_no_tiling;
  bool need_distribute;
};

struct AddrMapInfo {
  uint32_t addr_num{0U};
  uint64_t src_addr_list{0UL};
  uint64_t dst_addr_list{0UL};
};

struct AddrMapInfoV2 {
  uint32_t addr_num{0U};
  uint64_t src_addr_list{0UL};
  uint64_t dst_addr_list{0UL};
  uint64_t is_no_tiling_list{0UL};
  uint32_t len{0U};
  char_t extend_info[0];
};

///
/// @ingroup ge
/// @brief CpuTask base, inherit from TaskInfo used for manage.
///
class CpuTaskInfo : public TaskInfo {
 public:
  explicit CpuTaskInfo(const rtStream_t stream);
  ~CpuTaskInfo() override;

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override {
    (void)task_def;
    (void)davinci_model;
    return SUCCESS;
  }

 protected:
  void *args_{nullptr};
  uint32_t args_size_{0U};
 private:
  CpuTaskInfo &operator=(const CpuTaskInfo &) = delete;
  CpuTaskInfo(const CpuTaskInfo &) = delete;
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, bind input queue to task.
///
class CpuTaskModelDequeue : public CpuTaskInfo {
 public:
  explicit CpuTaskModelDequeue(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskModelDequeue() override = default;

  Status Init(const uint32_t queue_id, uintptr_t &in_mbuf);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, zero copy.
///
class CpuTaskZeroCopy : public CpuTaskInfo {
 public:
  explicit CpuTaskZeroCopy(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskZeroCopy() override;

  Status Init(std::vector<uintptr_t> &mbuf_list,
              const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
              const std::vector<bool> &is_no_tiling_list,
              ZeroCpyArgs &cpy_args);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
  Status InitAddrs(std::vector<uintptr_t> &mbuf_list,
                   const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
                   const std::vector<bool> &is_no_tiling_list,
                   const ZeroCpyArgs &cpy_args);

 private:
  void *src_addr_{nullptr};
  void *dst_addr_{nullptr};
  void *no_tiling_addr_{nullptr};
  bool has_no_tiling_ = false;
  uint32_t addr_num_ = 0U;
  std::vector<uint64_t> src_addrs_;
  std::vector<uint64_t> dst_addrs_;
  std::vector<int32_t> no_tilings_;
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, active original model stream.
///
class CpuTaskProcessOutput : public CpuTaskInfo {
 public:
  explicit CpuTaskProcessOutput(const rtStream_t stream,
                                const ProcessStage stage)
      : CpuTaskInfo(stream), stage_(stage) {}
  ~CpuTaskProcessOutput() override = default;

  Status Init(const uintptr_t addr, const uint32_t size, const uintptr_t in_mbuf, uintptr_t &out_mbuf);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
  ProcessStage stage_;
};

class CpuTaskModelEnqueue : public CpuTaskInfo {
 public:
  explicit CpuTaskModelEnqueue(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskModelEnqueue() override = default;

  Status Init(const uint32_t queue_id, const uintptr_t out_mbuf);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, active entry stream.
///
class CpuTaskActiveEntry : public CpuTaskInfo {
 public:
  explicit CpuTaskActiveEntry(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskActiveEntry() override = default;

  Status Init(const rtStream_t stream);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
  rtStream_t active_stream_{nullptr};
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, wait for end graph.
///
class CpuTaskWaitEndGraph : public CpuTaskInfo {
 public:
  explicit CpuTaskWaitEndGraph(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskWaitEndGraph() override = default;

  Status Init(const uint32_t model_id);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
};

///
/// @ingroup ge
/// @brief definiteness queue schedule, repeat run model.
///
class CpuTaskModelRepeat : public CpuTaskInfo {
 public:
  explicit CpuTaskModelRepeat(const rtStream_t stream) : CpuTaskInfo(stream) {}
  ~CpuTaskModelRepeat() override = default;

  Status Init(const uint32_t model_id);

  Status Distribute() override;

 private:
  using CpuTaskInfo::Init;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_CPU_QUEUE_SCHEDULE_H_
