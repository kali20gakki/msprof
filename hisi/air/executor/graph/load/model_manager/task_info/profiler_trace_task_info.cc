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

#include "graph/load/model_manager/task_info/profiler_trace_task_info.h"

#include "graph/load/model_manager/davinci_model.h"

namespace {
const uint64_t kProfilingIterStartLogid_5 = 5U;
const uint64_t kProfilingArStartLogid_10000 = 10000U;
const uint64_t kProfilingArMaxLogid_19999 = 19999U;
} // namespace

namespace ge {
Status ProfilerTraceTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("ProfilerTraceTaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model->GetStreamList()));

  model_id_ = davinci_model->GetModelId();
  GELOGD("model id is %u", model_id_);

  const auto &log_time_stamp_def = task_def.log_timestamp();
  log_id_ = log_time_stamp_def.logid();
  notify_ = log_time_stamp_def.notify();
  flat_ = log_time_stamp_def.flat();
  davinci_model_ = davinci_model;

  GELOGI("ProfilerTraceTaskInfo Init Success.");
  return SUCCESS;
}

Status ProfilerTraceTaskInfo::Distribute() {
  GELOGI("ProfilerTraceTaskInfo Distribute Start. logid = %lu. notify = %d.", log_id_, static_cast<int32_t>(notify_));
  if (((log_id_ > kProfilingIterStartLogid_5) && (log_id_ < kProfilingArStartLogid_10000)) ||
    (log_id_ > kProfilingArMaxLogid_19999)) {
    GELOGD("ProfilerTraceTaskInfo logid:%lu is out of range.", log_id_);
    return SUCCESS;
  }
  if ((davinci_model_ != nullptr) && (!davinci_model_->CheckModelNoInputAndOutput())) {
    GELOGD("ProfilerTraceTaskInfo load model with queue no need distribute.");
    return SUCCESS;
  }
  GELOGD("TaskInfo model id is %u.", model_id_);
  const rtError_t rt_ret = rtProfilerTraceEx(1UL, static_cast<uint64_t>(model_id_),
                                             static_cast<uint16_t>(log_id_), stream_);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtProfilerTraceEx failed, ret:0x%X, logid:%lu. notify:%d",
                      rt_ret, log_id_, static_cast<int32_t>(notify_));
    GELOGE(RT_FAILED, "[Call][RtProfilerTraceEx] failed, ret:0x%X, logid:%lu. notify:%d", rt_ret, log_id_,
           static_cast<int32_t>(notify_));
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  GELOGI("ProfilerTraceTaskInfo Distribute Success.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_PROFILER_TRACE, ProfilerTraceTaskInfo);
REGISTER_TASK_INFO(RT_MODEL_TASK_PROFILER_TRACE_EX, ProfilerTraceTaskInfo);
}  // namespace ge

