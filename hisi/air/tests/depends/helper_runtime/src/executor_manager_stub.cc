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

#include "deploy/execfwk/executor_manager.h"
#include <vector>
#include <sstream>
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "runtime/rt_mem_queue.h"
#define private public
#include "executor/cpu_sched_event_dispatcher.h"
#include "executor/dynamic_model_executor.h"
#include "executor/event_handler.h"
#undef private
#include "executor/executor_context.h"

namespace ge {
namespace {
std::map<const ExecutorProcess *, std::unique_ptr<EventHandler>> g_event_handlers;
std::mutex g_mu;

class MockDynamicModelExecutor : public DynamicModelExecutor {
 public:
  explicit MockDynamicModelExecutor(bool is_host) : DynamicModelExecutor(is_host) {}
  Status DoLoadModel(const shared_ptr<GeRootModel> &root_model) override {
    model_id_ = 0;
    return SUCCESS;
  }

  Status DoExecuteModel(const RunModelData &inputs, RunModelData &outputs) override {
    DataBuffer &data_buffer = outputs.blobs[0];
    if (data_buffer.data == nullptr) {
      data_buffer.data = output_buffer_;
    }
    data_buffer.length = 8;
    auto output_values = reinterpret_cast<int32_t *>(data_buffer.data);
    output_values[0] = 222;
    output_values[1] = 666;
    GeTensorDesc tensor_desc;
    tensor_desc.SetShape(GeShape({2}));
    output_tensor_descs_ = {tensor_desc};
    return SUCCESS;
  }

 private:
  uint8_t output_buffer_[8];
};

class ModelHandleMock : public ExecutorContext::ModelHandle {
 public:
  explicit ModelHandleMock(uint64_t model_size) : ModelHandle(model_size) {}
  Status DoLoadModel(const shared_ptr<GeRootModel> &root_model,
                     const vector<uint32_t> &input_queues,
                     const vector<uint32_t> &output_queues) override {
    bool is_dynamic = false;
    root_model->CheckIsUnknownShape(is_dynamic);
    if (is_dynamic) {
      executor_ = MakeUnique<MockDynamicModelExecutor>(true);
      GE_CHK_STATUS_RET(executor_->LoadModelWithQueue(root_model, input_queues, output_queues));

      rtMbufPtr_t input_mbuf = nullptr;
      RuntimeTensorDesc input_runtime_tensor_desc{};
      input_runtime_tensor_desc.shape[0] = 1;
      input_runtime_tensor_desc.shape[1] = 2;
      input_runtime_tensor_desc.original_shape[0] = 1;
      input_runtime_tensor_desc.original_shape[1] = 2;
      input_runtime_tensor_desc.dtype = DT_FLOAT;
      input_runtime_tensor_desc.format = FORMAT_ND;
      rtMbufAlloc(&input_mbuf, sizeof(input_runtime_tensor_desc) + 8);
      void *input_buffer = nullptr;
      rtMbufGetBuffAddr(input_mbuf, &input_buffer);
      memcpy(input_buffer, &input_runtime_tensor_desc, sizeof(input_runtime_tensor_desc));
      AICPUSubEventInfo event_info{};
      event_info.modelId = 0;
      rtEschedEventSummary_t event_summary;
      event_summary.msg = (char *)&event_info;
      event_summary.msgLen = sizeof(event_info);
      executor_->input_mbuf_addresses_[0] = input_mbuf;
      CpuSchedEventDispatcher::GetInstance().OnInputsReady(event_summary);
    }
    GELOGD("Root model: %s, is_dynamic = %d", root_model->GetRootGraph()->GetName().c_str(), is_dynamic);
    return SUCCESS;
  }

  Status DoUnloadModel(const uint32_t model_id) const override {
    if (executor_ != nullptr) {
      rtMbufFree(executor_->input_mbuf_addresses_[0]);
      rtMbufFree(executor_->output_mbuf_addresses_[0]);
      executor_->UnloadModel();
    }
    return SUCCESS;
  }

 private:
  std::unique_ptr<DynamicModelExecutor> executor_;
};

class ExecutionContextMock : public ExecutorContext {
 public:
 protected:
  std::unique_ptr<ModelHandle> CreateModelHandle(uint64_t model_size) const override {
   return MakeUnique<ModelHandleMock>(model_size);
 }
};
}  // namespace

ExecutorProcess::ExecutorProcess(int32_t pid, int32_t device_id, uint32_t msg_queue_id, std::string group_name)
    : pid_(pid), device_id_(device_id), msg_queue_id_(msg_queue_id), group_name_(std::move(group_name)) {
}

ExecutorProcess::~ExecutorProcess() {
  (void) Finalize();
}

Status ExecutorProcess::Initialize() {
  std::lock_guard<std::mutex> lk(g_mu);
  auto &handler = g_event_handlers[this];
  handler.reset(new EventHandler());
  handler->Initialize();
  handler->context_.reset(new ExecutionContextMock());
  return SUCCESS;
}

Status ExecutorManager::LoadAndExecuteChildProcess(int32_t device_id,
                                                   uint32_t msg_queue_id,
                                                   const std::string &group_name) {
  std::map<std::string, std::string> env_option;
  std::map<std::string, std::string> args_option;
  GE_CHECK_NOTNULL(dev_maintenance_cfg_);
  GE_CHK_STATUS_RET(dev_maintenance_cfg_->DecodeConfig(env_option, args_option), "Decode config failed.");
  DevMaintenanceCfgUtils::SetEnvByOption(env_option);
  std::vector<std::string> args_strings = DevMaintenanceCfgUtils::TransArgsOption2Array(args_option);
  return SUCCESS;
}

Status ExecutorProcess::InitMessageQueue() {
  return SUCCESS;
}

Status ExecutorProcess::WaitEvent(ExecutorEventType expected_event_type) const {
  return SUCCESS;
}

Status ExecutorProcess::WaitForExecutorInitialized() {
  return SUCCESS;
}

Status ExecutorProcess::SendRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &resp) const {
  EventHandler *handler = nullptr;
  {
    std::lock_guard<std::mutex> lk(g_mu);
    handler = g_event_handlers[this].get();
  }

  handler->HandleEvent(request, resp);
  return SUCCESS;
}

Status ExecutorProcess::SendEvent(ExecutorEventType event_type) const {
  return SUCCESS;
}

Status ExecutorProcess::Finalize() {
  std::lock_guard<std::mutex> lk(g_mu);
  g_event_handlers.erase(this);
  return SUCCESS;
}

Status ExecutorProcess::InitEventGroup() {
  return SUCCESS;
}


int32_t ExecutorProcess::GetPid() const {
  return pid_;
}

Status ExecutorManager::ForAndInit(const int32_t device_id,
                                   std::unique_ptr<ExecutorProcess> &executor_process) {
  return SUCCESS;
}

Status ExecutorManager::GetOrForkExecutorProcess(const ExecutorId &context, ExecutorProcess **executor_process) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = processes_.find(context);
  if (it != processes_.end()) {
    *executor_process = it->second.get();
    return SUCCESS;
  }

  GELOGD("Executor process not exist, start to create, context_id = %ld", context.context_id);
  std::unique_ptr<ExecutorProcess> process(new ExecutorProcess(0, 0, 0, "DM_QS_GROUP"));
  GE_CHK_STATUS_RET_NOLOG(process->Initialize());
  *executor_process = process.get();
  processes_.emplace(context, std::move(process));
  return LoadAndExecuteChildProcess(context.device_id, 0U, "stub_grp");
}

Status ExecutorManager::GetExecutorProcess(const ExecutorId &context, ExecutorProcess **executor_process) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = processes_.find(context);
  if (it != processes_.end()) {
    *executor_process = it->second.get();
    return SUCCESS;
  }

  GELOGE(FAILED, "Executor process not exist, context_id = %ld", context.context_id);
  return FAILED;
}

bool ExecutorManager::IsHost() const {
  return is_host_;
}
}  // namespace ge
