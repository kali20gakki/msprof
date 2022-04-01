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

#include "executor/engine_daemon.h"
#include "runtime/rt_mem_queue.h"
#include "external/runtime/rt_error_codes.h"
#include "mmpa/mmpa_api.h"
#include "exec_runtime/execution_runtime.h"
#include "proto/deployer.pb.h"
#include "event_handler.h"
#include "executor_event_defs.h"
#include "common/profiling/profiling_manager.h"
#include "common/utils/bind_cpu_utils.h"
#include "common/config/device_debug_config.h"
#include "common/profiling/command_handle.h"
#include "common/ge_inner_attrs.h"
#include "common/string_util.h"
#include "graph/ge_local_context.h"
#include "graph/ge_context.h"
#include "executor/cpu_sched_event_dispatcher.h"
#include "adx_datadump_server.h"

namespace ge {
namespace {
constexpr int32_t kDefaultTimeout = 10 * 1000;  // 10s
constexpr uint32_t kCpuNumsInDevice = 8U;
constexpr int32_t kAdxErrorNone = 0;
const char_t *const kDumpoff = "off";
const char_t *const kProfilingPath = "/var/log/npu/profiling/";
}

EngineDaemon::EngineDaemon(bool is_host_cpu) : is_host_cpu_(is_host_cpu) {}

Status EngineDaemon::InitializeWithArgs(int32_t argc, char **argv) {
  GE_CHK_STATUS_RET_NOLOG(ParseCmdLineArgs(argc, argv));
  GE_CHK_STATUS_RET_NOLOG(InitializeMaintenanceFromOption());
  // each device has 8 cpu core
  GE_CHK_STATUS_RET_NOLOG(BindCpuUtils::BindCore(device_id_ * kCpuNumsInDevice));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MemGrpAttach(mem_group_name_, kDefaultTimeout));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufInit());
  GE_CHK_RT_RET(rtSetAicpuAttr("RunMode", "PROCESS_MODE"));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::EschedAttachDevice(device_id_));
  GE_CHK_STATUS_RET_NOLOG(CpuSchedEventDispatcher::GetInstance().Initialize(device_id_));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::SetDevice(device_id_));
  GE_CHK_STATUS_RET_NOLOG(InitializeGeExecutor());
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MemQueueInit(device_id_));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::EschedAttachDevice(device_id_));
  GE_CHK_STATUS_RET_NOLOG(InitMessageQueue());
  GE_CHK_STATUS_RET_NOLOG(event_handler_.Initialize());
  GE_CHK_STATUS_RET_NOLOG(NotifyInitialized());
  return SUCCESS;
}

void EngineDaemon::Finalize() {
  CpuSchedEventDispatcher::GetInstance().Finalize();
  (void) ge_executor_.Finalize();
  GELOGD("Engine daemon finalized, device id = %d", device_id_);
}

Status EngineDaemon::InitProfilingFromOption(const std::map<std::string, std::string> &options_all) {
  auto options = options_all;
  rtProfCtrlHandle callback = ProfCtrlHandle;
  const rtError_t ret = rtProfRegisterCtrlCallback(GE, callback);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "Register CtrlCallback failed.");
    return FAILED;
  }
  bool is_execute_profiling = true;
  const auto &cfg_data = options[kProfilingDeviceConfigData];
  const auto &is_execute_on = options[kProfilingIsExecuteOn];
  if (cfg_data.empty()) {
    GELOGI("kProfilingDeviceConfigData is not set.");
    is_execute_profiling = false;
  }
  if (is_execute_on.empty() || (is_execute_on == "0")) {
    is_execute_profiling = false;
    GELOGI("kProfilingIsExecuteOn is not enable.");
  }
  if (is_execute_profiling) {
    struct MsprofCommandHandleParams prof_conf = {};
    if (strncpy_s(prof_conf.profData, sizeof(prof_conf.profData), cfg_data.c_str(), cfg_data.length()) != EOK) {
      GELOGE(INTERNAL_ERROR, "[copy][ProfilingConfigOption]Failed, data %s.", cfg_data.c_str());
      REPORT_INNER_ERROR("E19999", "Copy profiling data %s failed.", cfg_data.c_str());
      return INTERNAL_ERROR;
    }
    prof_conf.profDataLen = strlen(prof_conf.profData);
    if (strncpy_s(prof_conf.path, sizeof(prof_conf.profData), kProfilingPath,
                  strlen(kProfilingPath) + 1) != EOK) {
      GELOGE(INTERNAL_ERROR, "[copy][ProfilingConfigOption]Failed, path %s.", kProfilingPath);
      REPORT_INNER_ERROR("E19999", "Copy profiling_options %s failed.", kProfilingPath);
      return INTERNAL_ERROR;
    }
    prof_conf.pathLen = strlen(prof_conf.path);
    prof_conf.storageLimit = UINT32_MAX;
    const auto df_ret = MsprofInit(static_cast<uint32_t>(MSPROF_CTRL_INIT_HELPER), &prof_conf, sizeof(prof_conf));
    GELOGI("Profiling init in helper binary, return %d.", df_ret);
  } else {
    const auto df_ret = MsprofInit(MSPROF_CTRL_INIT_DYNA, nullptr, 0);
    GELOGI("Default profiling init, return %d.", df_ret);
  }
  return SUCCESS;
}

Status EngineDaemon::InitDumpFromOption(std::map<std::string, std::string> &options) {
  DumpConfig dump_cfg;
  args_option_[kExecutorDevId] = std::to_string(device_id_);
  const bool is_enable = DumpManager::GetInstance().GetCfgFromOption(options, dump_cfg);
  const Status ret = ge_executor_.SetDump(dump_cfg);
  if (!is_enable) {
    GELOGI("Dump is not open, do not need to init dump server, ret=%d.", ret);
    return ret;
  }
  const int32_t adx_ret = AdxDataDumpServerInit();
  if (adx_ret != kAdxErrorNone) {
    GELOGE(ge::INTERNAL_ERROR, "[AdxDataDumpServer][Init]dump server run failed, adx result[%d].", adx_ret);
    return ge::INTERNAL_ERROR;
  }
  is_dump_inited_ = true;
  return ret;
}

Status EngineDaemon::InitializeMaintenanceFromOption() {
  GE_CHK_STATUS_RET(InitProfilingFromOption(args_option_), "InitProfilingFromOption failed.");
  GE_CHK_STATUS_RET(InitDumpFromOption(args_option_), "InitProfilingFromOption failed.");
  return SUCCESS;
}

Status EngineDaemon::LoopEvents() {
  GELOGD("Event loop started");
  while (true) {
    ExecutorEventType event_type;
    bool is_timeout = false;
    GE_CHK_STATUS_RET_NOLOG(WaitEvent(event_type, is_timeout));
    if (is_timeout) {
      GELOGD("No event was received, continue");
      continue;
    }

    if (event_type == ExecutorEventType::kHeartbeat) {
      GELOGD("Heartbeat received");
    } else if (event_type == ExecutorEventType::kFinalize) {
      GELOGD("Finalize received");
      GE_CHK_STATUS_RET(FinalizeMaintenance());
      break;
    } else if (event_type == ExecutorEventType::kRequest) {
      GELOGD("Request received, start dequeue");
      GE_CHK_STATUS_RET_NOLOG(HandleEvent());
    } else {
      GELOGE(UNSUPPORTED, "Unsupported event type: %d", static_cast<int32_t>(event_type));
      GE_CHK_STATUS_RET_NOLOG(SendEvent(ExecutorEventType::kFailure, "unsupported event type"));
    }
  }
  return SUCCESS;
}

Status EngineDaemon::FinalizeMaintenance() {
  const auto ret = MsprofFinalize();
  GE_CHK_STATUS_RET(ret, "MsprofFinalize failed, ret=%d.", ret);
  if (is_dump_inited_) {
    const int32_t adx_ret = AdxDataDumpServerUnInit();
    GE_CHK_STATUS_RET(ret, "AdxDataDumpServerUnInit failed, ret=%d.", ret);
    DumpConfig dump_cfg;
    dump_cfg.dump_status = kDumpoff;
    dump_cfg.dump_debug = kDumpoff;
    const Status dump_ret = ge_executor_.SetDump(dump_cfg);
    GE_CHK_STATUS_RET(ret, "Executor set dump failed, ret=%d.", dump_ret);
  }
  is_dump_inited_ = false;
  return SUCCESS;
}

Status EngineDaemon::HandleEvent() {
  void *mbuf = nullptr;
  GE_CHK_RT_RET(rtMemQueueDeQueue(device_id_, msg_queue_id_, &mbuf));
  GELOGD("[Dequeue][Message] success");
  GE_MAKE_GUARD(mbuf, [mbuf]() {
    (void) rtMbufFree(mbuf);
  });
  void *buffer_addr = nullptr;
  uint64_t buffer_size = 0;
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetBufferAddr(mbuf, &buffer_addr));
  GE_CHK_STATUS_RET_NOLOG(RtsApiUtils::MbufGetBufferSize(mbuf, buffer_size));
  GELOGD("[Parse][Message] addr = %p, size = %lu", buffer_addr, buffer_size);
  deployer::ExecutorRequest request;
  if (!request.ParseFromArray(buffer_addr, static_cast<int32_t>(buffer_size))) {
    GELOGE(PARAM_INVALID, "[Parse][Message] failed");
    return FAILED;
  }

  GELOGD("On event: %s", request.DebugString().c_str());
  deployer::ExecutorResponse response;
  event_handler_.HandleEvent(request, response);
  if (response.status_code() == SUCCESS) {
    GELOGD("[Handle][Event] succeeded");
    GE_CHK_STATUS_RET_NOLOG(SendEvent(ExecutorEventType::kSuccess));
  } else {
    GELOGD("[Handle][Event] failed, error_code = %u, error_msg = %s",
           response.status_code(),
           response.error_message().c_str());
    GE_CHK_STATUS_RET_NOLOG(SendEvent(ExecutorEventType::kFailure, response.error_message()));
  }
  return SUCCESS;
}

void EngineDaemon::TransArray2ArgsOption(const int32_t start, const int32_t end, char_t **argv) {
  std::map<std::string, std::string> args_option;
  GELOGI("TransArray2ArgsOption enter, start=%d, end=%d.", start, end);
  for (int32_t var_id = start; var_id < end; var_id++) {
    const std::string str(argv[var_id]);
    constexpr size_t kPairArgsNum = 2UL;
    constexpr size_t kKeyPos = 0UL;
    constexpr size_t kValuePos = 1UL;
    std::vector<std::string> pair = StringUtils::Split(str, '=');
    if (pair.size() != kPairArgsNum) {
      GELOGW("Can not parse args in %s.", argv[var_id]);
      continue;
    }
    args_option.emplace(pair[kKeyPos], pair[kValuePos]);
  }
  args_option_ = std::move(args_option);
}

void EngineDaemon::GetGlobalEnvOptions(std::map<std::string, std::string> &env_option) {
  const std::string kLogLevelEnvName = "ASCEND_GLOBAL_LOG_LEVEL";
  const std::string kLogEventEnableEnvName = "ASCEND_GLOBAL_EVENT_ENABLE";
  const std::string kLogHostFileNumEnvName = "ASCEND_HOST_LOG_FILE_NUM";
  const std::string kLogProcessLogPathEnvName = "ASCEND_PROCESS_LOG_PATH";
  const std::vector<std::string> kLogEnvNames = {kLogLevelEnvName, kLogEventEnableEnvName, kLogHostFileNumEnvName,
                                                 kLogProcessLogPathEnvName};
  for (const auto &log_name : kLogEnvNames) {
    constexpr size_t kMaxClusterEnvStrLen = 1024UL;
    char_t env_value[kMaxClusterEnvStrLen] = {};
    const int32_t ret = mmGetEnv(log_name.c_str(), env_value, kMaxClusterEnvStrLen);
    if (ret != 0) {
      GELOGW("[Check][Env]Get env[%s] failed.", log_name.c_str());
      continue;
    }
    GELOGD("Get env, key=%s, val=%s.", log_name.c_str(), env_value);
    env_option.emplace(log_name, std::string(env_value));
  }
}

Status EngineDaemon::ParseCmdLineArgs(int32_t argc, char **argv) {
  const int32_t kExpectedArgCount = 5;
  if (argc < kExpectedArgCount) {
    GELOGE(PARAM_INVALID, "[Parse][Args] failed, arg count (%d) is invalid", argc);
    return PARAM_INVALID;
  }
  const char *memory_group_name = argv[1];
  GE_CHECK_NOTNULL(memory_group_name);
  mem_group_name_ = std::string(memory_group_name);
  const char *msg_queue_id = argv[2];
  GE_CHECK_NOTNULL(msg_queue_id);
  GE_CHK_STATUS_RET_NOLOG(ToNumber(msg_queue_id, msg_queue_id_));
  const char *device_id = argv[3];
  GE_CHECK_NOTNULL(device_id);
  GE_CHK_STATUS_RET_NOLOG(ToNumber(device_id, device_id_));
  const char *event_group_id = argv[4];
  GE_CHECK_NOTNULL(event_group_id);
  GE_CHK_STATUS_RET_NOLOG(ToNumber(event_group_id, event_group_id_));
  TransArray2ArgsOption(kExpectedArgCount - 1, argc, argv);
  // for test
  std::map<std::string, std::string> env_option;
  GetGlobalEnvOptions(env_option);
  for (const auto &env_it : env_option) {
    const auto &key = env_it.first;
    const auto &val = env_it.second;
    GELOGI("Get global env options, env=%s, env_val=%s.", key.c_str(), val.c_str());
  }
  GELOGD("[Parse][Args] succeeded, get env_option size=%zu, mem_grp_name = %s, msg_queue_id = %s, "
      "device_id = %d, evt_group_id = %u", env_option.size(), mem_group_name_.c_str(), msg_queue_id, device_id_,
      event_group_id_);
  return SUCCESS;
}

Status EngineDaemon::WaitEvent(ExecutorEventType &event_type, bool &is_timeout) {
  rtEschedEventSummary_t in_event;
  auto ret = rtEschedWaitEvent(device_id_, event_group_id_, 0, kDefaultTimeout, &in_event);
  if (ret == ACL_ERROR_RT_REPORT_TIMEOUT) {
    GELOGD("rtEschedWaitEvent timeout, device_id = %d, group_id = %u", device_id_, event_group_id_);
    is_timeout = true;
    return SUCCESS;
  }
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "Failed to invoke rtEschedWaitEvent, device_id = %d, group_id = %u, ret = 0x%X",
           device_id_, event_group_id_, ret);
    is_timeout = false;
    return FAILED;
  }
  event_type = static_cast<ExecutorEventType>(in_event.subeventId);
  return SUCCESS;
}

Status EngineDaemon::InitMessageQueue() {
  GE_CHK_STATUS_RET(RtsApiUtils::MemQueueAttach(device_id_, msg_queue_id_, kDefaultTimeout),
                    "[Attach][MsgQueue] failed, device_id = %d, queue_id = %u", device_id_, msg_queue_id_);
  GELOGD("[Attach][MsgQueue] succeeded, device_id = %d, queue_id = %u", device_id_, msg_queue_id_);
  GE_CHK_STATUS_RET(RtsApiUtils::EschedCreateGroup(device_id_, event_group_id_, RT_GRP_TYPE_BIND_CP_CPU));
  uint64_t mask = 1ULL << static_cast<uint32_t>(RT_EVENT_QUEUE_ENQUEUE);
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubscribeEvent(device_id_, event_group_id_, 0, mask),
                    "[Subscribe][Event] failed, device_id = %d, group_id = %u", device_id_, event_group_id_);
  GELOGD("[Subscribe][Event] succeeded, device_id = %d, group_id = %u", device_id_, event_group_id_);
  rtEschedEventSummary_t to_drop;
  // need wait before submit of another process, this wait would time out, and it's OK
  (void) rtEschedWaitEvent(device_id_, event_group_id_, 0, 1, &to_drop);

  parent_pid_ = getppid();
  GELOGD("parent_pid = %d", parent_pid_);
  return SUCCESS;
}
Status EngineDaemon::NotifyInitialized() {
  GE_CHK_STATUS_RET(SendEvent(ExecutorEventType::kSuccess, "Init success"),
                    "[Notify][Initialized] submit event failed, device_id = %d", device_id_);
  GELOGD("[Notify][Initialized] success");
  return SUCCESS;
}

Status EngineDaemon::SendEvent(ExecutorEventType type, const std::string &msg) const {
  rtEschedEventSummary_t event_info{};
  event_info.eventId = RT_EVENT_QUEUE_ENQUEUE;
  event_info.pid = parent_pid_;
  event_info.grpId = event_group_id_;
  event_info.subeventId = static_cast<uint32_t>(type);
  if (!msg.empty()) {
    event_info.msg = const_cast<char *>(msg.c_str());
    event_info.msgLen = msg.length();
  } else {
    event_info.msg = nullptr;
    event_info.msgLen = 0;
  }
  // only one grpc server on device 0, send event to grpc server(device 0)
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubmitEvent(0, event_info),
                    "[Send][Event] failed, device_id = 0, type = %u", static_cast<uint32_t>(type));
  GELOGD("[Send][Event] succeeded, device_id = 0, type = %u", static_cast<uint32_t>(type));
  return SUCCESS;
}

Status EngineDaemon::InitializeGeExecutor() {
  ExecutionRuntime::DisableHeterogeneous();
  args_option_.emplace(OPTION_EXEC_IS_USEHCOM, "1");
  args_option_.emplace(OPTION_GRAPH_RUN_MODE, "0");
  GE_CHK_STATUS_RET(ge_executor_.Initialize(args_option_), "Failed to init ge executor");
  if (is_host_cpu_) {
    std::map<std::string, std::string> options {
      {"ge.exec.placement", "HOST"}
    };
    ge::GetThreadLocalContext().SetGlobalOption(options);
    GetContext().SetCtxDeviceId(static_cast<uint32_t>(-1));
  }
  return SUCCESS;
}
}  // namespace ge
