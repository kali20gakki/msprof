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

#include "executor_manager.h"
#include <unistd.h>
#include <vector>
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "mmpa/mmpa_api.h"
#include "runtime/rt_mem_queue.h"
#include "common/utils/bind_cpu_utils.h"
#include "common/utils/rts_api_utils.h"
#include "common/mem_grp/memory_group_manager.h"
#include "common/data_flow/queue/helper_exchange_service.h"
namespace ge {
namespace {
constexpr uint32_t kEventGroupId = 1;
constexpr int32_t kDefaultTimeout = 10 * 1000;
constexpr size_t kMaxArgsSize = 128UL;
}
ExecutorProcess::ExecutorProcess(int32_t pid, int32_t device_id, uint32_t msg_queue_id, std::string group_name)
    : pid_(pid), device_id_(device_id), msg_queue_id_(msg_queue_id), group_name_(std::move(group_name)) {
}

Status ExecutorProcess::Initialize() {
  GE_CHK_STATUS_RET_NOLOG(InitEventGroup());
  GE_CHK_STATUS_RET_NOLOG(InitMessageQueue());
  GE_CHK_STATUS_RET_NOLOG(WaitForExecutorInitialized());
  GELOGD("[Initialize] succeeded");
  return SUCCESS;
}

Status ExecutorProcess::InitEventGroup() {
  static std::once_flag flag;
  std::call_once(flag, [this]() {
    GE_CHK_STATUS_RET(RtsApiUtils::EschedCreateGroup(0, event_group_id_, RT_GRP_TYPE_BIND_CP_CPU),
                      "[Create][EschedGroup] failed, device_id = 0");
    GELOGD("[Create][EschedGroup] succeeded, device_id = 0, group_id = %u", event_group_id_);
  });
  return SUCCESS;
}

Status ExecutorProcess::Finalize() {
  if (pid_ == 0) {
    return SUCCESS;
  }

  if (SendEvent(ExecutorEventType::kFinalize) != SUCCESS) {
    GELOGW("Failed to send finalize command to executor, pid = %d", pid_);
    GELOGI("Send SIGTERM signal to executor, pid = %d", pid_);
    if (kill(pid_, SIGTERM) != 0) {
      GELOGE(FAILED, "Fail to kill queue schedule, pid=%d, error=%s.", pid_, strerror(errno));
      return FAILED;
    }
  }
  (void) HelperExchangeService::GetInstance().DestroyQueue(device_id_, msg_queue_id_);
  msg_queue_id_ = UINT32_MAX;
  pid_ = 0;
  return SUCCESS;
}

int32_t ExecutorProcess::GetPid() const {
  return pid_;
}

Status ExecutorManager::GetCfgArgs(const char_t **fixed_argv, size_t fix_size,
                                   const std::vector<std::string> &args_strings, const char_t **var_args) {
  if ((fix_size + args_strings.size()) >= kMaxArgsSize) {
    GELOGE(FAILED, "too many args size, fix_size=%zu, var_args_size=%zu, fix_size + var_args_size can not above %zu.",
           fix_size, args_strings.size(), kMaxArgsSize);
    return FAILED;
  }
  // variable args
  size_t id = 0;
  for (; id < fix_size; id++) {
    var_args[id] = fixed_argv[id];
  }
  for (size_t var_id = 0UL; var_id < args_strings.size(); var_id++) {
    var_args[id + var_id] = args_strings[var_id].c_str();
  }
  for (size_t i = 0; i < (fix_size + args_strings.size()); i++) {
    GELOGI("Set configure arg[%zu]=%s.", i, var_args[i]);
  }
  return SUCCESS;
}

Status ExecutorManager::LoadAndExecuteChildProcess(int32_t device_id,
                                                   uint32_t msg_queue_id,
                                                   const std::string &group_name) {
  auto device_id_str = std::to_string(device_id);
  auto msg_queue_id_str = std::to_string(msg_queue_id);
  auto event_group_id = std::to_string(kEventGroupId);
  std::string bin_dir;
  std::string bin_name;
  if (is_host_) {
    bin_dir = "/usr/local/Ascend/runtime/bin/";
    bin_name = "host_cpu_executor_main";
  } else {
    bin_dir = "/usr/local/Ascend/modeldeployer/bin/";
    bin_name = "npu_executor_main";
  }
  auto bin_path = bin_dir + bin_name;
  const std::string enable = "1";
  const int32_t override = 1;
  // get options
  std::map<std::string, std::string> env_option;
  std::map<std::string, std::string> args_option;
  if (dev_maintenance_cfg_ != nullptr) {
    GE_CHK_STATUS_RET(dev_maintenance_cfg_->DecodeConfig(env_option, args_option), "Decode config failed.");
  }
  DevMaintenanceCfgUtils::SetEnvByOption(env_option);
  (void) mmSetEnv("AICPU_ADD_BUFFERGROUP", enable.c_str(), override);
  // fixed args
  const char_t *argv[] = {
      bin_name.c_str(),
      group_name.c_str(),
      msg_queue_id_str.c_str(),
      device_id_str.c_str(),
      event_group_id.c_str(),
      nullptr
  };
  const auto fix_size = sizeof(argv) / sizeof(char *) - 1;
  // variable args
  const char_t *var_args[kMaxArgsSize] = {nullptr};
  std::vector<std::string> args_strings = DevMaintenanceCfgUtils::TransArgsOption2Array(args_option);
  GE_CHK_STATUS_RET_NOLOG(GetCfgArgs(argv, fix_size, args_strings, var_args));
  auto res = ::execv(bin_path.c_str(), const_cast<char_t **>(var_args));
  if (res < 0) {
    GELOGE(FAILED, "[Execute] npu executor failed, path = %s, err_msg = %s", bin_path.c_str(), strerror(errno));
    return FAILED;
  }
  return SUCCESS;
}

Status ExecutorProcess::InitMessageQueue() {
  GE_CHK_STATUS_RET(MemoryGroupManager::GetInstance().MemGrpAddProc(group_name_, pid_, false, true),
                    "Failed to add group, pid = %d", pid_);
  rtMemQueueShareAttr_t c_queue_attr{};
  c_queue_attr.read = 1;
  GE_CHK_RT_RET(rtMemQueueGrant(device_id_, msg_queue_id_, pid_, &c_queue_attr));
  return SUCCESS;
}

Status ExecutorProcess::WaitEvent(ExecutorEventType expected_event_type) const {
  GE_CHK_STATUS_RET_NOLOG(BindCpuUtils::BindCore(0));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::EschedAttachDevice(0));
  uint64_t event_bitmap = 1ULL << static_cast<uint32_t>(RT_EVENT_QUEUE_ENQUEUE);
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubscribeEvent(0, event_group_id_, device_id_, event_bitmap));
  GELOGD("[Subscribe][Event] deivce[0] group[%u] thread[%d] success", event_group_id_, device_id_);
  uint32_t retry_cnt = 30;
  while (retry_cnt > 0) {
    // grpc server always wait event on device 0
    GELOGD("[Wait][Event] start, device_id = 0, group_id = %u, thread_id = %d", event_group_id_, device_id_);
    rtEschedEventSummary_t event_info;
    auto ret = rtEschedWaitEvent(0, event_group_id_, device_id_, kDefaultTimeout, &event_info);
    if (ret == RT_ERROR_NONE) {
      GELOGD("[Wait][Event] success, event_id = %d, sub_event_id = %u", event_info.eventId, event_info.subeventId);
      auto event_type = static_cast<ExecutorEventType>(event_info.subeventId);
      if (event_type != expected_event_type) {
        GELOGE(FAILED,
               "expect event type = %d, but got = %u, err_msg = %s",
               static_cast<int32_t>(expected_event_type),
               event_info.subeventId,
               event_info.msgLen > 0U ? event_info.msg : "");
        return FAILED;
      }
      return SUCCESS;
    }

    --retry_cnt;
    GELOGD("[Wait][Event] timeout, retry_count = %d", retry_cnt);
  }
  GELOGE(FAILED, "Wait timeout, device_id = 0, group_id = %u, thread_id = %d", event_group_id_, device_id_);
  return FAILED;
}

Status ExecutorProcess::WaitForExecutorInitialized() {
  GE_CHK_STATUS_RET(WaitEvent(ExecutorEventType::kSuccess),
                    "Failed to wait for executor process initialize.");
  return SUCCESS;
}

Status ExecutorProcess::SendRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &resp) const {
  auto req_size = request.ByteSizeLong();
  ExchangeService::FillFunc fill_func = [&request, &resp](void *buffer, size_t size) {
    if (request.SerializeToArray(buffer, static_cast<int32_t>(size))) {
      return SUCCESS;
    }
    resp.set_status_code(FAILED);
    resp.set_error_message("SerializeToArray failed");
    GELOGE(FAILED, "SerializeToArray failed");
    return FAILED;
  };

  GE_CHK_STATUS_RET(HelperExchangeService::GetInstance().Enqueue(device_id_, msg_queue_id_, req_size, fill_func),
                    "Failed to enqueue request");
  GELOGD("[Enqueue][Request] succeeded");
  GE_CHK_STATUS_RET_NOLOG(SendEvent(ExecutorEventType::kRequest));
  GELOGD("[Send][Event] succeeded");
  GE_CHK_STATUS_RET_NOLOG(WaitEvent(ExecutorEventType::kSuccess));
  GELOGD("[Wait][Event] succeeded");
  return SUCCESS;
}

Status ExecutorProcess::SendEvent(ExecutorEventType event_type) const {
  rtEschedEventSummary_t event_info;
  event_info.eventId = RT_EVENT_QUEUE_ENQUEUE;
  event_info.subeventId = static_cast<uint32_t>(event_type);
  event_info.pid = pid_;
  event_info.grpId = event_group_id_;
  event_info.msg = nullptr;
  event_info.msgLen = 0;
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubmitEvent(device_id_, event_info),
                    "[Submit][Event] failed, device_id = %d, group = %u, pid = %d",
                    device_id_, event_group_id_, pid_);
  GELOGD("[Submit][Event] succeeded, device_id = %d, group = %u, pid = %d",
         device_id_, event_group_id_, pid_);
  return SUCCESS;
}

ExecutorProcess::~ExecutorProcess() {
  (void) Finalize();
}

Status ExecutorManager::ForAndInit(int32_t device_id, std::unique_ptr<ExecutorProcess> &executor_process) {
  // 1. Create msg queue
  uint32_t queue_depth = 3;
  uint32_t msg_queue_id = UINT32_MAX;
  std::string msg_queue_name = "queue.executor_" + std::to_string(device_id);
  GE_CHK_STATUS_RET(HelperExchangeService::GetInstance().CreateQueue(device_id,
                                                                     msg_queue_name,
                                                                     queue_depth,
                                                                     RT_MQ_MODE_PUSH,
                                                                     msg_queue_id),
                    "[Create][MsgQueue] failed");
  GE_DISMISSABLE_GUARD(msg_queue, ([device_id, msg_queue_id]() {
    (void) HelperExchangeService::GetInstance().DestroyQueue(device_id, msg_queue_id);
  }));

  // 2. For child executor process
  const auto group_name = MemoryGroupManager::GetInstance().GetQsMemGroupName();
  auto pid = fork();
  if (pid < 0) {
    GELOGE(FAILED, "[Fork][Process] failed");
    return FAILED;
  }

  if (pid == 0) {
    // In child process
    return LoadAndExecuteChildProcess(device_id, msg_queue_id, group_name);
  }

  // In parent process
  GELOGD("[Fork][Process] succeeded, pid = %d", pid);
  executor_process.reset(new(std::nothrow)ExecutorProcess(pid, device_id, msg_queue_id, group_name));
  GE_CHECK_NOTNULL(executor_process);
  GE_DISMISS_GUARD(msg_queue);  // ownership transferred to executor_process
  GE_CHK_STATUS_RET(executor_process->Initialize(), "Failed to initialize executor process");
  return SUCCESS;
}

Status ExecutorManager::GetOrForkExecutorProcess(const ExecutorId &context, ExecutorProcess **executor_process) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = processes_.find(context);
  if (it != processes_.end()) {
    *executor_process = it->second.get();
    return SUCCESS;
  }

  GELOGD("Executor process not exist, start to create, context_id = %ld, device id = %d",
         context.context_id, context.device_id);
  std::unique_ptr<ExecutorProcess> process;
  GE_CHK_STATUS_RET(ForAndInit(context.device_id, process),
                    "Failed to fork and execute executor process, context_id = %ld, context.client_id, device id = %d",
                    context.context_id, context.device_id);
  GE_CHECK_NOTNULL(process);
  GELOGD("Executor process started successfully, context_id = %ld, device id = %d",
         context.context_id, context.device_id);
  processes_[context] = std::move(process);
  *executor_process = processes_[context].get();
  return SUCCESS;
}

Status ExecutorManager::GetExecutorProcess(const ExecutorId &context,
                                           ExecutorProcess **executor_process) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = processes_.find(context);
  if (it != processes_.end()) {
    *executor_process = it->second.get();
    return SUCCESS;
  }

  GELOGE(FAILED, "Executor process not exist, context_id = %ld, device id = %d",
         context.context_id, context.device_id);
  return FAILED;
}

bool ExecutorManager::IsHost() const {
  return is_host_;
}
}  // namespace ge
