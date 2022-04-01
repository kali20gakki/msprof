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

#include "hybrid/node_executor/rts/rts_node_executor.h"
#include "hybrid/node_executor/rts/rts_task_factory.h"

#include "common/plugin/ge_util.h"
#include "graph/utils/tensor_utils.h"
#include "hybrid/model/hybrid_model.h"
#include "framework/omg/omg_inner_types.h"
#include "framework/common/types.h"

namespace {
constexpr uint64_t kProfilingIterStartLogid = 5UL;
constexpr uint64_t kProfilingIterEndLogid = 4UL;
constexpr uint64_t kProfilingArStartLogid = 10000UL;
constexpr uint64_t kProfilingArMaxLogid = 19999UL;
} // namespace
namespace ge {
namespace hybrid {
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::RTS, RtsNodeExecutor);

REGISTER_RTS_TASK_CREATOR(IDENTITY, IdentityNodeTask);
REGISTER_RTS_TASK_CREATOR(IDENTITYN, IdentityNNodeTask);
REGISTER_RTS_TASK_CREATOR(READVARIABLEOP, ReadVariableOpNodeTask);
REGISTER_RTS_TASK_CREATOR(PROFILINGTRAININGTRACE, ProfilingTraceNodeTask);
REGISTER_RTS_TASK_CREATOR(STARTOFSEQUENCE, StartOfSequenceTask);
REGISTER_RTS_TASK_CREATOR(MEMCPYASYNC, IdentityNodeTask);

Status IdentityNodeTask::DoCopyTensor(const TaskContext &context, const int32_t index) {
  GE_CHK_STATUS_RET_NOLOG(context.AllocateOutputs());
  const auto input_desc = context.MutableInputDesc(index);
  GE_CHECK_NOTNULL(input_desc);
  int64_t copy_size = 0;
  GE_CHK_GRAPH_STATUS_RET(TensorUtils::GetTensorSizeInBytes(*input_desc, copy_size));
  // copy_size would not be negative since GetTensorSizeInBytes returned successfully.
  if (copy_size != 0) {
    GELOGD("[%s] index = %d, copy size = %ld", context.GetNodeName(), index, copy_size);
    const auto input = context.MutableInput(index);
    const auto output = context.MutableOutput(index);
    GE_CHECK_NOTNULL(input);
    GE_CHECK_NOTNULL(output);
    GE_CHK_RT_RET(rtMemcpyAsync(output->MutableData(),
                                output->GetSize(),
                                input->GetData(),
                                static_cast<uint64_t>(copy_size),
                                RT_MEMCPY_DEVICE_TO_DEVICE,
                                context.GetStream()));
  } else {
    GELOGW("[%s] index = %d, copy size = 0", context.GetNodeName(), index);
  }

  return SUCCESS;
}

Status ReadVariableOpNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  for (int32_t i = 0; i < context.NumInputs(); ++i) {
    GE_CHK_STATUS_RET(DoCopyTensor(context, i));
  }

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status IdentityNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  GE_CHK_STATUS_RET(DoCopyTensor(context, 0));

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status IdentityNNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  for (int32_t i = 0; i < context.NumInputs(); ++i) {
    GE_CHK_STATUS_RET(DoCopyTensor(context, i));
  }

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status ProfilingTraceNodeTask::Init(const HybridModel &model, const NodePtr &node) {
  const auto *const task_defs = model.GetTaskDefs(node);
  if ((task_defs == nullptr) || task_defs->empty()) {
    GELOGE(INTERNAL_ERROR, "[Check][Param] Profiling node has no task to execute.");
    return INTERNAL_ERROR;
  }
  GE_CHECK_NOTNULL(model.GetRootGraph());
  model_id_ = model.GetModelId();
  GELOGD("The model is online:%d, model id is %u",
         static_cast<int32_t>(domi::GetContext().is_online_model), model_id_);
  task_defs_ = *task_defs;
  GELOGD("[%s] Done initialization successfully.", node->GetName().c_str());
  return SUCCESS;
}

Status ProfilingTraceNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  (void)done_callback;
  uint64_t index_id = 1UL;
  for (const auto &task_def : task_defs_) {
    const auto log_time_stamp_def = task_def.log_timestamp();
    const uint64_t log_time_stamp_id = log_time_stamp_def.logid();

    GELOGD("ProfilingTraceTask execute async start. logid = %lu", log_time_stamp_id);

    if (((log_time_stamp_id > kProfilingIterEndLogid) && (log_time_stamp_id < kProfilingArStartLogid)) ||
        (log_time_stamp_id > kProfilingArMaxLogid)) {
      GELOGD("ProfilerTraceNodeTask log id:%lu out of range.", log_time_stamp_id);
      continue;
    }
    const rtError_t rt_ret = rtProfilerTraceEx(index_id, static_cast<uint64_t>(model_id_),
                                               static_cast<uint16_t>(log_time_stamp_id), context.GetStream());
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtProfilerTraceEx failed, ret:0x%X", rt_ret);
      GELOGE(RT_FAILED, "[Call][RtProfilerTraceEx] failed, ret:0x%X", rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
    GELOGD("[%s] ProfilingTraceTask[%lu] execute success.", context.GetNodeName(), log_time_stamp_id);
    index_id += 1UL;
  }
  return SUCCESS;
}

Status StartOfSequenceTask::Init(const HybridModel &model, const NodePtr &node) {
  GE_CHECK_NOTNULL(model.GetRootGraph());
  model_id_ = model.GetModelId();
  GELOGD("The model is online:%d, model id is %u",
         static_cast<int32_t>(domi::GetContext().is_online_model), model_id_);
  GELOGD("[%s] Done initialization successfully.", node->GetName().c_str());
  return SUCCESS;
}

Status StartOfSequenceTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  (void)done_callback;
  const uint64_t index_id = 1UL;
  GELOGD("StartOfSequenceTask execute async start. logid = %lu", kProfilingIterStartLogid);

  const rtError_t rt_ret = rtProfilerTraceEx(index_id, static_cast<uint64_t>(model_id_), kProfilingIterStartLogid,
                                             context.GetStream());
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtProfilerTraceEx failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][RtProfilerTraceEx] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  GELOGD("[%s] StartOfSequenceTask[%lu] execute success.", context.GetNodeName(), kProfilingIterStartLogid);
  return SUCCESS;
}

Status RtsNodeExecutor::PrepareTask(NodeTask &task, TaskContext &context) const {
  return task.UpdateArgs(context);
}

Status RtsNodeExecutor::LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const {
  GE_CHECK_NOTNULL(node);
  GELOGD("[%s] Load for local task.", node->GetName().c_str());
  const std::string node_type = NodeUtils::GetNodeType(node);
  const RtsNodeTaskPtr rts_task = RtsTaskFactory::GetInstance().Create(node_type);
  if (rts_task == nullptr) {
    REPORT_CALL_ERROR("E19999", "[%s] Unsupported RTS op type:%s", node->GetName().c_str(), node_type.c_str());
    GELOGE(UNSUPPORTED, "[Create][Task] failed, as [%s] Unsupported RTS op type:%s",
           node->GetName().c_str(), node_type.c_str());
    return UNSUPPORTED;
  }

  task = rts_task;
  return rts_task->Init(model, node);
}
}  // namespace hybrid
}  // namespace ge

