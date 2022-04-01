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

#ifndef GE_HYBRID_EXECUTOR_HYBRID_EXECUTION_CONTEXT_H_
#define GE_HYBRID_EXECUTOR_HYBRID_EXECUTION_CONTEXT_H_

#include <atomic>
#include <unordered_map>
#include "common/blocking_queue.h"
#include "common/properties_manager.h"
#include "framework/common/debug/ge_log.h"
#include "graph/ge_local_context.h"
#include "graph/runtime_inference_context.h"
#include "graph/load/model_manager/davinci_model.h"
#include "hybrid/common/npu_memory_allocator.h"
#include "hybrid/common/tensor_value.h"
#include "hybrid/executor/hybrid_profiler.h"
#include "hybrid/executor/node_done_manager.h"
#include "hybrid/executor/node_state.h"
#include "hybrid/executor/rt_callback_manager.h"
#include "hybrid/model/hybrid_model.h"
#include "hybrid/executor/hybrid_resource_manager.h"

// If expr is not SUCCESS, print the log and return the same value
#define HYBRID_CHK_STATUS_RET(expr, ...)        \
  do {                                          \
    const ge::Status _status = (expr);          \
    if (_status != ge::SUCCESS) {               \
      if (_status == ge::END_OF_SEQUENCE) {     \
        GELOGD("Got end of sequence");          \
      } else {                                  \
        GELOGE(_status, __VA_ARGS__);           \
      }                                         \
      return _status;                           \
    }                                           \
  } while (false)

namespace ge {
namespace hybrid {
struct GraphExecutionContext {
  GraphExecutionContext();
  ~GraphExecutionContext();
  GraphExecutionContext(const GraphExecutionContext &) = delete;
  GraphExecutionContext &operator=(const GraphExecutionContext &) = delete;
  GraphExecutionContext(GraphExecutionContext &&) = delete;
  GraphExecutionContext &operator=(GraphExecutionContext &&) = delete;

  void SetErrorCode(const Status error_code);
  Status InitProfiler() const;
  Status GetStatus() const;
  Status Synchronize(const rtStream_t rt_stream);
  Status DumpExceptionInfo(const std::vector<rtExceptionInfo> &exception_infos);
  bool IsDumpEnabled() const;

  uint64_t session_id = 0U;
  uint64_t context_id = 0U;
  const HybridModel *model = nullptr;
  const GEThreadLocalContext *ge_context = nullptr;
  rtStream_t stream = nullptr;
  rtStream_t hccl_stream = nullptr;
  rtContext_t rt_context = nullptr;
  CallbackManager *callback_manager = nullptr;
  NpuMemoryAllocator *allocator = nullptr;
  mutable std::unique_ptr<HybridProfiler> profiler = nullptr;
  DumpProperties dump_properties;
  bool trace_enabled = false;
  bool dump_enabled = false;
  bool is_host_cpu = false;
  ExceptionDumper exception_dumper;
  std::vector<std::shared_ptr<ge::DavinciModel>> davinci_model;
  std::atomic_bool is_eos_{false};
  static int64_t profiling_level;
  int64_t iteration = 0;
  void *global_step = nullptr;
  bool has_observer = false;
  bool own_callback_manager = false;
  RuntimeInferenceContext runtime_context_;
  int32_t stage_id{0};  // for pipeline model
  ResourceManager res_manager;

 private:
  Status status = SUCCESS;
  mutable std::mutex mu;
};
}  // namespace hybrid
}  // namespace ge

#define RECORD_PROFILING_EVENT(context, evt_type, fmt, category, node_name, ...)                                          \
do {                                                                                                                      \
  if (ge::hybrid::GraphExecutionContext::profiling_level > 0) {                                                           \
    if (((context) != nullptr) && ((context)->profiler != nullptr)) {                                                     \
      if ((node_name) != nullptr) {                                                                                       \
        (context)->profiler->RecordEvent((evt_type), "tid:%lu [%s@%ld] [%s] " fmt,                                        \
                                         GeLog::GetTid(), (node_name), (context)->iteration, (category), ##__VA_ARGS__);  \
      } else {                                                                                                            \
        (context)->profiler->RecordEvent((evt_type), "tid:%lu [%s] " fmt, GeLog::GetTid(), (category), ##__VA_ARGS__);    \
      }                                                                                                                   \
    }                                                                                                                     \
  }                                                                                                                       \
} while (false)

#define RECORD_MODEL_EXECUTION_EVENT(context, fmt, ...)                                                              \
do {                                                                                                                 \
  void *const name = nullptr;                                                                                        \
  RECORD_PROFILING_EVENT((context), HybridProfiler::EventType::GENERAL, fmt, "ModelExecutor", name, ##__VA_ARGS__);  \
} while (false)

#define RECORD_SHAPE_INFERENCE_EVENT(context, name, fmt, ...) \
  RECORD_PROFILING_EVENT((context), HybridProfiler::EventType::SHAPE_INFERENCE, fmt, "ShapeInference", (name),  ##__VA_ARGS__)

#define RECORD_COMPILE_EVENT(context, name, fmt, ...) \
  RECORD_PROFILING_EVENT((context), HybridProfiler::EventType::COMPILE, fmt, "Compilation", (name),  ##__VA_ARGS__)

#define RECORD_EXECUTION_EVENT(context, name, fmt, ...) \
  RECORD_PROFILING_EVENT((context), HybridProfiler::EventType::EXECUTION, fmt, "Execution", (name),  ##__VA_ARGS__)

#define RECORD_CALLBACK_EVENT(context, name, fmt, ...) \
  RECORD_PROFILING_EVENT((context), HybridProfiler::EventType::CALLBACKS, fmt, "Callback", (name),  ##__VA_ARGS__)

#endif // GE_HYBRID_EXECUTOR_HYBRID_EXECUTION_CONTEXT_H_
