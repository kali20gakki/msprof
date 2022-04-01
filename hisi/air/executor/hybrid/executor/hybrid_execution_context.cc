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

#include "hybrid/executor/hybrid_execution_context.h"

#include <atomic>

#include "graph/load/model_manager/model_manager.h"

namespace ge {
namespace hybrid {
namespace {
const int32_t kEndOfSequence = 0x0704000a;
const int32_t kEndOfSequenceNew = 507005;
const int32_t kModelAbortNormal = 0x0704000e;
const int32_t kModelAbortNormalNew = 507024;
const int32_t kIntBase = 10;

const char_t *const kEnvProfilingLevel = "HYBRID_PROFILING_LEVEL";
}  // namespace

int64_t GraphExecutionContext::profiling_level = 0;

GraphExecutionContext::GraphExecutionContext() {
  static std::atomic_ulong context_id_gen{};
  context_id = context_id_gen++;
}

GraphExecutionContext::~GraphExecutionContext() {
  if (own_callback_manager && (callback_manager != nullptr)) {
    delete callback_manager;
    callback_manager = nullptr;
  }
}

Status GraphExecutionContext::InitProfiler() const {
  char_t env_profiling_level[MMPA_MAX_PATH] = {};
  const INT32 res = mmGetEnv(kEnvProfilingLevel, &(env_profiling_level[0U]), static_cast<uint32_t>(MMPA_MAX_PATH));
  if (res == EN_OK) {
    GraphExecutionContext::profiling_level = std::strtol(&(env_profiling_level[0U]), nullptr, kIntBase);
    GELOGD("Got profiling level = %ld", GraphExecutionContext::profiling_level);
    if (GraphExecutionContext::profiling_level > 0) {
      profiler = MakeUnique<HybridProfiler>();
      GE_CHECK_NOTNULL(profiler);
    }
  }
  return SUCCESS;
}

void GraphExecutionContext::SetErrorCode(const Status error_code) {
  const std::lock_guard<std::mutex> lk(mu);
  this->status = error_code;
}

Status GraphExecutionContext::GetStatus() const {
  const std::lock_guard<std::mutex> lk(mu);
  return this->status;
}

Status GraphExecutionContext::Synchronize(const rtStream_t rt_stream) {
  const auto rt_ret = rtStreamSynchronize(rt_stream);
  if (rt_ret == RT_ERROR_NONE) {
    return SUCCESS;
  }

  if ((rt_ret == kEndOfSequence) || (rt_ret == kEndOfSequenceNew)) {
    GELOGI("Got end of sequence");
    is_eos_ = true;
    return END_OF_SEQUENCE;
  }

  if ((rt_ret == kModelAbortNormal) || (rt_ret == kModelAbortNormalNew)) {
    GELOGI("The model with multiple datasets aborts normally");
    return SUCCESS;
  }

  GELOGE(RT_FAILED, "[Invoke][rtStreamSynchronize] failed, ret = %d", rt_ret);
  REPORT_CALL_ERROR("E19999", "invoke rtStreamSynchronize failed, ret = %d", rt_ret);
  return RT_FAILED;
}

Status GraphExecutionContext::DumpExceptionInfo(const std::vector<rtExceptionInfo> &exception_infos) {
  if (exception_infos.empty()) {
    GELOGI("[Dump][ExceptionInfo] Exception info is null.");
    return SUCCESS;
  }
  GELOGI("[Dump][ExceptionInfo] Start to search dynamic op info and to dump.");
  const bool is_dump_exception_from_dynamic_graph = (exception_dumper.DumpExceptionInfo(exception_infos) == SUCCESS);
  GELOGI("[Dump][ExceptionInfo] Start to search static op info and to dump.");
  bool is_dump_exception_from_static_graph = false;
  for (const auto &iter : davinci_model) {
    if (iter != nullptr) {
      if (iter->DumpExceptionInfo(exception_infos) == SUCCESS) {
        is_dump_exception_from_static_graph = true;
      }
    }
  }
  if ((!is_dump_exception_from_dynamic_graph) && (!is_dump_exception_from_static_graph)) {
    GELOGE(FAILED, "[Dump][ExceptionInfo] Dump op exception info failed.");
    return FAILED;
  }
  return SUCCESS;
}

bool GraphExecutionContext::IsDumpEnabled() const {
  return dump_properties.IsDumpOpen() || ModelManager::GetInstance().IsDumpExceptionOpen() ||
         dump_properties.IsOpDebugOpen();
}
}  // namespace hybrid
}  // namespace ge