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

#include "hybrid/executor/hybrid_model_pipeline_executor.h"

#include "common/math/math_util.h"
#include "common/dump/dump_manager.h"
#include "graph/ge_context.h"
#include "graph/runtime_inference_context.h"
#include "graph/load/model_manager/model_manager.h"

namespace ge {
namespace hybrid {
namespace {
constexpr int32_t kNumExecutors = 2;
const int32_t kMinLoopCount = 2;
constexpr int32_t kWaitTimeoutInSec = 600;
}

StageExecutor::StageExecutor(const int32_t id, HybridModel *const model, PipeExecutionConfig *const config,
                             StageSubject *const stage_subject)
    : id_(id), model_(model), pipe_config_(config), stage_subject_(stage_subject) {}

StageExecutor::~StageExecutor() {
  GELOGD("~StageExecutor(), id = %d", id_);
  if (stream_ != nullptr) {
    GE_CHK_RT(rtStreamDestroy(stream_));
    stream_ = nullptr;
  }
  if (hccl_stream_ != nullptr) {
    GE_CHK_RT(rtStreamDestroy(hccl_stream_));
    hccl_stream_ = nullptr;
  }
}

Status StageExecutor::Init() {
  GELOGD("[Executor: %d] Start to init StateExecutor", id_);
  context_.rt_context = pipe_config_->rt_context;
  GE_CHK_STATUS_RET_NOLOG(InitExecutionContext());
  GE_CHK_RT_RET(rtStreamCreate(&stream_, static_cast<int32_t>(RT_STREAM_PRIORITY_DEFAULT)));
  GE_CHK_RT_RET(rtStreamCreate(&hccl_stream_, static_cast<int32_t>(RT_STREAM_PRIORITY_DEFAULT)));
  context_.stream = stream_;
  context_.hccl_stream = hccl_stream_;

  root_graph_executor_ = MakeUnique<SubgraphExecutor>(model_->GetRootGraphItem(), &context_);
  GE_CHECK_NOTNULL(root_graph_executor_);
  GE_CHK_STATUS_RET(root_graph_executor_->Init(), "[Init][RootGraphExecutor]Failed.");
  GELOGD("[Executor: %d] Init stage executor successfully", id_);
  return SUCCESS;
}

Status StageExecutor::ResetExecutionContext(GraphExecutionContext &context) {
  GE_CHK_STATUS_RET_NOLOG(context.callback_manager->Init());
  const std::string ctx_id = std::to_string(context.context_id);
  context.runtime_context_.Release();
  for (auto &host_tensor : context.model->GetHostTensors()) {
    const auto node_id = host_tensor.first;
    for (const auto &output_idx_and_tensor : host_tensor.second) {
      const auto output_idx = output_idx_and_tensor.first;
      GELOGD("Preload const host tensor, node_id = %ld, output id = %d", node_id, output_idx);
      (void)context.runtime_context_.SetTensor(node_id, output_idx, output_idx_and_tensor.second);
    }
  }
  return SUCCESS;
}

Status StageExecutor::Start(const std::vector<TensorValue> &inputs, const std::vector<ConstGeTensorDescPtr> &input_desc,
                            const int32_t iteration_count) {
  GELOGD("Start");
  GE_CHK_RT_RET(rtCtxSetCurrent(context_.rt_context));
  int32_t num_loops = iteration_count / pipe_config_->num_executors;
  if (id_ < (iteration_count % iteration_count)) {
    num_loops += 1;
  }
  FMK_INT32_MULCHECK(num_loops, pipe_config_->num_stages);
  num_loops *= pipe_config_->num_stages;
  GELOGD("[Executor: %d] loop count = %d", id_, num_loops);

  for (int32_t loop_idx = 0; loop_idx < num_loops; ++loop_idx) {
    GELOGD("[Executor: %d] Start to wait for task.", id_);
    StageTask task_info;
    (void)task_queue_.Pop(task_info);
    GELOGD("[Executor: %d] Got task, stage = %d, iteration = %ld", id_, task_info.stage, task_info.iteration);
    if (task_info.iteration >= pipe_config_->iteration_end) {
      GELOGE(INTERNAL_ERROR, "[Check][Range][Executor: %d] Unexpected iteration: %ld.", id_, task_info.iteration);
      REPORT_INNER_ERROR("E19999", "[Executor: %d] Unexpected iteration: %ld.", id_, task_info.iteration);
      return INTERNAL_ERROR;
    }

    if (task_info.event != nullptr) {
      GELOGD("[%d] Add StreamWaitEvent", id_);
      GE_CHK_RT_RET(rtStreamWaitEvent(stream_, task_info.event));
      RECORD_MODEL_EXECUTION_EVENT(&context_, "[iteration = %ld] [Stage = %d] EventWait End", task_info.iteration,
                                   task_info.stage);
    }

    RECORD_MODEL_EXECUTION_EVENT(&context_, "[iteration = %ld] [Stage = %d] Start", task_info.iteration,
                                 task_info.stage);

    if (task_info.stage == 0) {
      GELOGD("[Executor: %d] To ResetExecutionContext", id_);
      GE_CHK_STATUS_RET(ResetExecutionContext(context_),
                        "[Invoke][ResetExecutionContext][Executor: %d] Failed to reset context", id_);
      context_.iteration = task_info.iteration;
      GE_CHK_STATUS_RET_NOLOG(SetInputs(inputs, input_desc));
    }

    RECORD_MODEL_EXECUTION_EVENT(&context_, "[Stage = %d] PartialExecuteAsync Start", task_info.stage);
    GE_CHK_STATUS_RET(root_graph_executor_->PartialExecuteAsync(task_info.stage));
    RECORD_MODEL_EXECUTION_EVENT(&context_, "[Stage = %d] PartialExecuteAsync End", task_info.stage);
    GELOGD("[Executor: %d] PartialExecuteAsync successfully.", id_);

    // notify next execution unit
    StageTask next_task;
    next_task.stage = task_info.stage;
    next_task.iteration = task_info.iteration + 1;
    if (((task_info.iteration + 1) % iteration_count) > 0) {
      GE_CHK_RT_RET(rtEventCreate(&next_task.event));
      GE_CHK_RT_RET(rtEventRecord(next_task.event, context_.hccl_stream));
    }

    const auto sync_result = Synchronize();
    if (sync_result != SUCCESS) {
      GELOGE(sync_result,
             "[Invoke][Synchronize][Executor: %d] Failed to sync result:%d. iteration = %ld",
             id_, sync_result, task_info.iteration);
      REPORT_CALL_ERROR("E19999", "[Executor: %d] Failed to sync result:%d. iteration = %ld",
                        id_, sync_result, task_info.iteration);
      if (context_.profiler != nullptr) {
        context_.profiler->Dump(std::cout);
      }
      (void)context_.callback_manager->Destroy();
      context_.runtime_context_.Release();
      return sync_result;
    }
    stage_subject_->Release(task_info.stage);
    if (task_info.event != nullptr) {
      GE_CHK_RT_RET(rtEventDestroy(task_info.event));
      RECORD_MODEL_EXECUTION_EVENT(&context_, "[iteration = %ld] [Stage = %d] EventDestroy End", task_info.iteration,
                                   task_info.stage);
    }

    RECORD_MODEL_EXECUTION_EVENT(&context_, "[iteration = %ld] [Stage = %d] End", task_info.iteration, task_info.stage);

    // if end stage
    if (task_info.stage >= (pipe_config_->num_stages - 1)) {
      RECORD_MODEL_EXECUTION_EVENT(&context_, "[iteration = %ld] Schedule End",
                                   task_info.iteration);
      GELOGD("[Executor: %d] End of iteration [%ld]", id_, task_info.iteration);
      (void)context_.callback_manager->Destroy();
      context_.runtime_context_.Release();
    } else {
      if (model_->GetRootGraphItem()->IsDynamic()) {
        GE_CHK_STATUS_RET_NOLOG(stage_subject_->Await(task_info.stage + 1));
        auto& stage_cache = model_->GetRootGraphItem()->GetStageCache();
        GE_CHK_STATUS_RET_NOLOG(stage_cache.DoPropagate(task_info.stage + 1));
      }
    }
    GE_CHK_STATUS_RET_NOLOG(next_executor_->ExecuteAsync(next_task));
    GELOGD("[Executor: %d] Push item successfully.", id_);
  }

  GELOGD("[Executor: %d] Process task ended.", id_);
  return SUCCESS;
}

Status StageExecutor::ExecuteAsync(const StageTask &args) {
  (void)task_queue_.Push(args);
  return SUCCESS;
}

Status StageExecutor::Synchronize() {
  const auto ret = root_graph_executor_->Synchronize();
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[Synchronize] End, ret = %u", ret);
  return ret;
}

StageSubject::Cond &StageSubject::GetSubject(const int32_t stage) {
  const std::lock_guard<std::mutex> lk(mu_);
  return subjects_[stage];
}

Status StageSubject::Await(const int32_t stage) {
  GELOGD("Stage %d await start.", stage);
  const Status ret = GetSubject(stage).Await();
  GELOGD("Stage %d await ended.", stage);
  return ret;
}

void StageSubject::Release(const int32_t stage) {
  GetSubject(stage).Release();
}

Status StageSubject::Cond::Await() {
  std::unique_lock<std::mutex> lk(cond_mu_);
  if ((!first_exe_) && (!cv_.wait_for(lk,
                                      std::chrono::seconds(kWaitTimeoutInSec),
                                      [this]() { return is_released_; }))) {
    GELOGE(INTERNAL_ERROR, "[Invoke][wait_for]Wait timed out.");
    REPORT_INNER_ERROR("E19999", "wait timed out[%d].", kWaitTimeoutInSec);
    return INTERNAL_ERROR;
  }
  first_exe_ = false;
  is_released_ = false;
  return SUCCESS;
}

void StageSubject::Cond::Release() {
  const std::unique_lock<std::mutex> lk(cond_mu_);
  is_released_ = true;
  cv_.notify_all();
}

HybridModelPipelineExecutor::HybridModelPipelineExecutor(HybridModel *const model, const uint32_t device_id)
    : model_(model), device_id_(device_id) {
  config_.num_executors = kNumExecutors;
  config_.num_stages = static_cast<int32_t>(model_->GetRootGraphItem()->NumGroups());
  config_.device_id = device_id_;
  config_.iteration_end = 0;
}

Status StageExecutor::InitExecutionContext() {
  GE_CHK_RT_RET(rtCtxSetCurrent(context_.rt_context));

  context_.model = model_;
  context_.session_id = ::ge::GetContext().SessionId();
  GELOGD("session id from model = %lu, from context = %lu", model_->GetSessionId(), context_.session_id);
  context_.allocator = NpuMemoryAllocator::GetAllocator(pipe_config_->device_id, stream_);
  GE_CHECK_NOTNULL(context_.allocator);
  context_.callback_manager = new (std::nothrow) RtCallbackManager();
  GE_CHECK_NOTNULL(context_.callback_manager);
  context_.own_callback_manager = true;
  context_.dump_properties = DumpManager::GetInstance().GetDumpProperties(context_.session_id);
  context_.is_eos_ = false;
  if (IsLogEnable(GE_MODULE_NAME, DLOG_DEBUG)) {
    context_.trace_enabled = true;
  }
  return SUCCESS;
}

Status StageExecutor::SetInputs(const std::vector<TensorValue> &inputs,
                                const std::vector<ConstGeTensorDescPtr> &input_desc) const {
  root_graph_executor_->Reset();
  (void)root_graph_executor_->InitInputs(inputs, input_desc);
  return SUCCESS;
}

Status StageExecutor::GetOutputs(std::vector<TensorValue> &outputs,
                                 std::vector<ConstGeTensorDescPtr> &output_desc) const {
  return root_graph_executor_->GetOutputs(outputs, output_desc);
}

void StageExecutor::Reset() {
  task_queue_.Stop();
  task_queue_.Clear();
  task_queue_.Restart();
}

Status HybridModelPipelineExecutor::Init() {
  GE_CHK_STATUS_RET_NOLOG(context_.InitProfiler());
  GELOGD("Number of stages = %d, number of executors = %d", config_.num_stages, config_.num_executors);
  GE_CHK_RT_RET(rtCtxGetCurrent(&config_.rt_context));
  GE_CHK_STATUS_RET_NOLOG(InitStageExecutors());
  return SUCCESS;
}

Status HybridModelPipelineExecutor::InitStageExecutors() {
  for (int32_t i = 0; i < config_.num_executors; ++i) {
    auto stage_executor = MakeUnique<StageExecutor>(i, model_, &config_, &stage_subject_);
    GE_CHECK_NOTNULL(stage_executor);
    GE_CHK_STATUS_RET_NOLOG(stage_executor->Init());

    if (context_.profiler != nullptr) {
      // will call unique_ptr::release later
      stage_executor->context_.profiler.reset(context_.profiler.get());
    }

    stage_executors_.emplace_back(std::move(stage_executor));
  }

  // build propagation loop
  for (int32_t i = 0; i < (config_.num_executors - 1); ++i) {
    const int32_t index = i + 1;
    stage_executors_[static_cast<size_t>(i)]->SetNext(stage_executors_[static_cast<size_t>(index)].get());
  }
  const int32_t executor_index = config_.num_executors - 1;
  stage_executors_[static_cast<size_t>(executor_index)]->SetNext(stage_executors_[0U].get());
  return SUCCESS;
}

Status HybridModelPipelineExecutor::Execute(HybridModelExecutor::ExecuteArgs &args) {
  const int32_t loop_count = args.num_loops;
  GE_CHECK_GE(loop_count, kMinLoopCount);

  auto &inputs = args.inputs;
  auto &input_desc = args.input_desc;
  // Start schedulers
  std::vector<std::future<Status>> futures;
  for (size_t i = 0U; i < stage_executors_.size(); ++i) {
    GELOGD("Starting executor %zu", i);
    const auto executor = stage_executors_[i].get();
    executor->Reset();
    auto future = std::async([loop_count, executor, inputs,
                              input_desc](const struct error_message::Context &error_context) {
      ErrorManager::GetInstance().SetErrorContext(error_context);
      return executor->Start(inputs, input_desc, loop_count);
    }, ErrorManager::GetInstance().GetErrorManagerContext());

    futures.emplace_back(std::move(future));
  }

  // Push initial tasks
  GELOGD("Start to execute with loops, loop count = %d", loop_count);
  config_.iteration_end = iteration_ + loop_count;
  for (int32_t i = 0; i < config_.num_stages; ++i) {
    StageExecutor::StageTask task_info;
    task_info.stage = i;
    task_info.iteration = iteration_;
    (void)stage_executors_[0U]->ExecuteAsync(task_info);
  }

  // Wait for end of iterations
  bool has_error = false;
  for (size_t i = 0U; i < stage_executors_.size(); ++i) {
    GELOGD("Start to sync result of executor[%zu]", i);
    auto ret = futures[i].get();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Check][Result][Executor: %zu] Failed to schedule tasks.", i);
      REPORT_INNER_ERROR("E19999", "[Executor: %zu] Failed to schedule tasks.", i);
      has_error = true;
      continue;
    }

    ret = stage_executors_[i]->Synchronize();

    if (ret != SUCCESS) {
      const auto exception_infos = ModelManager::GetInstance().GetExceptionInfos();
      if (!exception_infos.empty()) {
        HYBRID_CHK_STATUS_RET(context_.DumpExceptionInfo(exception_infos),
                              "[Execute][GraphInternal] Dump exception info failed.");
      }
      GELOGE(ret, "[Invoke][Synchronize] failed for [Executor: %zu].", i);
      REPORT_CALL_ERROR("E19999", "[Executor: %zu] failed to Synchronize result.", i);
      has_error = true;
      continue;
    }
  }

  // record for profiling analyzer
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[Cleanup] End");

  if (context_.profiler != nullptr) {
    context_.profiler->Dump(std::cout);
  }

  iteration_ = config_.iteration_end;

  if (has_error) {
    GELOGE(FAILED, "[Check][Error]Error occurred while execution.");
    REPORT_INNER_ERROR("E19999", "Error occurred while execution.");
    return FAILED;
  }

  const auto last_iter_executor_idx = static_cast<size_t>(loop_count) % stage_executors_.size();
  GE_CHK_STATUS_RET(stage_executors_[last_iter_executor_idx]->GetOutputs(args.outputs, args.output_desc),
                    "[Get][Outputs]Failed from executor[%zu]", last_iter_executor_idx);
  return SUCCESS;
}

HybridModelPipelineExecutor::~HybridModelPipelineExecutor() {
  GELOGD("~HybridModelPipelineExecutor()");
  for (auto &executor : stage_executors_) {
    (void)executor->context_.profiler.release();
  }
}
}  // namespace hybrid
}  // namespace ge
